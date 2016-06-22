// MainDlg.cpp : implementation of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "aboutdlg.h"
#include "MainDlg.h"

CMainDlg::CMainDlg()
	: m_testDuration(5)
	, m_pickColor(RGB(255, 255, 255))
{

}

BOOL CMainDlg::PreTranslateMessage(MSG* pMsg)
{
	return CWindow::IsDialogMessage(pMsg);
}

BOOL CMainDlg::OnIdle()
{
	UIUpdateChildWindows();
	return FALSE;
}

LRESULT CMainDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	DoDataExchange();
	// center the dialog on the screen
	CenterWindow();

	// set icons
	HICON hIcon = AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON));
	SetIcon(hIcon, TRUE);
	HICON hIconSmall = AtlLoadIconImage(IDR_MAINFRAME, LR_DEFAULTCOLOR, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON));
	SetIcon(hIconSmall, FALSE);

	// register object for message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->AddMessageFilter(this);
	pLoop->AddIdleHandler(this);

	UIAddChildWindowContainer(m_hWnd);

	m_edPickPoint.Attach(GetDlgItem(IDC_PICK_POINT_ED));

	HWND hTopWnd = getTopWindow();
	CRect rect;
	::GetWindowRect(hTopWnd, &rect);
	m_pickPnt.x = rect.left + rect.Width() / 2;
	m_pickPnt.y = rect.top + rect.Height() / 2;
	CString str;
	str.Format(L"(%d, %d)", m_pickPnt.x, m_pickPnt.y);
	m_edPickPoint.SetWindowText(str);

	m_pickColor = pickColor(m_pickPnt);

	return TRUE;
}

LRESULT CMainDlg::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	// unregister message filtering and idle updates
	CMessageLoop* pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop != NULL);
	pLoop->RemoveMessageFilter(this);
	pLoop->RemoveIdleHandler(this);

	return 0;
}

LRESULT CMainDlg::OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CAboutDlg dlg;
	dlg.DoModal();
	return 0;
}

LRESULT CMainDlg::OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	// TODO: Add validation code 
	CloseDialog(wID);
	return 0;
}

LRESULT CMainDlg::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	CloseDialog(wID);
	return 0;
}

void CMainDlg::CloseDialog(int nVal)
{
	DestroyWindow();
	::PostQuitMessage(nVal);
}


LRESULT CMainDlg::OnBnClickedPickBtn(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	return 0;
}


LRESULT CMainDlg::OnDeltaPosTestDurationSpin(int /*idCtrl*/, LPNMHDR pNMHDR, BOOL& /*bHandled*/)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	// TODO: Add your control notification handler code here
	
	if (pNMUpDown->iDelta < 0 && abs(pNMUpDown->iDelta) >= m_testDuration)
		m_testDuration = 0;
	else
		m_testDuration += pNMUpDown->iDelta;

	DoDataExchange();
	return 0;
}

HWND CMainDlg::getTopWindow()
{
	HWND hWnd = ::GetForegroundWindow();
	if (hWnd == m_hWnd)
		hWnd = ::GetWindow(hWnd, GW_HWNDNEXT);
	return hWnd;
}

COLORREF CMainDlg::pickColor(const CPoint &pnt)
{
	HDC hDC = ::GetDC(NULL);
	COLORREF color = ::GetPixel(hDC, pnt.x, pnt.y);
	::ReleaseDC(NULL, hDC);
	return color;
}


LRESULT CMainDlg::OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	bHandled = FALSE;
	HWND hWnd = GetDlgItem(IDC_PICK_COLOR_ST);
	CRect rect;
	::GetClientRect(hWnd, &rect);
	HDC hDC = ::GetDC(hWnd);

	HBRUSH hBrush = ::CreateSolidBrush(m_pickColor);
	::FillRect(hDC, rect, hBrush);
	::DeleteObject(hBrush);

	::ReleaseDC(hWnd, hDC);
	return 0;
}
