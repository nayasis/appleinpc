// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__773A52F9_856B_48ED_8E9A_1F1616291758__INCLUDED_)
#define AFX_STDAFX_H__773A52F9_856B_48ED_8E9A_1F1616291758__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdisp.h>        // MFC Automation classes
#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#include "afxmt.h"

#include "arch/resource.h"
#include "aipcdefs.h"

typedef void	(*mem_write_handler) (void* objFrom, void* objTo, int nAddr, BYTE byData);
typedef BYTE	(*mem_read_handler) (void* objFrom, void* objTo, int nAddr);
typedef void	(*callback_handler) (void* objFrom, void* objTo, LPARAM lParam);

typedef struct _STWriteHandler
{
	void* objTo;
	mem_write_handler func;
} STWriteHandler;

typedef struct _STReadHandler
{
	void* objTo;
	mem_read_handler func;
} STReadHandler;

typedef struct _STCallbackHandler
{
	void* objTo;
	callback_handler func;
} STCallbackHandler;

#define WRITE_HANDLER(func)		void func( void* objFrom, void* objTo, int nAddr, BYTE byData )
#define READ_HANDLER(func)		void func( void* objFrom, void* objTo, int nAddr )
#define CALLBACK_HANDLER(func)	void func( void* objFrom, void* objTo, LPARAM lParam )

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__773A52F9_856B_48ED_8E9A_1F1616291758__INCLUDED_)
