// SetChDlg.cpp: 구현 파일
//

#include "pch.h"
#include "afxdialogex.h"
#include "SetChDlg.h"
#include "resource.h"
#include "framework.h"
// SetChDlg 대화 상자

IMPLEMENT_DYNAMIC(SetChDlg, CDialogEx)

SetChDlg::SetChDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_SET_CH, pParent)
{

}

SetChDlg::~SetChDlg()
{
}

void SetChDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);	
}


BEGIN_MESSAGE_MAP(SetChDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &SetChDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_CHK_ALLCH1, &SetChDlg::OnBnClickedChkAllch1)
	ON_BN_CLICKED(IDC_CHK_16CH, &SetChDlg::OnBnClickedChk16ch)
	ON_BN_CLICKED(IDC_CHK_16CH2, &SetChDlg::OnBnClickedChk16ch2)
END_MESSAGE_MAP()


// SetChDlg 메시지 처리기


BOOL SetChDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	
	m_checkList1.SubclassDlgItem(IDC_LIST1, this);
	m_checkList2.SubclassDlgItem(IDC_LIST2, this);
	for (int i = 0; i < 16; i++)
	{
		CString str;
		str.Format("ch %2d", i + 1);
		m_checkList1.InsertString(i, str);
		str.Format("ch %2d", (i + 1)+16);
		m_checkList2.InsertString(i, str);
		if ((setch1 & (1 << i)) != 0)	m_checkList1.SetCheck(i, TRUE);
		if ((setch2 & (1 << i)) != 0)	m_checkList2.SetCheck(i, TRUE);

	}
	activechCnt = 0;
	d_msg("setchDlg ch1 %x ch2%x\n", setch1, setch2);
	
	// 모든 채널 체크되어 있으면.
	if (setch1 == 0xFFFF && setch2 == 0xFFFF)   ((CButton*)GetDlgItem(IDC_CHK_ALLCH1))->SetCheck(TRUE);
	
	if (setch1 == 0xFFFF) {
		((CButton*)GetDlgItem(IDC_CHK_16CH))->SetCheck(TRUE);
		chkChLeft = TRUE;		
	}
	if (setch2 == 0xFFFF) {
		((CButton*)GetDlgItem(IDC_CHK_16CH2))->SetCheck(TRUE);
		chkChRight = TRUE; 
	}
	return TRUE;  // return TRUE unless you set the focus to a control
	// 예외: OCX 속성 페이지는 FALSE를 반환해야 합니다.
}


void SetChDlg::OnBnClickedOk()
{	
	chkChAll = ((CButton*)GetDlgItem(IDC_CHK_ALLCH1))->GetCheck();
	setch1 = setch2 = 0;
	for (int i = 0; i < 16; i++)
	{
		if (m_checkList1.GetCheck(i))
		{			
			setch1 |= 1 << i;
			activechCnt++;
		}
		if (m_checkList2.GetCheck(i))
		{			
			setch2 |= 1 << i;
			activechCnt++;
		}
	}	
	setCmd1 = GetDlgItemInt(IDC_EDIT_CMD1);	
	if (setCmd1 > 100) setCmd1 = 0;
	CDialogEx::OnOK();
}


void SetChDlg::OnBnClickedChkAllch1()
{
	chkChAll = ((CButton*)GetDlgItem(IDC_CHK_ALLCH1))->GetCheck();	
	for (int i = 0; i < 16; i++)
	{
		m_checkList1.SetCheck(i, chkChAll);
		m_checkList2.SetCheck(i, chkChAll);
	}
	if (chkChAll == TRUE) {
		((CButton*)GetDlgItem(IDC_CHK_16CH))->SetCheck(TRUE);
		((CButton*)GetDlgItem(IDC_CHK_16CH2))->SetCheck(TRUE);
	}
	else {
		((CButton*)GetDlgItem(IDC_CHK_16CH))->SetCheck(FALSE);
		((CButton*)GetDlgItem(IDC_CHK_16CH2))->SetCheck(FALSE);
	}
}


void SetChDlg::OnBnClickedChk16ch()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	chkChLeft = ((CButton*)GetDlgItem(IDC_CHK_16CH))->GetCheck();
	chkChRight = ((CButton*)GetDlgItem(IDC_CHK_16CH2))->GetCheck();
	for (int i = 0; i < 16; i++)	m_checkList1.SetCheck(i, chkChLeft);
	
	if(chkChLeft == TRUE && chkChRight == TRUE)	((CButton*)GetDlgItem(IDC_CHK_ALLCH1))->SetCheck(TRUE);
	else ((CButton*)GetDlgItem(IDC_CHK_ALLCH1))->SetCheck(FALSE);
}


void SetChDlg::OnBnClickedChk16ch2()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.	
	chkChLeft = ((CButton*)GetDlgItem(IDC_CHK_16CH))->GetCheck();
	chkChRight = ((CButton*)GetDlgItem(IDC_CHK_16CH2))->GetCheck();
	for (int i = 0; i < 16; i++)	m_checkList2.SetCheck(i, chkChRight);

	if (chkChLeft == TRUE && chkChRight == TRUE)	((CButton*)GetDlgItem(IDC_CHK_ALLCH1))->SetCheck(TRUE);
	else ((CButton*)GetDlgItem(IDC_CHK_ALLCH1))->SetCheck(FALSE);
}
