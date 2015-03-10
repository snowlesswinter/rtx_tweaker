#include "stdafx.h"

#include <cassert>
#include <iostream>
#include <string>

#include "macros.h"
#include "rtx_predefines.h"
#include "rtx_tweaker.h"
#include "main_dialog.h"
#include "tray_icon_helper.h"
#include "tray_icon_menu.h"
#include "blacklist.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using std::wcout;
using std::endl;

class RTXTweaker::MainDialogDelegate : public MainDialog::Delegate
{
public:
    MainDialogDelegate(RTXTweaker* app)
        : app_(app)
    {
    }

    virtual void OnWindowDestructed(HWND hwnd) override
    {
        app_->OnWindowDestructed(hwnd);
    }

private:
    RTXTweaker* app_;
};

class RTXTweaker::TrayIconHelperDelegate : public TrayIconHelper::Delegate
{
public:
    TrayIconHelperDelegate(RTXTweaker* app)
        : app_(app)
    {
    }

    virtual void OnTrayMenuCallback(int message) override
    {
        app_->OnTrayMenuCallback(message);
    }

private:
    RTXTweaker* app_;
};

HWND RTXTweaker::g_main_dialog_hwnd_ = nullptr;

struct RTXTweaker::RTXWindow
{
    RTXWindow()
        : mute_(false)
        , richedit_window_(nullptr)
        , title_()
        , content_()
    {
    }

    bool mute_;
    HWND richedit_window_;
    std::wstring title_;
    std::wstring content_;
};

BEGIN_MESSAGE_MAP(RTXTweaker, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()

RTXTweaker::RTXTweaker()
    : rtx_main_window_(nullptr)
    , event_hook_(nullptr)
    , rtx_windows_()
{
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;
}

RTXTweaker theApp;

BOOL RTXTweaker::InitInstance()
{
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);

	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();


	AfxEnableControlContainer();

	CShellManager *pShellManager = new CShellManager;

	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));

	SetRegistryKey(_T("应用程序向导生成的本地应用程序"));

    if (!Initialize()) {
        MessageBox(nullptr, L"Failed to initialize application", L"ERROR",
                   MB_OK);
        return FALSE;
    }

    MainDialogDelegate main_dialog_delegate(this);
    MainDialog main_dialog(&main_dialog_delegate);
    m_pMainWnd = &main_dialog;

    if (!main_dialog.Create(IDD_RTX_TWEAKER_DIALOG)) {
        MessageBox(nullptr, L"Failed to initialize main dialog", L"ERROR",
                   MB_OK);
        return FALSE;
    }

    g_main_dialog_hwnd_ = main_dialog.GetSafeHwnd();

    TrayIconHelperDelegate tray_helper_delegate(this);
    TrayIconHelper tray_icon(&tray_helper_delegate);
    tray_icon.Setup();
    tray_icon.ShowBalloon(std::wstring(), L"RTX Tweaker is now working");

    main_dialog.SetTimer(
        3000, std::bind(&RTXTweaker::UpdateChatWindows, this));
    main_dialog.SetTimer(
        1000, std::bind(&RTXTweaker::HighlightChatWindows, this));
    main_dialog.SetTimer(
        2000, std::bind(&RTXTweaker::MuteChatInBlacklist, this));
    main_dialog.ShowWindow(SW_NORMAL);

    MSG message;
    while (GetMessage(&message, nullptr, 0, 0)) {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }

    Uninitialize();

	if (pShellManager != NULL)
	{
		delete pShellManager;
	}

	return FALSE;
}

void RTXTweaker::WinEventProc(HWINEVENTHOOK hookHandle, DWORD event,
                                   HWND hwnd, LONG idObject, LONG idChild,
                                   DWORD idEventThread, DWORD eventTimeMS)
{
    if (event == EVENT_OBJECT_DESTROY && idObject == OBJID_WINDOW &&
        idChild == INDEXID_CONTAINER) {
        PostMessage(g_main_dialog_hwnd_,
                    MainDialog::GetDestructionNotificationMessageId(),
                    reinterpret_cast<WPARAM>(hwnd), 0);
    }
}

bool RTXTweaker::Initialize()
{
    wcout.imbue(std::locale("chs"));

    rtx_main_window_ = FindWindowEx(nullptr, nullptr, L"#32770", nullptr);
    if (!rtx_main_window_)
        return false;

    DWORD processId = 0;
    DWORD threadId = GetWindowThreadProcessId(rtx_main_window_, &processId);
    if (!threadId)
        return false;

    event_hook_ = SetWinEventHook(EVENT_OBJECT_DESTROY, EVENT_OBJECT_DESTROY,
                                  nullptr, &RTXTweaker::WinEventProc,
                                  processId, threadId, WINEVENT_OUTOFCONTEXT);
    return !!event_hook_;
}

void RTXTweaker::CollectNewChatWindows()
{
    const wchar_t* sysDialogClassName =
        RTXPredefines::GetChatWindowClassName();
    HWND window_handle = FindWindowEx(nullptr, nullptr, sysDialogClassName,
                                      nullptr);
    while (window_handle) {
        if (rtx_windows_.find(window_handle) == rtx_windows_.end()) {
            std::wstring title(256, '\0');
            GetWindowTextFromOtherProcess(window_handle, &title);
            const wchar_t* title_keyword =
                RTXPredefines::GetChatWindowTitleKeyword();
            if (title.find(title_keyword) != title.npos) {
                wcout << "window found " << title << endl;
                std::unique_ptr<RTXWindow> newChatWindow(new RTXWindow());
                newChatWindow->mute_ = Blacklist::IsInBlacklist(title);
                newChatWindow->title_.swap(title);
                rtx_windows_.insert(
                    std::make_pair(window_handle, std::move(newChatWindow)));
            }
        }

        window_handle = FindWindowEx(nullptr, window_handle,
                                     sysDialogClassName, nullptr);
    }
}

bool RTXTweaker::GetWindowTextFromOtherProcess(HWND hwnd,
                                                    std::wstring* text)
{
    if (!text)
        return false;

    int capacity = max(text->capacity(), 256);
    size_t size = 0;
    do 
    {
        text->resize(capacity - 1);

        // TODO: Handle text longer than 64K with EM_STREAMOUT. Need injection.
        size = SendMessage(hwnd, WM_GETTEXT, capacity,
                           reinterpret_cast<LPARAM>(&(*text)[0]));
        if (size > 0 && size < text->size()) {
            text->resize(size);
            return true;
        }

        capacity *= 2;
    } while (size > 0);
    
    return false;
}

void RTXTweaker::HighlightChatWindows()
{
    for (auto i = rtx_windows_.begin(); i != rtx_windows_.end(); ++i) {
        if (!i->second->mute_) {
            if (UpdateWindowContent(i->first, i->second.get()))
                HighlightWindow(i->first);
        }
    }
}

void RTXTweaker::HighlightWindow(HWND hwnd)
{
    // Windows has a obscene bug that FlashWindow() may do NOTHING if we only
    // call it one time.
    FlashWindow(hwnd, FALSE);
    FlashWindow(hwnd, FALSE);
}

void RTXTweaker::MuteChatInBlacklist()
{
    for (auto i = rtx_windows_.begin(); i != rtx_windows_.end(); ++i) {
        if (i->second->mute_) {
            MuteWindow(i->first);
        }
    }
}

void RTXTweaker::MuteWindow(HWND hwnd)
{
    FLASHWINFO flash_info = {};
    flash_info.cbSize = sizeof(flash_info);
    flash_info.hwnd = hwnd;
    flash_info.dwFlags = FLASHW_STOP; // Stop flashing.
    FlashWindowEx(&flash_info);
}

void RTXTweaker::OnTrayMenuCallback(int message)
{
    if (message == WM_RBUTTONUP) {
        POINT pos = {};
        GetCursorPos(&pos);

        TrayIconMenu menu;
        menu.Create();
        menu.Popup(m_pMainWnd, pos.x, pos.y);
    } else if (message == WM_LBUTTONDBLCLK) {
        m_pMainWnd->ShowWindow(SW_NORMAL);
        m_pMainWnd->SetForegroundWindow();
    }
}

void RTXTweaker::OnWindowDestructed(HWND hwnd)
{
    rtx_windows_.erase(hwnd);
}

void RTXTweaker::Uninitialize()
{
    if (event_hook_) {
        UnhookWinEvent(event_hook_);
        event_hook_ = nullptr;
    }
}

void RTXTweaker::UpdateChatWindows()
{
    CollectNewChatWindows();
}

bool RTXTweaker::UpdateWindowContent(HWND rtx_window_handle,
                                          RTXWindow* rtx_window)
{
    assert(rtx_window);
    if (!rtx_window)
        return false;

    if (!rtx_window->richedit_window_) {
        HWND child = FindWindowEx(rtx_window_handle, nullptr, L"Static",
                                  nullptr);
        assert(!!child);
        if (child) {
            HWND richedit = FindWindowEx(
                child, nullptr,
                RTXPredefines::GetChatWindowRichEditClassName(), nullptr);

            assert(richedit);
            if (richedit) {
                rtx_window->richedit_window_ = richedit;
                GetWindowTextFromOtherProcess(richedit,
                                              &rtx_window->content_);

                // When the first time we detect a new chat window we will not
                // make it flash, because we don't know whether the content has
                // changed since "last time"(we are not even aware that when
                // the last time is).
                return false;
            }
        }
    } else {
        int old_length = rtx_window->content_.length();
        if (GetWindowTextFromOtherProcess(rtx_window->richedit_window_,
                                          &rtx_window->content_)) {
            return old_length != rtx_window->content_.length();
        }
    }

    return false;
}
