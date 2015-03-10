#include "stdafx.h"
#include "tray_icon_helper.h"

#include <cassert>

#include "macros.h"
#include "resource.h"

namespace
{
void DestroyIcon(void* h)
{
    if (h)
        ::DestroyIcon(reinterpret_cast<HICON>(h));
}
}

class TrayIconHelper::MessageCallbackWindow : public CWnd
{
public:
    MessageCallbackWindow(TrayIconHelper* owner, int callbackMessageId);

    bool Create();

private:
    virtual BOOL OnWndMsg(UINT message, WPARAM w, LPARAM l,
                          LRESULT* result) override;

    TrayIconHelper* owner_;
    int callbackMessageId_;
    const wchar_t* class_name_;
};

TrayIconHelper::MessageCallbackWindow::MessageCallbackWindow(
    TrayIconHelper* owner, int callbackMessageId)
    : owner_(owner)
    , callbackMessageId_(callbackMessageId)
    , class_name_(nullptr)
{
}

bool TrayIconHelper::MessageCallbackWindow::Create()
{
    if (!class_name_)
        class_name_ = AfxRegisterWndClass(0);

    return !!CWnd::CreateEx(0, class_name_, L"", 0, 0, 0,
                            0, 0, HWND_MESSAGE, nullptr, nullptr);
}

BOOL TrayIconHelper::MessageCallbackWindow::OnWndMsg(UINT message, WPARAM w,
                                                     LPARAM l, LRESULT* result)
{
    if (message == callbackMessageId_) {
        if (owner_)
            owner_->OnTrayMenuCallback(l);

        return TRUE;
    }

    return CWnd::OnWndMsg(message, w, l, result);
}

TrayIconHelper::TrayIconHelper(Delegate* delegate)
    : delegate_(delegate)
    , tray_icon_callback_message_(
        RegisterWindowMessage(L"RTXTweakerTrayIconCallback"))
    , icon_(nullptr, DestroyIcon)
{

}

TrayIconHelper::~TrayIconHelper()
{
    Remove();
}

int TrayIconHelper::GetCallbackMessageId() const
{
    return tray_icon_callback_message_;
}

void TrayIconHelper::OnTrayMenuCallback(int message)
{
    if (delegate_)
        delegate_->OnTrayMenuCallback(message);
}

void TrayIconHelper::Remove()
{
    if (!callback_handler_)
        return;

    NOTIFYICONDATA data = {};
    data.cbSize = sizeof(data);
    data.uFlags = NIF_INFO;
    data.dwInfoFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    data.uCallbackMessage = tray_icon_callback_message_;
    data.hIcon = reinterpret_cast<HICON>(icon_.get());
    data.hWnd = callback_handler_->GetSafeHwnd();

    Shell_NotifyIcon(NIM_DELETE, &data);
}

bool TrayIconHelper::Setup()
{
    if (!callback_handler_) {
        callback_handler_.reset(
            new MessageCallbackWindow(this, tray_icon_callback_message_));
        if (!callback_handler_->Create()) {
            callback_handler_.reset();
            return false;
        }
    }
    if (!icon_)
        icon_.reset(
            LoadIcon(GetModuleHandle(nullptr),
                     MAKEINTRESOURCE(IDR_MAINFRAME)));

    NOTIFYICONDATA data = {};
    data.cbSize = sizeof(data);
    data.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    data.uCallbackMessage = tray_icon_callback_message_;
    data.hIcon = reinterpret_cast<HICON>(icon_.get());
    data.hWnd = callback_handler_->GetSafeHwnd();

    return !!Shell_NotifyIcon(NIM_ADD, &data);
}

void TrayIconHelper::ShowBalloon(const std::wstring& title,
                                 const std::wstring& text)
{
    assert(callback_handler_);
    if (!callback_handler_)
        return;

    NOTIFYICONDATA data = {};
    data.cbSize = sizeof(data);
    data.uFlags = NIF_INFO;
    data.dwInfoFlags = NIIF_INFO;
    data.uCallbackMessage = tray_icon_callback_message_;
    data.hIcon = reinterpret_cast<HICON>(icon_.get());
    data.hWnd = callback_handler_->GetSafeHwnd();

    size_t size =
        min(title.length() * sizeof(title[0]),
            arraysize(data.szInfoTitle) * sizeof(data.szInfoTitle[0]));
    memcpy(data.szInfoTitle, title.c_str(), size);

    size =
        min(text.length() * sizeof(text[0]),
        arraysize(data.szInfo) * sizeof(data.szInfo[0]));
    memcpy(data.szInfo, text.c_str(), size);

    Shell_NotifyIcon(NIM_MODIFY, &data);
}
