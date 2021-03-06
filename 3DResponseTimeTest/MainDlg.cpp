// MainDlg.cpp : implementation of the CMainDlg class
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"

#include "aboutdlg.h"
#include "MainDlg.h"

#include <chrono>
#include <vector>
#include <algorithm>
#include <numeric>

#define  DEFAULT_HOTKEY L"J"

HWND hDlg = NULL;
HHOOK mHook = NULL;
POINT hookPnt;
LRESULT CALLBACK LowLevelMouseProc(
	_In_ int    nCode,
	_In_ WPARAM wParam,
	_In_ LPARAM lParam
)
{
	if (wParam == WM_LBUTTONDOWN) {
		MSLLHOOKSTRUCT *pllhs = (MSLLHOOKSTRUCT*)lParam;
		hookPnt.x = pllhs->pt.x;
		hookPnt.y = pllhs->pt.y;

		UnhookWindowsHookEx(mHook);
		mHook = NULL;
		PostMessage(hDlg, WM_PICKED, 0, 0);
		hDlg = NULL;
		return TRUE;
	}

	return CallNextHookEx(mHook, nCode, wParam, lParam);
}

CMainDlg::CMainDlg()
	: m_testDuration(5)
	, m_testInterval(500)
	, m_pickColor(RGB(255, 255, 255))
	, m_stopTest(true)
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
	m_edHotkey.Attach(GetDlgItem(IDC_HOTKEY_ED));
	m_stPickColor.Attach(GetDlgItem(IDC_PICK_COLOR_ST));

	m_edHotkey.SetWindowText(DEFAULT_HOTKEY);

	HWND hTopWnd = getTopWindow();
	CRect rect;
	::GetWindowRect(hTopWnd, &rect);
	m_pickPnt.x = rect.left + rect.Width() / 2;
	m_pickPnt.y = rect.top + rect.Height() / 2;
	CString str;
	str.Format(L"(%d, %d)", m_pickPnt.x, m_pickPnt.y);
	m_edPickPoint.SetWindowText(str);

	m_pickColor = pickColor(m_pickPnt);
	setPickColor(m_pickColor);

	return TRUE;
}

LRESULT CMainDlg::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	stopThread();

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
	stopThread();

	HWND hWnd = ::WindowFromPoint(m_pickPnt);
	//::SetActiveWindow(hWnd);
	::SetForegroundWindow(hWnd);

	m_stopTest = false;
	m_thread.reset(new std::thread(&CMainDlg::runTest, this));
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
	if (mHook != NULL)
		return 0;
	mHook = SetWindowsHookEx(WH_MOUSE_LL, &LowLevelMouseProc, GetModuleHandle(NULL), NULL);
	hDlg = m_hWnd;
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


void CMainDlg::setPickColor(COLORREF color)
{
	CDCHandle dc = m_stPickColor.GetDC();
	CRect rect;
	m_stPickColor.GetClientRect(rect);
	CBrush brush;
	brush.CreateSolidBrush(color);
	dc.FillRect(rect, brush);
}

WORD CMainDlg::getHotkey()
{
	CString str;
	auto len = m_edHotkey.GetWindowTextLength();
	++len;
	m_edHotkey.GetWindowText(str.GetBuffer(len), len);
	str.ReleaseBuffer();
	return str[0];
}

LRESULT CMainDlg::OnPicked(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	m_pickPnt = hookPnt;
	m_pickColor = pickColor(m_pickPnt);
	setPickColor(m_pickColor);

	CString str;
	str.Format(L"(%d, %d)", m_pickPnt.x, m_pickPnt.y);
	m_edPickPoint.SetWindowText(str);
	return 0;
}

LRESULT CMainDlg::OnTestDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
	::SetWindowText(GetDlgItem(IDC_REPORT_ED), m_report);
	::SetForegroundWindow(m_hWnd);
	stopThread();
	return 0;
}

void CMainDlg::runTest()
{
	CPoint pickPnt(m_pickPnt);
	COLORREF testColor = pickColor(pickPnt);
	std::chrono::seconds testDuration(m_testDuration);
	std::chrono::milliseconds testInterval(m_testInterval);
	std::vector<std::chrono::milliseconds> responseTimes;
	WORD hotkey = m_hostKey;
	UINT scanCode = 1 | (MapVirtualKeyEx((UINT)hotkey, MAPVK_VK_TO_VSC_EX, GetKeyboardLayout(0)) << 16);

	INPUT input = { 0 };
	input.type = INPUT_KEYBOARD;
	input.ki.wVk = hotkey;
	input.ki.wScan = scanCode;
	
	auto startTestTime = std::chrono::high_resolution_clock::now();
	auto prevTestTime = startTestTime;
	while (!m_stopTest) {
		auto nowTime = std::chrono::high_resolution_clock::now();
		auto duration = nowTime - startTestTime;
		if (duration > testDuration) {
			break;
		}

		input.ki.dwFlags = 0;
		SendInput(1, &input, sizeof(input));
		input.ki.dwFlags = KEYEVENTF_KEYUP;
		SendInput(1, &input, sizeof(input));

		auto sendTime = std::chrono::high_resolution_clock::now();
		nowTime = sendTime;
		while (nowTime - sendTime <= testInterval) {
			auto color = pickColor(pickPnt);
			if (color != testColor) {
				responseTimes.push_back(std::chrono::duration_cast<std::chrono::milliseconds>(nowTime - sendTime));
				testColor = color;
				break;
			}

			std::this_thread::sleep_for(std::chrono::microseconds(1));
			nowTime = std::chrono::high_resolution_clock::now();
		}

		nowTime = std::chrono::high_resolution_clock::now();
		duration = nowTime - prevTestTime;
		if (duration < testInterval) {
			std::this_thread::sleep_for(testInterval - duration);
		}

		prevTestTime = sendTime;
	}

	auto minmax = std::minmax_element(responseTimes.begin(), responseTimes.end());
	decltype(responseTimes)::value_type total(0);
	for (auto &t : responseTimes)
		total += t;

	m_report.Empty();
	if (responseTimes.empty()) {
		m_report = L"无测试结果！";
	}
	else {
		constexpr int bufferSize = 1024;
		swprintf_s(m_report.GetBuffer(bufferSize), bufferSize,
			L"最小响应：%I64d ms\r\n最大响应：%I64d ms\r\n平均响应：%.2lf ms\r\n测试次数：%d",
			minmax.first->count(), minmax.second->count(), (double)total.count() / responseTimes.size(),
			responseTimes.size());
		m_report.ReleaseBuffer();
	}

	PostMessage(WM_TEST_DWON);
}

void CMainDlg::stopThread()
{
	if (m_thread == nullptr)
		return;

	m_stopTest = true;
	if (m_thread->joinable()) {
		m_thread->join();
	}
}

LRESULT CMainDlg::OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
{
	bHandled = FALSE;
	setPickColor(m_pickColor);
	return 0;
}


LRESULT CMainDlg::OnEnChangeHotkeyEd(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
	auto len = m_edHotkey.GetWindowTextLength();
	if (len == 0) {
		m_edHotkey.SetWindowText(DEFAULT_HOTKEY);
		return 0;
	}

	CString str;
	++len;
	m_edHotkey.GetWindowText(str.GetBuffer(len), len);
	str.ReleaseBuffer();

	if (str.GetLength() > 1) {
		str.Delete(1, len - 1);
		m_edHotkey.SetWindowText(str);
	}

	if (islower(str[0])) {
		str.Replace(str[0], toupper(str[0]));
		m_edHotkey.SetWindowText(str);
	}
	m_hostKey = str[0];
	return 0;
}
