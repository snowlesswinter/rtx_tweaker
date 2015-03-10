#include "stdafx.h"

#include <cassert>

#include "rtx_tweaker.h"
#include "main_dialog.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

const int MainDialog::kDestructionNotificationMessageId = WM_USER + 1024;

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);

protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


MainDialog::MainDialog(Delegate* delegate)
	: CDialogEx(MainDialog::IDD, nullptr)
    , delegate_(delegate)
    , currentTimerId(0)
    , timer_tasks_()
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void MainDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(MainDialog, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
    ON_WM_TIMER()
    ON_WM_CLOSE()
    ON_MESSAGE(kDestructionNotificationMessageId,
               &MainDialog::OnChatWindowDestructed)
END_MESSAGE_MAP()

BOOL MainDialog::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	SetIcon(m_hIcon, TRUE);
	SetIcon(m_hIcon, FALSE);

	return TRUE;
}

void MainDialog::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

void MainDialog::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this);

		SendMessage(WM_ICONERASEBKGND,
                    reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

HCURSOR MainDialog::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

int MainDialog::SetTimer(int intervalMS,
                              const std::function<void(void)>& closure)
{
    int id = CWnd::SetTimer(++currentTimerId, intervalMS, nullptr);
    assert(id == currentTimerId);
    std::unique_ptr<TimerTask> task(new TimerTask());
    task->closure_ = closure;
    task->handle_ = 0;
    timer_tasks_.insert(std::make_pair(id, std::move(task)));
    return id;
}

void MainDialog::KillTimer(int timerHandle)
{
    assert(timer_tasks_.find(timerHandle) != timer_tasks_.end());
    timer_tasks_.erase(timerHandle);

    CWnd::KillTimer(timerHandle);
}

void MainDialog::TimerProc(int id)
{
    auto i = timer_tasks_.find(id);
    assert(i != timer_tasks_.end());
    if (i != timer_tasks_.end()) {
        if (i->second->closure_)
            i->second->closure_();
    }
}

void MainDialog::OnTimer(UINT_PTR id)
{
    TimerProc(id);
    CDialogEx::OnTimer(id);
}

LRESULT MainDialog::OnChatWindowDestructed(WPARAM w, LPARAM l)
{
    HWND chat_window_hwnd = reinterpret_cast<HWND>(w);
    if (delegate_)
        delegate_->OnWindowDestructed(chat_window_hwnd);

    return 1;
}

void MainDialog::OnOK()
{
    PostMessage(WM_QUIT);

    CDialogEx::OnOK();
}


void MainDialog::OnClose()
{
    ShowWindow(SW_MINIMIZE);
    Sleep(200);
    ShowWindow(SW_HIDE);
}

int MainDialog::GetDestructionNotificationMessageId()
{
    return kDestructionNotificationMessageId;
}

BOOL MainDialog::OnCommand(WPARAM wParam, LPARAM lParam)
{
    if (wParam == TRAY_MENU_SHOW) {
        ShowWindow(SW_NORMAL);
    } else if (wParam == TRAY_MENU_EXIT) {
        PostMessage(WM_QUIT);
    }

    return CDialogEx::OnCommand(wParam, lParam);
}