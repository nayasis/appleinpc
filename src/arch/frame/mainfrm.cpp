// MainFrm.cpp : implementation of the CMainFrame class
//

#include <afxmt.h>
#include <windows.h>
#include <windowsx.h>

#include "arch/frame/stdafx.h"
#include "arch/frame/mainfrm.h"
#include "arch/frame/aipc.h"
#include "arch/directx/dibase.h"
#include "arch/directx/dxsound.h"
#include "arch/directx/dijoystick.h"
#include "arch/directx/dikeyboard.h"
#include "arch/directx/dimouse.h"
#include "keyboard.h"
#include "card.h"
#include "phasor.h"
#include "memory.h"
#include "aipcdefs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CDIJoystick g_cDIJoystick;
CDIKeyboard g_cDIKeyboard;
CDIMouse	g_cDIMouse;
int g_nSerializeVer = 0;

static CString GetStatusFilePath();

#define STATUS_VERSION		(5)
#define STATUS_MIN_VERSION	(3)
#define STATUS_MAGIC	0x89617391

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_COMMAND(ID_DEBUG, OnDebug)
	ON_WM_KEYDOWN()
	ON_WM_GETMINMAXINFO()
	ON_COMMAND(ID_CONFIGURE_SLOTS, OnConfigureSlots)
	ON_COMMAND(ID_REBOOT, OnReboot)
	ON_COMMAND(ID_RESET, OnReset)
	ON_COMMAND(ID_MONITOR, OnMonitor)
	ON_COMMAND(ID_2XSCREEN, OnChangeSize)
	ON_UPDATE_COMMAND_UI(ID_MONITOR, OnUpdateMonitor)
	ON_COMMAND(ID_POWERON, OnPowerOn)
	ON_UPDATE_COMMAND_UI(ID_POWERON, OnUpdatePowerOn)
	ON_COMMAND(ID_POWEROFF, OnPowerOff)
	ON_UPDATE_COMMAND_UI(ID_POWEROFF, OnUpdatePowerOff)
	ON_UPDATE_COMMAND_UI(ID_RESET, OnUpdateReset)
	ON_WM_KEYUP()
	ON_COMMAND(ID_POWER, OnPower)
	ON_COMMAND(ID_DISKETTE, OnDiskette)
	ON_UPDATE_COMMAND_UI(ID_POWER, OnUpdatePower)
	ON_WM_CLOSE()
	ON_MESSAGE(UM_REQACQUIRE, OnReqAcquire)
	ON_MESSAGE(UM_KEYDOWN, OnMyKeyDown)
	ON_MESSAGE(UM_KEYUP, OnMyKeyUp)
	ON_MESSAGE(UM_KEYREPEAT, OnMyKeyRepeat)
	ON_COMMAND(ID_FULL_SCREEN, OnToggleFullScreen)
	ON_WM_KILLFOCUS()
	ON_COMMAND(ID_HARDDISK, OnHarddisk)
	ON_UPDATE_COMMAND_UI(ID_DISKETTE, OnUpdateDiskette)
	ON_UPDATE_COMMAND_UI(ID_HARDDISK, OnUpdateHarddisk)
	ON_UPDATE_COMMAND_UI(ID_SCANLINE, OnUpdateScanline)
	ON_COMMAND(ID_SCANLINE, OnScanline)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	// TODO: add member initialization code here
	m_winRect.SetRectEmpty();
	m_bDoubleSize = FALSE;
	m_bFullScreen = FALSE;
	g_pBoard = new CAppleClock();
}

CMainFrame::~CMainFrame()
{
	delete g_pBoard;
	CWnd::~CWnd();
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	g_pBoard->m_lpwndMainFrame = this;
	g_pBoard->m_pScreen = &m_wndView;

	if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP
		| CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
		!m_wndToolBar.LoadToolBar(IDR_TOOLBAR))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	if (!m_wndStatusBar.Create(this))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}
	
	// TODO: Delete these three lines if you don't want the toolbar to
	//  be dockable
//	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
//	EnableDocking(CBRS_ALIGN_ANY);
//	DockControlBar(&m_wndToolBar);

	m_wndToolBar.SetHeight( 32 );
	// create a view to occupy the client area of the frame
	if (!m_wndView.Create(NULL, NULL, AFX_WS_DEFAULT_VIEW,
		CRect(0, 0, 0, 0), this, AFX_IDW_PANE_FIRST, NULL))
	{
		TRACE0("Failed to create view window\n");
		return -1;
	}

	g_DXSound.Create(this->GetSafeHwnd(), DSSCL_NORMAL);

	// Initialise Joystick
	g_cDIJoystick.SetHWND(m_hWnd);
	if ( g_cDIJoystick.InitJoystick() )
		g_pBoard->m_joystick.m_bHasPCJoystick = TRUE;

	// Initialise Keyboard
	g_cDIKeyboard.SetHWND(m_hWnd);
	g_cDIKeyboard.InitKeyboard();

	// Initialise Mouse
	g_cDIMouse.SetHWND(m_hWnd);
	g_cDIMouse.SetCoOpLevel(TRUE);			// exclusive mode
	g_cDIMouse.InitMouse();

	m_cMenu.LoadMenu( IDR_MAINFRAME );
	SetMenu( &m_cMenu );


	g_pBoard->Initialize();
//	g_pBoard->CreateThread();
//	g_pBoard->SetThreadPriority(THREAD_PRIORITY_ABOVE_NORMAL);


	CFile file;
	CString strStatusFile = GetStatusFilePath();
	if ( file.Open( strStatusFile, CFile::modeRead ) )
	{
		int nVal, nVal2;
		BOOL bError = FALSE;
		CArchive ar(&file, CArchive::load);
		try
		{
			ar >> nVal;
			ar >> nVal2;
			if ( nVal != STATUS_MAGIC || nVal2 < STATUS_MIN_VERSION )
			{
				throw new CArchiveException();
			}
			g_nSerializeVer = nVal2;

			ar >> m_bDoubleSize;
			g_pBoard->Serialize(ar);
			ar >> nVal;
			g_DXSound.SetPan( nVal );
			ar >> nVal;
			g_DXSound.SetVolume( nVal );
			ar >> g_DXSound.m_bMute;
			ar >> nVal;
			ar >> nVal2;
			g_cDIKeyboard.SetDelayTime( nVal, nVal2 );
		}
		catch (CFileException* fe)
		{
			(void)fe;
			bError = TRUE;
		}
		catch (CArchiveException* ae)
		{
			(void)ae;
			bError = TRUE;
		}
		ar.Close();
		file.Close();
		if ( bError == TRUE )
		{
			CString strMessage;
			this->MessageBox( TEXT("Fail to load last state"), TEXT("Error - Apple in PC"), MB_OK );
			strMessage.Format( TEXT("Remove status file to fix this error:\n%s"), strStatusFile );
			this->MessageBox( strMessage, TEXT("Error - Apple in PC"), MB_OK );
			exit(1);
			return 1;
		}
	}

	RECT rc;
	::GetWindowRect( m_hWnd, &rc );
	m_stWindowPos.x = rc.left;
	m_stWindowPos.y = rc.top;

	ResizeWindow();

	g_pBoard->Resume();

	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	cs.dwExStyle &= ~WS_EX_CLIENTEDGE;
	cs.lpszClass = AfxRegisterWndClass(0);
	cs.style &= ~WS_THICKFRAME;
	cs.style &= ~WS_MAXIMIZEBOX;

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG

BOOL CMainFrame::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	// let the view have first crack at the command
	if (m_wndView.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
		return TRUE;

	// otherwise, do default handling
	return CFrameWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}


void CMainFrame::OnDebug() 
{
	// TODO: Add your command handler code here
	g_pBoard->OnDebug();
}

void CMainFrame::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	// TODO: Add your message handler code here and/or call default
//	g_pBoard->m_keyboard.OnKeyDown(nChar, nFlags);
	CFrameWnd::OnKeyDown(nChar, nRepCnt, nFlags);
}


void CMainFrame::OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI) 
{
	CFrameWnd::OnGetMinMaxInfo(lpMMI);
	// TODO: Add your message handler code here and/or call default
	if( !m_winRect.IsRectEmpty() && g_pBoard->m_pScreen->m_bWindowed ){
		lpMMI->ptMaxSize.x = m_winRect.Width();
		lpMMI->ptMaxSize.y = m_winRect.Height();
		lpMMI->ptMinTrackSize.x = m_winRect.Width();
		lpMMI->ptMinTrackSize.y = m_winRect.Height();
		lpMMI->ptMaxTrackSize.x = m_winRect.Width();
		lpMMI->ptMaxTrackSize.y = m_winRect.Height();
	}
}

void CMainFrame::CalcWindowRect(LPRECT lpClientRect, UINT nAdjustType) 
{
	// TODO: Add your specialized code here and/or call the base class
	CFrameWnd::CalcWindowRect(lpClientRect, nAdjustType);
}


void CMainFrame::OnClose()
{
	CFile file;
	CString strStatusFile = GetStatusFilePath();
	if ( file.Open( strStatusFile, CFile::modeCreate | CFile::modeWrite ) )
	{
		CArchive ar(&file, CArchive::store);
		int nVal, nVal2;
		try
		{
			g_nSerializeVer = STATUS_VERSION;

			ar << STATUS_MAGIC;
			ar << STATUS_VERSION;
			ar << m_bDoubleSize;
			g_pBoard->Serialize(ar);
			ar << g_DXSound.GetPan();
			ar << g_DXSound.GetVolume();
			ar << g_DXSound.m_bMute;
			g_cDIKeyboard.GetDelayTime( &nVal, &nVal2 );
			ar << nVal;
			ar << nVal2;
		}
		catch (CFileException* fe)
		{
			(void)fe;
			this->MessageBox( TEXT("Fail to store current state"), TEXT("Error - Apple in PC"), MB_OK );
		}
		catch (CArchiveException* ae)
		{
			(void)ae;
			this->MessageBox( TEXT("Fail to store current state"), TEXT("Error - Apple in PC"), MB_OK );
		}
		ar.Close();
		file.Close();
	}
	g_pBoard->Exit();

	CFrameWnd::OnClose();
}

void CMainFrame::OnConfigureSlots() 
{
	// TODO: Add your command handler code here
	g_pBoard->OnConfigureSlots();
}

void CMainFrame::OnReboot() 
{
	// TODO: Add your command handler code here
	g_pBoard->Reboot();
}

void CMainFrame::OnReset() 
{
	// TODO: Add your command handler code here
	g_pBoard->Reset();
}

void CMainFrame::OnMonitor() 
{
	// TODO: Add your command handler code here
	g_pBoard->ChangeMonitorType();
}

void CMainFrame::OnChangeSize()
{
	m_bDoubleSize = !m_bDoubleSize;
	ResizeWindow();
}

void CMainFrame::OnUpdateMonitor(CCmdUI* pCmdUI) 
{
	// TODO: Add your command update UI handler code here
	int mode=g_pBoard->GetMonitorType();
	if ( mode == SM_COLOR )
		pCmdUI->SetText(_T("Change &Monitor to Color 2\tF8"));
	else if ( mode == SM_COLOR2 )
		pCmdUI->SetText(_T("Change &Monitor to White\tF8"));
	else if( mode == SM_WHITE )
		pCmdUI->SetText(_T("Change &Monitor to Green\tF8"));
	else
		pCmdUI->SetText(_T("Change &Monitor to Color\tF8"));
}

void CMainFrame::OnPowerOn() 
{
	// TODO: Add your command handler code here
	g_pBoard->PowerOn();
}

void CMainFrame::OnUpdatePowerOn(CCmdUI* pCmdUI) 
{
	// TODO: Add your command update UI handler code here
	if ( g_pBoard->GetIsActive() )
		pCmdUI->Enable( FALSE );
	else
		pCmdUI->Enable( TRUE );
}

void CMainFrame::OnPowerOff() 
{
	// TODO: Add your command handler code here
	g_pBoard->PowerOff();
}

void CMainFrame::OnUpdatePowerOff(CCmdUI* pCmdUI) 
{
	// TODO: Add your command update UI handler code here
	if ( g_pBoard->GetIsActive() )
		pCmdUI->Enable( TRUE );
	else
		pCmdUI->Enable( FALSE );
}

void CMainFrame::OnUpdateReset(CCmdUI* pCmdUI) 
{
	// TODO: Add your command update UI handler code here
	if ( g_pBoard->GetIsActive() )
		pCmdUI->Enable( TRUE );
	else
		pCmdUI->Enable( FALSE );
}

void CMainFrame::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	// TODO: Add your message handler code here and/or call default
	
	g_pBoard->m_keyboard.OnKeyUp(nChar, nFlags);
	CFrameWnd::OnKeyUp(nChar, nRepCnt, nFlags);
}


void CMainFrame::OnPower() 
{
	// TODO: Add your command handler code here
	if ( g_pBoard->GetIsActive() )
		g_pBoard->PowerOff();
	else
		g_pBoard->PowerOn();

}

void CMainFrame::OnDiskette() 
{
	// TODO: Add your command handler code here
	g_pBoard->m_cSlots.ConfigureDiskette();
}


void CMainFrame::OnHarddisk() 
{
	// TODO: Add your command handler code here
	g_pBoard->m_cSlots.ConfigureHardDisk();
}

void CMainFrame::OnUpdatePower(CCmdUI* pCmdUI) 
{
	// TODO: Add your command update UI handler code here
	if ( g_pBoard->GetIsActive() )
		pCmdUI->SetText("&Power Off\tctrl-F11");
	else
		pCmdUI->SetText("&Power On\tctrl-F11");
}

LRESULT CMainFrame::OnReqAcquire(WPARAM wParam, LPARAM lParam)
{
	CDIBase* cDIBase = (CDIBase*)lParam;
	if ( wParam )
	{
		if ( cDIBase != NULL )
		{
			if ( cDIBase->Acquire( TRUE ) )
			{
				return 0;
			}
		}
	}
//	else
	{
		if ( cDIBase != NULL )
		{
			cDIBase->Acquire( FALSE );
		}
		g_cDIMouse.SetActive( FALSE, FALSE );	// don't wait for mouse exit. mouse will wait for unacquiring it self.
		g_cDIKeyboard.SetActive( FALSE, FALSE );	// same
		if ( m_bFullScreen == TRUE )
		{
			m_bFullScreen = FALSE;
			m_wndView.SetFullScreenMode( FALSE );
			ResizeWindow();
		}
	}
	return 0;
}

LRESULT CMainFrame::OnMyKeyDown(WPARAM wParam, LPARAM lParam)
{
	g_pBoard->m_keyboard.OnKeyDown(wParam, 0);
	return 0;
}

LRESULT CMainFrame::OnMyKeyUp(WPARAM wParam, LPARAM lParam)
{
//	g_pBoard->m_keyboard.OnKeyUp(wParam, lParam);
	return 0;
}

LRESULT CMainFrame::OnMyKeyRepeat(WPARAM wParam, LPARAM lParam)
{
	g_pBoard->m_keyboard.OnKeyDown(wParam, 1);
	return 0;
}

void CMainFrame::OnToggleFullScreen() 
{
	// TODO: Add your command handler code here
	m_bFullScreen = !m_bFullScreen;
	if ( m_bFullScreen == TRUE )
	{
		// fullscreen으로 바뀌기 전의 창 위치를 기억한다.
		RECT rc;
		::GetWindowRect( m_hWnd, &rc );
		m_stWindowPos.x = rc.left;
		m_stWindowPos.y = rc.top;
	}
	m_wndView.SetFullScreenMode(m_bFullScreen);
	ResizeWindow();
}

void CMainFrame::ResizeWindow()
{
	// Resize Window with window style, apple display window

	RECT rc, rcWork;
	//::GetClientRect( m_hWnd, &rc );

	if ( ::IsIconic( m_hWnd ) != 0 )
	{
		this->ShowWindow( SW_SHOWNORMAL );
	}
	m_winRect.SetRectEmpty();

	if ( m_bFullScreen == TRUE )
	{
		this->m_wndStatusBar.ShowWindow( FALSE );
		this->m_wndToolBar.ShowWindow( FALSE );
		this->SetMenu(NULL); 
		ModifyStyle( WS_CAPTION, 0 );
		::SetRect( &rc, 0, 0, ::GetSystemMetrics(SM_CXSCREEN), ::GetSystemMetrics(SM_CYSCREEN) );
		::SetWindowPos( m_hWnd, HWND_NOTOPMOST, 0, 0, rc.right, rc.bottom,
			SWP_NOZORDER );
	}
	else
	{

		ModifyStyle( 0, WS_CAPTION );
		this->SetMenu( &this->m_cMenu );
		this->m_wndStatusBar.ShowWindow( TRUE );
		this->m_wndToolBar.ShowWindow( TRUE );
		if ( m_bDoubleSize == TRUE )
		{
			::SetRect( &rc, 0, 0, WIN_WIDTH * 2, WIN_HEIGHT * 2 );
		}
		else
		{
			::SetRect( &rc, 0, 0,  WIN_WIDTH, WIN_HEIGHT );
		}
		::AdjustWindowRectEx( &rc, GetWindowStyle(m_hWnd), ::GetMenu(m_hWnd) != NULL,
			GetWindowExStyle(m_hWnd) );
		
		::SetWindowPos( m_hWnd, NULL, 0, 0, rc.right-rc.left, rc.bottom-rc.top,
			SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE );
		
		::SetWindowPos( m_hWnd, HWND_NOTOPMOST, 0, 0, 0, 0,
			SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE );
		
		//  Make sure our window does not hang outside of the work area
		::SystemParametersInfo( SPI_GETWORKAREA, 0, &rcWork, 0 );

		::SetWindowPos( m_hWnd, NULL, m_stWindowPos.x, m_stWindowPos.y, 0, 0,
			SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE );
		::GetWindowRect( m_hWnd, &rc );
		
		if( rc.right > rcWork.right ) rc.left = rcWork.right - (rc.right-rc.left);
		if( rc.bottom > rcWork.bottom ) rc.top = rcWork.bottom - (rc.bottom-rc.top);
		if( rc.left < rcWork.left ) rc.left = rcWork.left;
		if( rc.top  < rcWork.top )  rc.top  = rcWork.top;
		::SetWindowPos( m_hWnd, NULL, rc.left, rc.top, 0, 0,
			SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE );
		
		m_stWindowPos.x = rc.left;
		m_stWindowPos.y = rc.top;

		// Resize window with tool bar, status bar
		CRect rcClientStart;
		CRect rcClientNow;
		
		GetClientRect(rcClientStart);
		
		RepositionBars(AFX_IDW_CONTROLBAR_FIRST,
			AFX_IDW_CONTROLBAR_LAST,
			0, reposQuery, rcClientNow);
		
		CPoint ptOffset(rcClientNow.left - rcClientStart.left,
			rcClientNow.top - rcClientStart.top);
		
		// pScreen 이 수정한 창 크기는 다른 child 창을 포함하지 않고 계산한 것이다.
		// 그러므로 child 창들의 크기를 더해준다. : status bar, tool bar
		CRect rcChild;
		CWnd* pwndChild = GetWindow(GW_CHILD);
		while (pwndChild)
		{
			pwndChild->GetWindowRect(rcChild);
			ScreenToClient(rcChild);
			rcChild.OffsetRect(ptOffset);
			pwndChild->MoveWindow(rcChild, FALSE);
			pwndChild = pwndChild->GetNextWindow();
		}
		
		CRect rcWindow;
		GetWindowRect(rcWindow);
		
		rcWindow.right += rcClientStart.Width() - rcClientNow.Width();
		rcWindow.bottom += rcClientStart.Height() - rcClientNow.Height();
		MoveWindow(rcWindow, FALSE);
		m_winRect.CopyRect( &rcWindow );
		
		if ( m_bDoubleSize == TRUE )
		{
			m_cMenu.CheckMenuItem( ID_2XSCREEN, MF_CHECKED );
		}
		else
		{
			m_cMenu.CheckMenuItem( ID_2XSCREEN, MF_UNCHECKED );
		}
	}

}

static CString GetStatusFilePath()
{
	char szPath[MAX_PATH];
	CString strPath;
	int nLen;
	GetModuleFileName( NULL, szPath, MAX_PATH );


	strPath = szPath;
	nLen = strPath.ReverseFind( '\\' );
	if ( nLen > 0 )
	{
		strPath = strPath.Left( nLen );
	}
	else
	{
		strPath = "";
	}
	strPath += "\\status.dat";

	return strPath;
}


void CMainFrame::OnKillFocus(CWnd* pNewWnd) 
{
	CFrameWnd::OnKillFocus(pNewWnd);
	
	// TODO: Add your message handler code here
	::PostMessage( m_hWnd, UM_REQACQUIRE, FALSE, (LPARAM)NULL );
}

void CMainFrame::OnUpdateDiskette(CCmdUI* pCmdUI) 
{
	// TODO: Add your command update UI handler code here
	if ( g_pBoard->m_cSlots.HasDiskInterface() )
		pCmdUI->Enable( TRUE );
	else
		pCmdUI->Enable( FALSE );
}

void CMainFrame::OnUpdateHarddisk(CCmdUI* pCmdUI) 
{
	// TODO: Add your command update UI handler code here
	if ( g_pBoard->m_cSlots.HasHardDiskInterface() )
		pCmdUI->Enable( TRUE );
	else
		pCmdUI->Enable( FALSE );
}

void CMainFrame::OnUpdateScanline(CCmdUI* pCmdUI) 
{
	// TODO: Add your command update UI handler code here
	if ( g_pBoard->m_pScreen->GetScanline() == TRUE )
	{
		pCmdUI->SetCheck( TRUE );
	}
	else
	{
		pCmdUI->SetCheck( FALSE );
	}
}

void CMainFrame::OnScanline() 
{
	// TODO: Add your command handler code here
	if ( g_pBoard->m_pScreen->GetScanline() == TRUE )
	{
		g_pBoard->m_pScreen->SetScanline( FALSE );
	}
	else
	{
		g_pBoard->m_pScreen->SetScanline( TRUE );
	}
		
}
