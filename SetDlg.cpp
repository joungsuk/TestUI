// SetDlg.cpp: 구현 파일
//

#include "pch.h"
#include "SetDlg.h"
#include "afxdialogex.h"
#include "SetDlg.h"
#include "resource.h"

// SetDlg 대화 상자

IMPLEMENT_DYNAMIC(SetDlg, CDialog)

SetDlg::SetDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_SET_DIALOG, pParent)
{

}

SetDlg::~SetDlg()
{
}

void SetDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(SetDlg, CDialog)
	ON_BN_CLICKED(IDOK, &SetDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// SetDlg 메시지 처리기


void SetDlg::OnBnClickedOk()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	CDialog::OnOK();
}
