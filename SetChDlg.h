#pragma once
#include "afxdialogex.h"


// SetChDlg 대화 상자

class SetChDlg : public CDialogEx
{
	DECLARE_DYNAMIC(SetChDlg)

public:
	SetChDlg(CWnd* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~SetChDlg();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SET_CH };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()
public:
	CCheckListBox m_checkList1;
	CCheckListBox m_checkList2;
	uint32_t setch1;
	uint32_t setch2;
	int32_t setCmd1;
	uint32_t activechCnt;
	bool chkChAll;
	bool chkChLeft;
	bool chkChRight;
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedChkAllch1();
	afx_msg void OnBnClickedChk16ch();
	afx_msg void OnBnClickedChk16ch2();
};
