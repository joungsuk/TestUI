
// DisChgTester.cpp: 애플리케이션에 대한 클래스 동작을 정의합니다.
//

#include "pch.h"
#include "framework.h"
#include "DisChgTester.h"
#include "DisChgTesterDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CDisChgTesterApp

BEGIN_MESSAGE_MAP(CDisChgTesterApp, CWinApp)
	//ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// CDisChgTesterApp 생성

CDisChgTesterApp::CDisChgTesterApp()
{
	// TODO: 여기에 생성 코드를 추가합니다.
	// InitInstance에 모든 중요한 초기화 작업을 배치합니다.
}


// 유일한 CDisChgTesterApp 개체입니다.

CDisChgTesterApp theApp;


// CDisChgTesterApp 초기화

BOOL CDisChgTesterApp::InitInstance()
{
	CWinApp::InitInstance();

	CDisChgTesterDlg dlg;
	m_pMainWnd = &dlg;
	dlg.DoModal();
	return FALSE;
}

