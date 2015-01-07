//
// CWindow.cpp
//

#ifdef _WIN32
#define STRICT
#include <windows.h>
#include <stdio.h>
#include "CFile.h"
#include "CWindow.h"

///////////////////////
// -CDialogBase

BOOL CALLBACK CDialogBase::DialogProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam ) // static
{
	CDialogBase * pDlg;
	if ( message == WM_INITDIALOG )	// NOTE: I get a useless WM_SETFONT msg first !
	{
		pDlg = (CDialogBase *)( lParam );
		ASSERT( pDlg );
		pDlg->m_hWnd = hWnd;	// OnCreate()
		pDlg->SetWindowLong( GWL_USERDATA, (DWORD) pDlg );
	}
	else
	{
		pDlg = (CDialogBase *)(LPVOID)::GetWindowLong( hWnd, GWL_USERDATA );
	}
	if ( pDlg )
	{
		return pDlg->DefDialogProc( message, wParam, lParam );
	}
	else
	{
		return FALSE;
	}
}

///////////////////////
// -CWindowBase

ATOM CWindowBase::RegisterClass( WNDCLASS & wc )	// static
{
	wc.lpfnWndProc = WndProc;
	return ::RegisterClass( &wc );
}

LRESULT WINAPI CWindowBase::WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam ) // static
{
	// NOTE: It is important to call OnDestroy() for asserts to work.
	CWindowBase * pWnd;
	if ( message == WM_NCCREATE || message == WM_CREATE )
	{
		LPCREATESTRUCT lpCreateStruct = (LPCREATESTRUCT)lParam;
		ASSERT(lpCreateStruct);
		CWindowBase *pWnd = (CWindowBase *)( lpCreateStruct->lpCreateParams );
		ASSERT( pWnd );
		pWnd->m_hWnd = hWnd;	// OnCreate()
		pWnd->SetWindowLong( GWL_USERDATA, (DWORD) pWnd );
	}
	pWnd = (CWindowBase *)(LPVOID)::GetWindowLong( hWnd, GWL_USERDATA );
	if ( pWnd )
	{
		return pWnd->DefWindowProc( message, wParam, lParam );
	}
	else
	{
		return ::DefWindowProc( hWnd, message, wParam, lParam );
	}
}

#endif // _WIN32
