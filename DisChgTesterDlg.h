
// DisChgTesterDlg.h: 헤더 파일
//

#pragma once
#include <thread>
#include "PCANBasicClass.h"
// CDisChgTesterDlg 대화 상자
class CDisChgTesterDlg : public CDialogEx
{
// 생성입니다.
public:
	CDisChgTesterDlg(CWnd* pParent = nullptr);	// 표준 생성자입니다.

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DISCHGTESTER_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.


// 구현입니다.
protected:
	HICON m_hIcon;

	// 생성된 메시지 맵 함수
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedBtnConnect();
	afx_msg void OnDropdownComboPort();

private:
	CComboBox m_ComboPort;
public:
	afx_msg void OnBnClickedBtnClose();
public:
	int SendCAN_Data(int len, char* data, long id);
//private:
	int SendControlCmd(long id, uint8_t cmd);
public:
	afx_msg void OnBnClickedBtnCh1Start();
	afx_msg void OnBnClickedBtnCh1Stop();
private:
	CComboBox m_ComboCh1Mode;
	CComboBox m_ComboCh1_ID;
public:
	afx_msg void OnBnClickedBtnT1();
	afx_msg void OnBnClickedBtnT2();
private:
	// 각 채널별 설정값 int 형 리턴
	int GetDevSetData(uint8_t ch, uint8_t type);	
public:
	afx_msg void OnBnClickedBtnT3();
public:
	afx_msg LRESULT OnUpdateRcvData(WPARAM wParam, LPARAM lParam);
public:
	afx_msg void OnBnClickedBtnT4();	
private:
	LRESULT OnSendData(WPARAM wP, LPARAM lP);
public:
	CListCtrl m_ListCh1;	
	afx_msg void OnBnClickedBtnListClear();
private:
	CEdit m_EditCsvName;
public:
	afx_msg void OnBnClickedBtnListSave();
	CEdit m_EditIfo;
	void SetInfoMsg(CString msg);
private:
	void SetDevSetData(uint8_t ch);
	void InitDevData();
public:
	afx_msg void OnCbnSelchangeComboCh1Op();
public:
	void SetWindowUI(int dev, int type);
public:
	afx_msg void OnBnClickedBtnInfoClear();	

public:	
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnDestroy();
	void SaveSetParamFile(int dev);
	void LoadSetParamFile(int dev);
	afx_msg void OnBnClickedBtnT5();
	afx_msg void OnBnClickedCheckSaveCsv();
private:
	CButton m_ChkboxSaveCsv;
public:
	void StopDev(int dev);
	CButton m_ChkAutoStop;
	afx_msg void OnBnClickedCheckAutoStop();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnCbnDropdownComboCh1Op();
	afx_msg void OnCbnDropdownComboCh2Op();
	afx_msg void OnBnClickedBtnFilleOpen();
	// FW업데이트 파일
	CString strUpdateFilePath;
	CString strUpdateFileName;
	afx_msg void OnBnClickedBtnUpdate();
	int GetDevID(int num);
protected:
	std::thread hThread;
public:
	afx_msg void OnCbnSelchangeComboCh2Id();
	// 펌웨어 업데이트 중일 경우 true
	bool m_isUpdateFW;
	void KillThred(int num);
	afx_msg void OnBnClickedBtnT9();
	afx_msg void OnCbnSelchangeCanbpsCb();
	void Wait(DWORD dwMillisecond);
	afx_msg void OnBnClickedBtnT10();
	afx_msg void OnBnClickedBtnT8();
	afx_msg void OnEnChangeEditCh1Volt();
	afx_msg void OnCbnSelchangeTypeCb();
	CComboBox m_ComboType;
	void LoadDevTypeData(int type);
	afx_msg void OnBnClickedBtnRest();
	afx_msg void OnBnClickedCheckAutoSend();
	afx_msg void OnBnClickedCheckAutoMsr();
	afx_msg void OnBnClickedBtnCur();		
	afx_msg void OnBnClickedBtnPatn();
	afx_msg void OnBnClickedBtnChgCur();
	// Can 명령과 데이터 전송용도
	int SendCAN_CmdData(long id, uint8_t cmd, char* data, uint8_t len);
	// 연속 패킷 테스트 시 사용
	bool m_isPacketTest;	
	afx_msg void OnBnClickedBtnSetParm();
	// 디버깅창에서 CAN Raw 데이터 표시 여부
	int m_DspCAN;
	afx_msg void OnBnClickedBtnSetProtec();	

	// Handles of non plug and play PCAN-Hardware
	//
	TPCANHandle m_NonPnPHandles[9];
	TPCANHandle m_PcanHandle;
	TPCANBaudrate m_Baudrate;
	TPCANType m_HwType;

	// Variables to store the current reading mode
	// 0 : Timer Mode
	// 1 : Event Mode
	// 2 : Manual Mode
	//
	int m_ActiveReadingMode = 0;

	//
	bool m_IsFD = false;
	PCANBasicClass* m_objPCANBasic = NULL;
	CString FormatChannelName(TPCANHandle handle);
	CString FormatChannelName(TPCANHandle handle, bool isFD);
	CString GetTPCANHandleName(TPCANHandle handle);
	DWORD HexTextToInt(CString ToConvert);
	void ProcessMessage(TPCANMsg theMsg, TPCANTimestamp itsTimeStamp);	
	int PCAN_Send(TPCANHandle Channel, long ID, int length, char data[64], int Ext, int RTR);
	// USB-CAN, PCAN, ...ETC
	CAN_Dev_Type_t m_CanDevType = NONE_CAN;
	afx_msg void OnBnClickedBtnPack1();	
	afx_msg void OnBnClickedCheckAutoReq();
	afx_msg void OnBnClickedBtnPack2();
	afx_msg void OnBnClickedBtnPack3();
	int SendCAN_DataEx(long id, int len, char* data, int ex);	
	afx_msg void OnBnClickedCheck1k2k();
	afx_msg void OnBnClickedCheckAutoStart();
	CButton m_ChkAutoStart;
	char* faultCodeMsg(int type, int code);
	CComboBox m_ComboCh1_Mode2;
	afx_msg void OnCbnSelchangeComboCh1Mode();
	afx_msg void OnBnClickedBtnSet();
};
