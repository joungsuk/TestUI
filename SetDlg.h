#pragma once


// SetDlg 대화 상자

class SetDlg : public CDialog
{
	DECLARE_DYNAMIC(SetDlg)

public:
	SetDlg(CWnd* pParent = nullptr);   // 표준 생성자입니다.
	virtual ~SetDlg();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SET_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
};
