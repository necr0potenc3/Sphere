//
// CWindow = a base window class for controls.
// Copyright Menace Software (www.menasoft.com).
//

#ifndef _INC_CWINDOW_H
#define _INC_CWINDOW_H
#pragma once

#include "cstring.h"
#include <RICHEDIT.H>	// CRichEditCtrl

class CWindow    // similar to Std MFC class CWnd
{
public:
	HWND m_hWnd;
public:
	operator HWND () const       // cast as a HWND
	{
		return( m_hWnd );
	}
	HWND GetSafeHwnd() const
	{
		if ( this == NULL )
			return( NULL );
		return( m_hWnd );
	}
	CWindow()
	{
		m_hWnd = NULL;
	}
	~CWindow()
	{
		DestroyWindow();
	}

	// Standard message handlers.
	BOOL OnCreate( HWND hwnd, LPCREATESTRUCT lpCreateStruct = NULL  )
	{
		m_hWnd = hwnd;
		return( TRUE );
	}
	void OnDestroy()
	{
		m_hWnd = NULL;
	}
	void OnDestroy( HWND hwnd )
	{
		m_hWnd = NULL;
	}

	// Basic window functions.
	BOOL IsWindow() const
	{
		if ( this == NULL )
			return( false );
		if ( m_hWnd == NULL )
			return( false );
		return( ::IsWindow( m_hWnd ));
	}
	HWND GetParent() const
	{
		ASSERT( m_hWnd );
		return( ::GetParent(m_hWnd));
	}
	LRESULT SendMessage( UINT uMsg, WPARAM wParam = 0, LPARAM lParam = 0 ) const
	{
		ASSERT( m_hWnd );
		return( ::SendMessage( m_hWnd, uMsg, wParam, lParam ));
	}
	BOOL PostMessage( UINT uMsg, WPARAM wParam = 0, LPARAM lParam = 0 ) const
	{
		ASSERT( m_hWnd );
		return( ::PostMessage( m_hWnd, uMsg, wParam, lParam ));
	}
	int GetDlgCtrlID() const
	{
		ASSERT( m_hWnd );
		return( ::GetDlgCtrlID( m_hWnd ));
	}
	HWND GetDlgItem( int id ) const
	{
		ASSERT(m_hWnd);
		return( ::GetDlgItem( m_hWnd, id ));
	}
	LONG SendDlgItemMessage( int nIDDlgItem, UINT Msg, WPARAM wParam, LPARAM lParam ) const
	{
		ASSERT(m_hWnd);
		return( ::SendDlgItemMessage( m_hWnd, nIDDlgItem, Msg, wParam, lParam ));
	}
	UINT GetDlgItemText( int nIDDlgItem, LPSTR lpString, int nMaxCount ) const
	{
		ASSERT(m_hWnd);
		return( ::GetDlgItemText( m_hWnd, nIDDlgItem, lpString, nMaxCount ));
	}
	BOOL SetDlgItemText( int nIDDlgItem, LPCSTR lpString )
	{
		ASSERT(m_hWnd);
		return( ::SetDlgItemText( m_hWnd, nIDDlgItem, lpString ));
	}

	// Create/Destroy
	void DestroyWindow()
	{
		if ( m_hWnd == NULL )
			return;
		::DestroyWindow( m_hWnd );
		ASSERT( m_hWnd == NULL );
	}

	// Area and location
	void GetClientRect( LPRECT pRect ) const
	{
		ASSERT( m_hWnd );
		::GetClientRect( m_hWnd, pRect );
	}
	void GetWindowRect( LPRECT pRect ) const
	{
		ASSERT( m_hWnd );
		::GetWindowRect( m_hWnd, pRect );
	}
	BOOL MoveWindow( int X, int Y, int nWidth, int nHeight, BOOL bRepaint = TRUE )
	{
		return( ::MoveWindow( m_hWnd, X, Y, nWidth, nHeight, bRepaint ));
	}
	BOOL SetForegroundWindow()
	{
		ASSERT( m_hWnd );
		return( ::SetForegroundWindow( m_hWnd ));
	}
	HWND SetFocus()
	{
		ASSERT( m_hWnd );
		return( ::SetFocus( m_hWnd ));
	}
	void ClientToScreen( POINT * pPoint ) const
	{
		::ClientToScreen( m_hWnd, pPoint );
	}
	void ClientToScreen( RECT * pRect ) const
	{
		ClientToScreen( (POINT*)&(pRect->left));
		ClientToScreen( (POINT*)&(pRect->right));
	}
	void ScreenToClient( POINT * pPoint ) const
	{
		::ScreenToClient( m_hWnd, pPoint );
	}
	void ScreenToClient( RECT * pRect ) const
	{
		ScreenToClient( (POINT*)&(pRect->left));
		ScreenToClient( (POINT*)&(pRect->right));
	}
	BOOL IsWindowVisible() const
	{
		return( ::IsWindowVisible( m_hWnd ));
	}
	BOOL IsZoomed() const
	{
		return( ::IsZoomed( m_hWnd ));
	}
	BOOL IsIconic() const
	{
		return( ::IsIconic( m_hWnd ));
	}
	BOOL ShowWindow( int nCmdShow )
	{
		// SW_SHOW
		return( ::ShowWindow( m_hWnd, nCmdShow ));
	}

	// Drawing.
	HDC GetDC() const
	{
		return( ::GetDC( m_hWnd ));
	}
	void ReleaseDC( HDC hDC ) const
	{
		::ReleaseDC( m_hWnd, hDC );
	}
	void ValidateRect( RECT * pRect = NULL ) const
	{
		if ( m_hWnd == NULL )
			return;
		::ValidateRect( m_hWnd, pRect );
	}
	void InvalidateRect( RECT * pRect = NULL, BOOL fErase = FALSE ) const
	{
		if ( m_hWnd == NULL )
			return;
		::InvalidateRect( m_hWnd, pRect, fErase );
	}
	void InvalidateRect( int sx, int sy, int iWidth, int iHeight ) const
	{
		RECT rect;
		rect.left = sx;
		rect.top = sy;
		rect.right = sx + iWidth;
		rect.bottom = sy + iHeight;
		InvalidateRect( &rect );
	}

	// Standard windows props.
	int GetWindowText( LPSTR lpszText, int iLen )
	{
		ASSERT( m_hWnd );
		return ::GetWindowText( m_hWnd, lpszText, iLen );
	}
	BOOL SetWindowText( LPCSTR lpszText )
	{
		ASSERT( m_hWnd );
		return ::SetWindowText( m_hWnd, lpszText );
	}
	HMENU GetMenu() const
	{
		return( ::GetMenu( m_hWnd ));
	}
	HMENU GetSystemMenu( BOOL fReset ) const
	{
		return( ::GetSystemMenu( m_hWnd, fReset ));
	}

	HFONT GetFont() const
	{
		return( (HFONT) SendMessage( WM_GETFONT ));
	}

	void SetFont( HFONT hFont, BOOL fRedraw = false )
	{
		SendMessage( WM_SETFONT, (WPARAM) hFont, MAKELPARAM(fRedraw, 0));
	}
   

	HICON SetIcon( HICON hIcon, BOOL fType = false )
	{
		// ICON_BIG vs ICON_SMALL
		return( (HICON)(DWORD) SendMessage( WM_SETICON, (WPARAM)fType, (LPARAM) hIcon ));
	}

	UINT SetTimer( UINT uTimerID, UINT uWaitmSec )
	{
		ASSERT(m_hWnd);
		return( ::SetTimer( m_hWnd, uTimerID, uWaitmSec, NULL ));
	}
	BOOL KillTimer( UINT uTimerID )
	{
		ASSERT(m_hWnd);
		return( ::KillTimer( m_hWnd, uTimerID ));
	}
	int MessageBox( LPCSTR lpszText, LPCSTR lpszTitle, UINT fuStyle = MB_OK	) const
	{
		// ASSERT( m_hWnd ); ok for this to be NULL !
		return( ::MessageBox( m_hWnd, lpszText, lpszTitle, fuStyle ));
	}
	LONG SetWindowLong( int nIndex, LONG dwNewLong )
	{
		ASSERT(m_hWnd);
		return( ::SetWindowLong( m_hWnd, nIndex, dwNewLong ));
	}
	LONG GetWindowLong( int nIndex ) const
	{
		ASSERT(m_hWnd);
		return( ::GetWindowLong( m_hWnd, nIndex ));
	}

	DWORD GetStyle() const
	{
		// GetWindowStyle()
		return( (DWORD) GetWindowLong(GWL_STYLE));
	}
	DWORD GetExStyle() const
	{
		// GetWindowExStyle()
		return( (DWORD) GetWindowLong(GWL_EXSTYLE));
	}

#ifdef _WIN32
	int SetDlgItemText( int ID, LPCSTR lpszText ) const
	{
		return( ::SetDlgItemText( m_hWnd, ID, lpszText ));
	}
#else
	void SetDlgItemText( int ID, LPCSTR lpszText ) const
	{
		::SetDlgItemText( m_hWnd, ID, lpszText );
	}
#endif

	// Special methods.
	// Cursor selection stuff.
	void CenterWindow();
	void SetCursorStart() const;
	void SetCursorEnd() const;
	void SetSize( int ID, bool bInit );
	void SetEnableRange( int ID, int IDEnd, bool State );
};

class CDialogBase : public CWindow
{
public:
	static BOOL CALLBACK DialogProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );
public:
	virtual BOOL DefDialogProc( UINT message, WPARAM wParam, LPARAM lParam )
	{
		return FALSE;
	}
};

class CWindowBase : public CWindow
{
public:
	static LRESULT WINAPI WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );
public:
	static ATOM RegisterClass( WNDCLASS & wc );
	virtual LRESULT DefWindowProc( UINT message, WPARAM wParam, LPARAM lParam )
	{
		return ::DefWindowProc( m_hWnd, message, wParam, lParam );
	}
};

class CWinApp	// Similar to MFC type
{
public:
	LPCSTR	 	m_pszAppName;	// Specifies the name of the application. (display freindly)
	HINSTANCE 	m_hInstance;	// Identifies the current instance of the application.
	LPSTR 		m_lpCmdLine;	// Points to a null-terminated string that specifies the command line for the application.
	CWindow *	m_pMainWnd;		// Holds a pointer to the application's main window. For an example of how to initialize m_pMainWnd, see InitInstance.
	CGString	m_pszExeName;	// The module name of the application.
	CGString	m_pszHelpFilePath;	// The path to the application's Help file.
	CGString	m_pszProfileName;	// the path to the profile.
	int			m_nCmdShow;

public:
	CWinApp()
	{
		m_hInstance = NULL;
		m_pMainWnd = NULL;
		m_nCmdShow = 0;
	}

	void InitInstance( LPCTSTR pszAppName, HINSTANCE hInstance, LPSTR lpszCmdLine )
	{
		m_pszAppName = pszAppName;	// assume this is a static data pointer valid forever.
		m_hInstance	= hInstance;
		m_lpCmdLine	= lpszCmdLine;

		char szFileName[ _MAX_PATH ];
		if ( ! GetModuleFileName( m_hInstance, szFileName, sizeof( szFileName )))
			return;
		m_pszExeName = szFileName;

        LPSTR pszTmp = strrchr( m_pszExeName, '\\' );	// Get title
        lstrcpy( szFileName, ( pszTmp == NULL ) ? m_pszExeName : ( pszTmp + 1 ));
		pszTmp = strrchr( szFileName, '.' );	// Get extension.
		if ( pszTmp != NULL )
			pszTmp[0] = '\0';
		lstrcat( szFileName, ".INI" );

		OFSTRUCT ofs;
		if ( OpenFile( szFileName, &ofs, OF_EXIST ) != HFILE_ERROR)
		{
			m_pszProfileName = ofs.szPathName;
		}
		else
		{
			m_pszProfileName = szFileName;
		}
	}

	BOOL WriteProfileString( LPCSTR pszSection, LPCSTR pszEntry, LPCSTR pszVal )
	{
		return( ::WritePrivateProfileString( pszSection, pszEntry, pszVal, m_pszProfileName ));
	}
	BOOL WriteProfileInt( LPCSTR pszSection, LPCSTR pszEntry, int iVal )
	{
		char szValue[16];
		wsprintf( szValue, "%d", iVal );
		return( WriteProfileString( pszSection, pszEntry, szValue ));
	}
	int GetProfileString( LPCSTR pszSection, LPCSTR pszEntry, LPCSTR pszDefault, LPSTR pszReturnBuffer, int cbReturnBuffer )
	{
		return( ::GetPrivateProfileString( pszSection, pszEntry, pszDefault, pszReturnBuffer, cbReturnBuffer, m_pszProfileName ));
	}
	UINT GetProfileInt( LPCSTR pszSection, LPCSTR pszEntry, int iDefault )
	{
		return( ::GetPrivateProfileInt( pszSection, pszEntry, iDefault, m_pszProfileName ));
	}

	HICON LoadIcon( int id ) const
	{
		return( ::LoadIcon( m_hInstance, MAKEINTRESOURCE( id )));
	}
	HMENU LoadMenu( int id ) const
	{
		return( ::LoadMenu( m_hInstance, MAKEINTRESOURCE( id )));
	}
	FARPROC MakeProcInst( FARPROC lpProc ) const
	{
		return( MakeProcInstance( lpProc, m_hInstance ));
	}
};

class CScrollBar : public CWindow
{
// Constructors
public:
	CScrollBar::CScrollBar() 
	{
	}

// Attributes
	int CScrollBar::GetScrollPos() const
	{
		ASSERT(IsWindow());
		return ::GetScrollPos(m_hWnd, SB_CTL);
	}
	int CScrollBar::SetScrollPos(int nPos, BOOL bRedraw)
	{
		ASSERT(IsWindow());
		return ::SetScrollPos(m_hWnd, SB_CTL, nPos, bRedraw);
	}
	void CScrollBar::GetScrollRange(LPINT lpMinPos, LPINT lpMaxPos) const
	{
		ASSERT(IsWindow());
		::GetScrollRange(m_hWnd, SB_CTL, lpMinPos, lpMaxPos);
	}
	void CScrollBar::SetScrollRange(int nMinPos, int nMaxPos, BOOL bRedraw)
	{
		ASSERT(IsWindow());
		::SetScrollRange(m_hWnd, SB_CTL, nMinPos, nMaxPos, bRedraw);
	}
	void CScrollBar::ShowScrollBar(BOOL bShow)
	{
		ASSERT(IsWindow());
		::ShowScrollBar(m_hWnd, SB_CTL, bShow);
	}
	BOOL CScrollBar::EnableScrollBar(UINT nArrowFlags)
	{
		ASSERT(IsWindow());
		return ::EnableScrollBar(m_hWnd, SB_CTL, nArrowFlags);
	}

	BOOL CScrollBar::SetScrollInfo(LPSCROLLINFO lpScrollInfo, BOOL bRedraw)
	{
		lpScrollInfo->cbSize = sizeof(*lpScrollInfo);
		::SetScrollInfo(m_hWnd, SB_CTL, lpScrollInfo, bRedraw);
		return TRUE;
	}

	BOOL CScrollBar::GetScrollInfo(LPSCROLLINFO lpScrollInfo, UINT nMask)
	{
		lpScrollInfo->cbSize = sizeof(*lpScrollInfo);
		lpScrollInfo->fMask = nMask;
		return ::GetScrollInfo(m_hWnd, SB_CTL, lpScrollInfo);
	}

	int CScrollBar::GetScrollLimit()
	{
		int nMin, nMax;
		GetScrollRange(&nMin, &nMax);
		SCROLLINFO info;
		if (GetScrollInfo(&info, SIF_PAGE))
		{
			int iPage = info.nPage-1;
			if ( iPage > 0 )
				nMax -= iPage;
		}
		return nMax;
	}

// Implementation
public:
	virtual ~CScrollBar()
	{
	}
};

class CEdit : public CWindow
{
// Constructors
public:
	CEdit() {}

// Operations

	void SetSel( DWORD dwSelection, BOOL bNoScroll = FALSE )
	{
		ASSERT(IsWindow());
		SendMessage( EM_SETSEL, (WPARAM) dwSelection, (LPARAM) dwSelection );
	}
	void SetSel( int nStartChar, int nEndChar, BOOL bNoScroll = FALSE )
	{
		ASSERT(IsWindow());
		SendMessage( EM_SETSEL, (WPARAM) nStartChar, (LPARAM) nEndChar );
	}
	DWORD GetSel() const
	{
		ASSERT(IsWindow());
		return((DWORD) SendMessage( EM_GETSEL ));
	}
	void GetSel(int& nStartChar, int& nEndChar) const
	{
		ASSERT(IsWindow());
		DWORD dwSel = GetSel();
		nStartChar = LOWORD(dwSel);
		nEndChar = HIWORD(dwSel);
	}

	void ReplaceSel( LPCTSTR lpszNewText, BOOL bCanUndo = FALSE )
	{
		ASSERT(IsWindow());
		SendMessage( EM_REPLACESEL, (WPARAM) bCanUndo, (LPARAM) lpszNewText );
	}

	int GetLineCount() const
	{
		ASSERT(IsWindow());
		return (int) SendMessage( EM_GETLINECOUNT );
	}

	BOOL GetModify() const
	{
		return((BOOL)(DWORD)SendMessage(EM_GETMODIFY));
	}
	void SetModify( BOOL fModified )
	{
		SendMessage( EM_SETMODIFY, (WPARAM)(UINT)(fModified));
	}
	void ScrollCaret()
	{
		SendMessage(EM_SCROLLCARET);
	}
	void SetTabStops( int cTabs, const int * lpTabs ) 
	{
		SendMessage( EM_SETTABSTOPS, (WPARAM)(int)(cTabs), (LPARAM)(const int *)(lpTabs));
	}

	HLOCAL GetHandle() const
	{
		return((HLOCAL)(UINT)(DWORD)SendMessage( EM_GETHANDLE ));
	}
	void SetHandle( HLOCAL h)
	{
		SendMessage( EM_SETHANDLE, (WPARAM)(UINT)(HLOCAL)(h));
	}
	int GetFirstVisibleLine() const
	{
		return((int)(DWORD)SendMessage(EM_GETFIRSTVISIBLELINE));
	}

// Implementation
public:
	virtual ~CEdit()
	{
	}
};



class CRichEditCtrl : public CEdit
{
public:
	
	BOOL SetTextMode( UINT uMask )
	{
		// TEXTMODE
		return( (BOOL) SendMessage( EM_SETTEXTMODE, uMask ));
	}
	UINT GetTextMode()
	{
		// TEXTMODE
		return( (UINT) SendMessage( EM_GETTEXTMODE ));
	}

	COLORREF SetBackgroundColor( BOOL bSysColor, COLORREF cr )
	{ 
		return( (COLORREF)(DWORD) SendMessage( EM_SETBKGNDCOLOR, (WPARAM) bSysColor, (LPARAM) cr ));
	}

	void SetSel( int nStartChar, int nEndChar, BOOL bNoScroll = FALSE )
	{
		ASSERT(IsWindow());
		CHARRANGE range;
		range.cpMin = nStartChar;
		range.cpMax = nEndChar;
		SendMessage( EM_EXSETSEL, 0, (LPARAM) &range );
	}
	void GetSel(int& nStartChar, int& nEndChar) const
	{
		ASSERT(IsWindow());
		CHARRANGE range;
		SendMessage( EM_EXGETSEL, 0, (LPARAM) &range );
		nStartChar = range.cpMin;
		nEndChar = range.cpMax;
	}

	DWORD Scroll( int iAction = SB_PAGEDOWN )
	{
		return( (DWORD) SendMessage( EM_SCROLL, (WPARAM) iAction ));
	}
	void ScrollCaret()
	{
		// NOTE: This only works in rich text mode if the edit has focus.!?
		// EM_SCROLLCARET
		SendMessage((WM_USER + 49));
	}

	// Formatting.
	BOOL SetDefaultCharFormat( CHARFORMAT& cf )
	{
		return( (BOOL)(DWORD) SendMessage( EM_SETCHARFORMAT, (WPARAM) SCF_DEFAULT, (LPARAM) &cf ));
	}
	BOOL SetSelectionCharFormat( CHARFORMAT& cf )
	{
		return( (BOOL)(DWORD) SendMessage( EM_SETCHARFORMAT, (WPARAM) SCF_SELECTION, (LPARAM) &cf ));
	}

	// Events.
	long GetEventMask() const
	{
		return( (DWORD) SendMessage( EM_GETEVENTMASK ));
	}
	DWORD SetEventMask( DWORD dwEventMask = ENM_NONE )
	{
		// ENM_NONE = default.
		return( (DWORD) SendMessage( EM_SETEVENTMASK, 0, (LPARAM) dwEventMask ));
	}
};

class CListbox : public CWindow
{
// Constructors
public:
	CListbox() {}

// Operations

	void ResetContent()
	{
		ASSERT(IsWindow());
		SendMessage( LB_RESETCONTENT );
	}
	int GetCount() const
	{
		return( (int)(DWORD) SendMessage( LB_GETCOUNT ));
	}
	int AddString( LPCTSTR lpsz ) const
	{
		return( (int)(DWORD) SendMessage( LB_ADDSTRING, 0L, (LPARAM)(lpsz)));
	}
	int DeleteString( int index )
	{
		return( (int)(DWORD) SendMessage( LB_DELETESTRING, (WPARAM)(index), 0L ));
	}
	int InsertString( int index, LPCTSTR lpsz )
	{
		return((int)(DWORD)SendMessage( LB_INSERTSTRING, (WPARAM)(index), (LPARAM)(lpsz)));
	}
	DWORD GetItemData( int index ) const
	{
		return( (DWORD) SendMessage( LB_GETITEMDATA, (WPARAM)(index), 0L ));
	}
	int SetItemData( int index, DWORD data )
	{
		return( (int)(DWORD) SendMessage( LB_SETITEMDATA, (WPARAM)(int)(index), (LPARAM)(data)));
	}
	int GetTextLen( int index )          
	{
		return((int)(DWORD)SendMessage( LB_GETTEXTLEN, (WPARAM)(index) ));
	}
	int GetText( int index, LPCTSTR lpszBuffer ) 
	{
		return( (int)(DWORD)SendMessage( LB_GETTEXT, (WPARAM)(index), (LPARAM)(lpszBuffer)));
	}
	int AddItemData( DWORD data)
	{
		return((int)(DWORD)SendMessage( LB_ADDSTRING, 0L, (LPARAM)(data)));
	}
	int InsertItemData( int index, DWORD data ) 
	{
		return((int)(DWORD)SendMessage( LB_INSERTSTRING, (WPARAM)(index), (LPARAM)(data)));
	}
	void SetTabStops( int cTabs, const int * lpTabs ) 
	{
		SendMessage( LB_SETTABSTOPS, (WPARAM)(int)(cTabs), (LPARAM)(const int *)(lpTabs));
	}
	int SetCurSel( int index )
	{
		return((int)(DWORD)SendMessage( LB_SETCURSEL, (WPARAM) index ));
	}
	int GetCurSel() const
	{
		return( (int)(DWORD) SendMessage( LB_SETCURSEL ));
	}

// Implementation
public:
	virtual ~CListbox()
	{
	}
};

class CButton : public CWindow
{
// Constructors
public:
	CButton() {}

// Operations
	int GetCheck() const
	{
		return((int)(DWORD)SendMessage( BM_GETCHECK ));
	}
	void SetCheck( int check )
	{
		SendMessage( BM_SETCHECK, (WPARAM)(check));
	}

	int GetState() const
	{
		return((int)(DWORD)SendMessage( BM_GETSTATE ));
	}
	UINT SetState( int state )
	{
		return((UINT)(DWORD)SendMessage( BM_SETSTATE, (WPARAM)(state)));
	}

// Implementation
public:
	virtual ~CButton()
	{
	}
};

#endif	// _INC_CWINDOW_H