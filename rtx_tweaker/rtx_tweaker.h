#pragma once

#ifndef __AFXWIN_H__
	#error "在包含此文件之前包含“stdafx.h”以生成 PCH 文件"
#endif

#include "resource.h"

#include <hash_map>
#include <memory>

class RTXTweaker : public CWinApp
{
public:
	RTXTweaker();

public:
	virtual BOOL InitInstance();

	DECLARE_MESSAGE_MAP()

private:
    struct RTXWindow;
    class MainDialogDelegate;
    class TrayIconHelperDelegate;

    static void __stdcall WinEventProc(HWINEVENTHOOK hookHandle, DWORD event,
                                       HWND hwnd, LONG idObject, LONG idChild,
                                       DWORD idEventThread, DWORD eventTimeMS);

    bool Initialize();
    void CollectNewChatWindows();
    bool GetWindowTextFromOtherProcess(HWND hwnd, std::wstring* text);
    void HighlightChatWindows();
    void HighlightWindow(HWND hwnd);
    bool IsInBlacklist(const std::wstring& title);
    void MuteChatInBlacklist();
    void MuteWindow(HWND hwnd);
    void OnTrayMenuCallback(int message);
    void OnWindowDestructed(HWND hwnd);
    void Uninitialize();
    void UpdateChatWindows();
    bool UpdateWindowContent(HWND rtx_window_handle, RTXWindow* rtx_window);

    static HWND g_main_dialog_hwnd_;

    HWND rtx_main_window_;
    HWINEVENTHOOK event_hook_;
    std::hash_map<HWND, std::unique_ptr<RTXWindow>> rtx_windows_;
};

extern RTXTweaker theApp;