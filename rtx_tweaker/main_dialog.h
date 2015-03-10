#pragma once

#include <functional>
#include <map>
#include <memory>

#include <stdint.h>

class MainDialog : public CDialogEx
{
public:
    class Delegate
    {
    public:
        virtual void OnWindowDestructed(HWND hwnd) = 0;
    };

    explicit MainDialog(Delegate* delegate);

    static int GetDestructionNotificationMessageId();

    int SetTimer(int intervalMS, const std::function<void (void)>& closure);
    void KillTimer(int timerHandle);

	enum { IDD = IDD_RTX_TWEAKER_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);

protected:
	HICON m_hIcon;

	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

private:
    struct TimerTask
    {
        int handle_;
        std::function<void(void)> closure_;
    };

    static const int kDestructionNotificationMessageId;

    void TimerProc(int id);
    afx_msg void OnTimer(UINT_PTR id);
    afx_msg void OnClose();
    LRESULT OnChatWindowDestructed(WPARAM w, LPARAM l);
    virtual void OnOK();

    Delegate* delegate_;
    int currentTimerId;
    std::map<int, std::unique_ptr<TimerTask>> timer_tasks_;
    virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
};