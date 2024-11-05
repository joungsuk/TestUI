// DisChgTesterDlg.cpp: 구현 파일
//

#include "pch.h"
#include "framework.h"
#include "DisChgTester.h"
#include "DisChgTesterDlg.h"
#include "afxdialogex.h"
#include "CAN_DLL/CAN_Access.h"
#include "Command.h"
#include "SetDlg.h"
#include "SetChDlg.h"
#include <process.h>    /* _beginthread, _endthread */
#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
using namespace std;

// CDisChgTesterDlg 대화 상자
static UINT ThreadRecvCAN_Data(LPVOID param);
static UINT ThreadSendCAN_Data(LPVOID param);
static UINT ThreadUpdateFW(LPVOID param);
static UINT ThreadUpdateFW_Stm32(LPVOID param);

void ThreadUpdateFW_Data(void* ptr);
static uint32_t crc32_fdl(uint32_t crc, const uint8_t* buf, size_t len);
CString GetTimeStamp(uint8_t type);
void CALLBACK TimeProc(UINT m_nTimerID, UINT uiMsg, DWORD dwUser, DWORD dw1, DWORD d2);
void CALLBACK TimeRcvProc(UINT m_nTimerID, UINT uiMsg, DWORD dwUser, DWORD dw1, DWORD d2);

// 전역 변수..
CAN_HANDLE hBus = -1;
bool openSuccess = false;
long CanBps = 0;
bool CanBpsSet = false;
DeviceSetParam dev1Set = { 0, };
ContactParam contactSet = { 0, };
Rcv_Measure_t rcvMeasure[20] = { 0, };
SCH_Protection_t protectSet = { 4500, 0, 210000, 0, 0, 800, 100, 0 };  // 최대최소전압, 최대최소전류, 최대용량 , 온도, 케이블 드랍 100mV
//SCH_Protection_t protectSet_md = { 4500, 0, 210000, 0, 0, 800, 100, 0 };  // 최대최소전압, 최대최소전류, 최대용량 , 온도, 케이블 드랍 100mV
SCH_Protection_t protectSet_DCIR = { 125000, 0, 410000, 0, 0, 800, 100, 0 };  // 최대최소전압, 최대최소전류, 최대용량 , 온도, 케이블 드랍

SCH_Protection2_t protectSet_160v32ch = { 165000, 24000, 61000, 0, 4351, 2001, 760, 0 };  // 최대최소전압, 최대최소전류, 최대용량 , 온도, 케이블 드랍 100mV


bool m_isRecvCANThread = false;
bool m_isSendCANThread = false;
bool m_isUpdateFW_DataThread = false;
CWinThread* m_pThreadSendCAN_Data;
CWinThread* m_pThreadRecvCAN_Data;

CWinThread* m_pThreadUpdateFW_Data;
static int sendRepCnt = 1;
bool adminMode = false;
CString debugMsg = "";

uint32_t chDelayTimer = CH_DELAY_TIMER; // 3600초
int CanRcvCnt = 0;
uint32_t send100msTimer = 0;
int fFwMaxCnt = 0;
int FwCnt = fFwMaxCnt;
static bool Setgpio = 0;
uint8_t getGpio2;
static uint8_t sendBtnCanTimer = 0;  // 버튼을 CAN 송신시 자동 전송 되지않게.
int mmTimerID = -1;
int mmRcvTimerID = -1;
uint32_t RcvTimeErrCnt = 0;
uint32_t getCanId = 0;
int rcvCAN_chkTimer = 40;
//int rcvCAN_sleepTimer = 50;
CString canMsgLog = "";		// can log 저장.
int saveCanMsgCnt = 0;		// 에러 발생이후 10초까지 저장.
int setRcvType = 0;			// setRcvType 이 1일 경우 타이머로 파싱 처리. 0은 ID 파싱.

uint32_t autoStartCnt = 0, restTime = 0, autoModeTog = 0, autoEndCh = 0;
CFont m_font;

CDisChgTesterDlg::CDisChgTesterDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DISCHGTESTER_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDI_ICON1);
}

void CDisChgTesterDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_PORT, m_ComboPort);
	DDX_Control(pDX, IDC_COMBO_CH1_OP, m_ComboCh1Mode);
	DDX_Control(pDX, IDC_COMBO_CH1_ID, m_ComboCh1_ID);
	DDX_Control(pDX, IDC_LIST_CH1, m_ListCh1);
	DDX_Control(pDX, IDC_EDIT_CSV_NAME, m_EditCsvName);
	DDX_Control(pDX, IDC_EDIT_INFO, m_EditIfo);
	DDX_Control(pDX, IDC_CHECK_SAVE_CSV, m_ChkboxSaveCsv);
	DDX_Control(pDX, IDC_CHECK_AUTO_STOP, m_ChkAutoStop);
	DDX_Control(pDX, IDC_TYPE_CB, m_ComboType);
	DDX_Control(pDX, IDC_CHECK_AUTO_START, m_ChkAutoStart);
	DDX_Control(pDX, IDC_COMBO_CH1_MODE, m_ComboCh1_Mode2);
}

BEGIN_MESSAGE_MAP(CDisChgTesterDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_CONNECT, &CDisChgTesterDlg::OnBnClickedBtnConnect)
	ON_CBN_DROPDOWN(IDC_COMBO_PORT, &CDisChgTesterDlg::OnDropdownComboPort)
	ON_BN_CLICKED(IDC_BTN_CLOSE, &CDisChgTesterDlg::OnBnClickedBtnClose)
	ON_BN_CLICKED(IDC_BTN_CH1_START, &CDisChgTesterDlg::OnBnClickedBtnCh1Start)
	ON_BN_CLICKED(IDC_BTN_CH1_STOP, &CDisChgTesterDlg::OnBnClickedBtnCh1Stop)
	ON_BN_CLICKED(IDC_BTN_T1, &CDisChgTesterDlg::OnBnClickedBtnT1)
	ON_BN_CLICKED(IDC_BTN_T2, &CDisChgTesterDlg::OnBnClickedBtnT2)
	ON_BN_CLICKED(IDC_BTN_T3, &CDisChgTesterDlg::OnBnClickedBtnT3)
	ON_MESSAGE(WM_USER_CAN_RCV_ALL_MSG, &CDisChgTesterDlg::OnUpdateRcvData)
	ON_MESSAGE(WM_USER_CAN_SEND_MSG, &CDisChgTesterDlg::OnSendData)
	ON_BN_CLICKED(IDC_BTN_T4, &CDisChgTesterDlg::OnBnClickedBtnT4)
	ON_BN_CLICKED(IDC_BTN_LIST_CLEAR, &CDisChgTesterDlg::OnBnClickedBtnListClear)
	ON_BN_CLICKED(IDC_BTN_LIST_SAVE, &CDisChgTesterDlg::OnBnClickedBtnListSave)
	ON_CBN_SELCHANGE(IDC_COMBO_CH1_OP, &CDisChgTesterDlg::OnCbnSelchangeComboCh1Op)
	ON_BN_CLICKED(IDC_BTN_INFO_CLEAR, &CDisChgTesterDlg::OnBnClickedBtnInfoClear)
	ON_WM_TIMER()
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BTN_T5, &CDisChgTesterDlg::OnBnClickedBtnT5)
	ON_BN_CLICKED(IDC_CHECK_SAVE_CSV, &CDisChgTesterDlg::OnBnClickedCheckSaveCsv)
	ON_BN_CLICKED(IDC_CHECK_AUTO_STOP, &CDisChgTesterDlg::OnBnClickedCheckAutoStop)
	ON_CBN_DROPDOWN(IDC_COMBO_CH1_OP, &CDisChgTesterDlg::OnCbnDropdownComboCh1Op)

	ON_BN_CLICKED(IDC_BTN_FILLE_OPEN, &CDisChgTesterDlg::OnBnClickedBtnFilleOpen)
	ON_BN_CLICKED(IDC_BTN_UPDATE, &CDisChgTesterDlg::OnBnClickedBtnUpdate)

	ON_BN_CLICKED(IDC_BTN_T9, &CDisChgTesterDlg::OnBnClickedBtnT9)
	ON_CBN_SELCHANGE(IDC_CANBPS_CB, &CDisChgTesterDlg::OnCbnSelchangeCanbpsCb)
	ON_BN_CLICKED(IDC_BTN_T10, &CDisChgTesterDlg::OnBnClickedBtnT10)
	ON_BN_CLICKED(IDC_BTN_T8, &CDisChgTesterDlg::OnBnClickedBtnT8)
	ON_EN_CHANGE(IDC_EDIT_CH1_VOLT, &CDisChgTesterDlg::OnEnChangeEditCh1Volt)
	ON_CBN_SELCHANGE(IDC_TYPE_CB, &CDisChgTesterDlg::OnCbnSelchangeTypeCb)
	ON_BN_CLICKED(IDC_BTN_REST, &CDisChgTesterDlg::OnBnClickedBtnRest)
	ON_BN_CLICKED(IDC_CHECK_AUTO_SEND, &CDisChgTesterDlg::OnBnClickedCheckAutoSend)
	ON_BN_CLICKED(IDC_CHECK_AUTO_MSR, &CDisChgTesterDlg::OnBnClickedCheckAutoMsr)
	ON_BN_CLICKED(IDC_BTN_CUR, &CDisChgTesterDlg::OnBnClickedBtnCur)
	ON_BN_CLICKED(IDC_BTN_PATN, &CDisChgTesterDlg::OnBnClickedBtnPatn)
	ON_BN_CLICKED(IDC_BTN_CHG_CUR, &CDisChgTesterDlg::OnBnClickedBtnChgCur)
	ON_BN_CLICKED(IDC_BTN_SET_PARM, &CDisChgTesterDlg::OnBnClickedBtnSetParm)
	ON_BN_CLICKED(IDC_BTN_SET_PROTEC, &CDisChgTesterDlg::OnBnClickedBtnSetProtec)
	ON_BN_CLICKED(IDC_BTN_PACK1, &CDisChgTesterDlg::OnBnClickedBtnPack1)
	ON_BN_CLICKED(IDC_CHECK_AUTO_REQ, &CDisChgTesterDlg::OnBnClickedCheckAutoReq)
	ON_BN_CLICKED(IDC_BTN_PACK2, &CDisChgTesterDlg::OnBnClickedBtnPack2)
	ON_BN_CLICKED(IDC_BTN_PACK3, &CDisChgTesterDlg::OnBnClickedBtnPack3)
	ON_BN_CLICKED(IDC_CHECK_1K_2K, &CDisChgTesterDlg::OnBnClickedCheck1k2k)
	ON_BN_CLICKED(IDC_CHECK_AUTO_START, &CDisChgTesterDlg::OnBnClickedCheckAutoStart)		
	ON_CBN_SELCHANGE(IDC_COMBO_CH1_MODE, &CDisChgTesterDlg::OnCbnSelchangeComboCh1Mode)
	ON_BN_CLICKED(IDC_BTN_SET, &CDisChgTesterDlg::OnBnClickedBtnSet)
END_MESSAGE_MAP()

// CDisChgTesterDlg 메시지 처리기

BOOL CDisChgTesterDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 이 대화 상자의 아이콘을 설정합니다.  응용 프로그램의 주 창이 대화 상자가 아닐 경우에는
	//  프레임워크가 이 작업을 자동으로 수행합니다.
	SetIcon(m_hIcon, TRUE);			// 큰 아이콘을 설정합니다.
	SetIcon(m_hIcon, FALSE);		// 작은 아이콘을 설정합니다.
	CString Ver;
	CString volt, cur, cap, prog, temp, wattH, Time, state, err, step, end;
	// TODO: 여기에 추가 초기화 작업을 추가합니다.
	//SetBackgroundColor(RGB(160, 200, 100));   // 녹색
	SetBackgroundColor(RGB(170, 204, 160));		// 약녹색.
	//SetBackgroundColor(RGB(226, 194, 185));  // 약 핑크
	d_msg("App start [%s %s]", __DATE__, __TIME__);
#ifdef _DEBUG
	prog.Format("[D] Tester %s [%s %s]\r\nMSG %d, %s", PROG_VER, __DATE__, __TIME__, DEBUG_MSG_01, PROG_INFO);
	//OnDropdownComboPort();  // 포트 찾고 강제로 2번째 선택
	//m_ComboPort.SetCurSel(1);
#else
	prog.Format("Tester %s [%s %s]\r\n%s", PROG_VER, __DATE__, __TIME__, PROG_INFO);
#endif
	SetInfoMsg(prog);
#ifdef CAN_PACKET_TEST
	SetInfoMsg("CAN_PACKET_TEST ver");
#endif
#ifdef DEBUG_MSG_01
	//SetInfoMsg("DEBUG_MSG_01");
	GetDlgItem(IDC_STATIC_CON)->SetWindowText("MSG 01");
#endif
	
	//m_font.CreateFont(10, 80, 0, 0, 1, 0,0,0,0, OUT_DEFAULT_PRECIS, 0, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "맑은 고딕");
	m_font.CreatePointFont(105, _T("맑은 고딕"));
	//m_font.CreateFont(
	//	10,                     // 글자높이
	//	8,                     // 글자너비
	//	0,                      // 출력각도
	//	0,                      // 기준 선에서의각도
	//	FW_NORMAL,              // 글자굵기
	//	FALSE,                  // Italic 적용여부
	//	FALSE,                  // 밑줄적용여부
	//	FALSE,                  // 취소선적용여부
	//	DEFAULT_CHARSET,       // 문자셋종류
	//	OUT_DEFAULT_PRECIS,    // 출력정밀도
	//	CLIP_DEFAULT_PRECIS,   // 클리핑정밀도
	//	DEFAULT_QUALITY,       // 출력문자품질
	//	DEFAULT_PITCH,         // 글꼴Pitch
	//	_T("맑은 고딕")           // 글꼴

	//);

	GetDlgItem(IDC_EDIT_INFO)->SetFont(&m_font);
	GetDlgItem(IDC_STATIC_PACK_ITEM)->SetFont(&m_font);
	GetDlgItem(IDC_STATIC_PACK_DATA)->SetFont(&m_font);
	GetDlgItem(IDC_STATIC_PACK_DATA2)->SetFont(&m_font);
	GetDlgItem(IDC_STATIC_PACK_DATA3)->SetFont(&m_font);

	InitDevData();
	OnDropdownComboPort();
	if (GetFindFile("set.ini")) {
		LoadSetParamFile(3);		// set.ini 파일 적용
		Wait(5);
	}
	Ver.Format("Cycler Tester %s ", PROG_VER);
	SetWindowText(Ver);

	SetDevSetData(DEV_1);
	CComboBox* pComboBox = (CComboBox*)GetDlgItem(IDC_COMBO_CH1_MODE);
	pComboBox->SetCurSel(0);
	//SetDevSetData(DEV_2);	
	m_ComboType.SetCurSel(dev1Set.type);
	LoadDevTypeData(dev1Set.type);
	m_EditIfo.SetLimitText(0); // 최대 길이로 적용.

	m_ListCh1.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_GRIDLINES);

	// 리스트 컬럼 추가.
	m_ListCh1.InsertColumn(0, _T("Cnt"), LVCFMT_LEFT, 60);
	int j = 1, x = 1;

	for (int i = 1; i <= MAX_CH; i++)
	{
		volt.Format("%s", "전압");
		cur.Format("%s", "전류");
		cap.Format("%s", "용량");
		wattH.Format("%s", "WattH");
		Time.Format("%s", "Time");
		end.Format("%s", "종료");
		err.Format("%s", "에러");		
		temp.Format("%s", "온도");
		state.Format("%s", "상태");
		step.Format("%s", "Step");
		if (dev1Set.type == _160V_60A_32CH_TYPE)	temp.Format("%s", "IR");
		m_ListCh1.InsertColumn(j++, step, LVCFMT_LEFT, 60);
		m_ListCh1.InsertColumn(j++, state, LVCFMT_LEFT, 60);
		m_ListCh1.InsertColumn(j++, volt, LVCFMT_LEFT, 100);
		m_ListCh1.InsertColumn(j++, cur, LVCFMT_LEFT, 100);
		m_ListCh1.InsertColumn(j++, cap, LVCFMT_LEFT, 100);
		m_ListCh1.InsertColumn(j++, wattH, LVCFMT_LEFT, 100);
		m_ListCh1.InsertColumn(j++, Time, LVCFMT_LEFT, 100);
		m_ListCh1.InsertColumn(j++, end, LVCFMT_LEFT, 90);
		m_ListCh1.InsertColumn(j++, err, LVCFMT_LEFT, 60);
		m_ListCh1.InsertColumn(j++, temp, LVCFMT_LEFT, 60);
	}
	/*temp.Format("온도");
	m_ListCh1.InsertColumn(x++, temp, LVCFMT_LEFT, 70);	*/
	//m_EditCsvName.SetWindowTextA("제어기1 저장파일명");
	//m_EditCsvName2.SetWindowTextA("제어기2 저장파일명");
	m_ChkboxSaveCsv.SetCheck(true);
	//m_ChkAutoStop.SetCheck(true);
	((CButton*)GetDlgItem(IDC_CHECK_VER_EX))->SetCheck(true);

	SetWindowUI(DEV_ALL, 1);

	if (!adminMode) {
		//GetDlgItem(IDC_BTN_T1)->ShowWindow(SW_HIDE);  // 관리자 모드 아닌 경우 버전확인 버튼 숨김
		GetDlgItem(IDC_BTN_FILLE_OPEN)->ShowWindow(SW_HIDE);  // 관리자 모드 아닌 경우 FW 업데이트 기능 버튼 숨김
		GetDlgItem(IDC_BTN_UPDATE)->ShowWindow(SW_HIDE);

	}
#ifndef _DEBUG  
	GetDlgItem(IDC_BTN_PATN)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_CHECK_AUTO_STOP)->ShowWindow(SW_HIDE);
	GetDlgItem(IDC_BTN_T8)->ShowWindow(SW_HIDE);
#endif

	// 테스트 타입 UI 표시 수정.
	if (dev1Set.type == _TEST_DUTY_TYPE)
	{
		SetWindowUI(DEV_ALL, 0xF0);
		CString strPwm;
		strPwm.Format("\t\t Duty [0~%d]", dev1Set.dutyRange);
		GetDlgItem(IDC_STATIC_CH3)->SetWindowText(strPwm);
		GetDlgItem(IDC_STATIC_CH4)->SetWindowText("시간Sec ");
		GetDlgItem(IDC_BTN_CUR)->SetWindowText("Duty 설정");
		GetDlgItem(IDC_BTN_T10)->SetWindowText("센싱 확인");
		GetDlgItem(IDC_BTN_CHG_CUR)->SetWindowText("LD-SW ");
		GetDlgItem(IDC_BTN_T3)->SetWindowText("STA ");
		GetDlgItem(IDC_BTN_REST)->SetWindowText("FAN ");
		SetBackgroundColor(RGB(226, 194, 185));  // 약 핑크		
		

		m_ComboCh1_Mode2.ResetContent();
		char* idList[6] = { "DCIR", "160V60A", "Spare", "Spare", "Spare", "Spare" };
		for (int i = 0; i < 6; i++)		m_ComboCh1_Mode2.AddString(idList[i]);
		//m_ComboCh1_Mode2.SetCurSel(0);
		m_ComboCh1_Mode2.SetCurSel(dev1Set.testModeType);
		OnCbnSelchangeComboCh1Mode();
		//((CButton*)GetDlgItem(IDC_CHECK_GROUP))->SetCheck(0);
		((CButton*)GetDlgItem(IDC_CHECK_VER_EX))->SetCheck(false);
		
	}
	else if (dev1Set.type == _100V_500A_TYPE || dev1Set.type == _5V_200A_TYPE)
	{

		//GetDlgItem(IDC_BTN_CUR)->SetWindowText("패턴 시작");
		//GetDlgItem(IDC_BTN_CUR)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_BTN_CUR)->SetWindowText("센싱 확인");
		GetDlgItem(IDC_BTN_CHG_CUR)->SetWindowText("패턴 갱신");
		GetDlgItem(IDC_CHECK_AUTO_RELAY)->ShowWindow(SW_HIDE);
	}

	if (dev1Set.type == _5V_20A_PACK_TYPE)
	{
		//GetDlgItem(IDC_BTN_SET_PROTEC)->SetWindowText("Pack Data");
	}
	else
	{
		//GetDlgItem(IDC_CHECK_GROUP)->ShowWindow(SW_HIDE);	
		// 배터리 팩 설정 UI 비활성화.
		GetDlgItem(IDC_BTN_PACK1)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_BTN_PACK2)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_BTN_PACK3)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_EDIT_PACK1)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_EDIT_PACK2)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_EDIT_PACK3)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_EDIT_PACK4)->ShowWindow(SW_HIDE);
	}

	if (dev1Set.type == _74V_80A_TYPE)
	{
		//GetDlgItem(IDC_BTN_CUR)->ShowWindow(SW_HIDE);
		//GetDlgItem(IDC_BTN_CHG_CUR)->SetWindowText("DAQ-Rly");
		//GetDlgItem(IDC_BTN_CHG_CUR)->ShowWindow(SW_HIDE);		
		GetDlgItem(IDC_CHECK_GROUP)->ShowWindow(SW_HIDE);
		dev1Set.parallelEn = 0;
	}
	if (dev1Set.type == _160V_60A_32CH_TYPE)
	{	
		Ver.Format("160V60A 32ch Tester %s ", PROG_VER);
		SetWindowText(Ver);
		SetBackgroundColor(RGB(119, 180, 194));  // 약 청색
		//GetDlgItem(IDC_CHECK_AUTO_REQ)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_CHECK_AUTO_REQ)->SetWindowText("BMS 저장");
		GetDlgItem(IDC_BTN_SET_PARM)->ShowWindow(SW_HIDE);		
		CString setTxt = "동작\t   모드\t       초충     측정시간ms";
		GetDlgItem(IDC_EDIT_CH1_STEP)->EnableWindow(TRUE);
		GetDlgItem(IDC_STATIC_CH1)->SetWindowText(setTxt);
		setTxt = "전압V\t     전류A       동작Cell";
		GetDlgItem(IDC_STATIC_CH3)->SetWindowText(setTxt);
		GetDlgItem(IDC_EDIT_CH1_POWER)->ModifyStyle(ES_NUMBER, FALSE);  // 숫자 입력 끄기.		
		//GetDlgItem(IDC_EDIT_CH1_STEP)->EnableWindow(FALSE);
		GetDlgItem(IDC_EDIT_CH1_POWER)->EnableWindow(FALSE);
		GetDlgItem(IDC_BTN_SET)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_BTN_PATN)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_BTN_PATN)->SetWindowTextA("Set");
		//GetDlgItem(IDC_BTN_SET)->EnableWindow(TRUE);		
		GetDlgItem(IDC_BTN_T8)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_BTN_T8)->SetWindowText("TEST_BD");
		dev1Set.activeCh = 0xFFFFFFFF;              // 32채널 활성화 상태
		dev1Set.activeChCnt = MAX_BMS_CH;           // 32채널 갯수.
		CString strCh;
		strCh.Format("%X", dev1Set.activeCh);
		GetDlgItem(IDC_EDIT_CH1_POWER)->SetWindowText(strCh);		
		((CButton*)GetDlgItem(IDC_CHECK_VER_EX))->SetCheck(false);
	}

	SetTimer(TIMER_1S_ID, 1000, NULL);
	SetTimer(TIMER_500ms_ID, 500, NULL);
	//SetTimer(TIMER_100ms_ID, 100, NULL);	

#ifdef _SYS_LOG
	debugMsg.Format("%s start prog ver %s\n", GetTimeStamp(1), _T(PROG_VER));
#endif

#ifdef CYCLE_MODE
	adminMode = TRUE;
	dev1Set.cycleMode = TRUE;
#endif


	m_isUpdateFW = false;
	dev1Set.rcvChkTimer = 5;
	GetDevSetData(DEV_1, 0);
	//OnCbnSelchangeCanbpsCb();	
	dev1Set.updateUICnt = UPDATE_UI_CNT;

#ifndef SELECT_VOLT_CMD
	GetDlgItem(IDC_BTN_T8)->ShowWindow(SW_HIDE);  // 전압 선택 버튼 숨김.
#endif


	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

// 대화 상자에 최소화 단추를 추가할 경우 아이콘을 그리려면
//  아래 코드가 필요합니다.  문서/뷰 모델을 사용하는 MFC 애플리케이션의 경우에는
//  프레임워크에서 이 작업을 자동으로 수행합니다.

void CDisChgTesterDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 그리기를 위한 디바이스 컨텍스트입니다.

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 클라이언트 사각형에서 아이콘을 가운데에 맞춥니다.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 아이콘을 그립니다.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// 사용자가 최소화된 창을 끄는 동안에 커서가 표시되도록 시스템에서
//  이 함수를 호출합니다.
HCURSOR CDisChgTesterDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CDisChgTesterDlg::OnBnClickedBtnConnect()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	openSuccess = false;
	CString tempport, port, strTemp;
	int pcanHandleTemp;
	TPCANStatus stsResult;
	int selectedIO = 0x100;
	int selectedInterrupt = 3;
	CString txtBitrate, strErr;
	CString info;
	m_ComboPort.GetWindowText(tempport);
	strTemp = tempport;
	int chkPort = strTemp.Find("PCAN");
	//d_msg("chkPort %d\n", chkPort);

	if (chkPort == -1) 	m_CanDevType = MW_USB2CAN;
	else m_CanDevType = USB_PCAN;

	if (m_CanDevType == MW_USB2CAN)
	{
		if (0 <= hBus) {
			CAN_SetTransferMode(hBus, 0);
			CAN_Close(hBus);
			hBus = -1;
		}


		port.Format("\\\\.\\%s", _TEXT(tempport));
		hBus = CAN_OpenVcp(port, 921600);
		//hBus = CAN_OpenFifo("NT5TEUUL");
		d_msg("open CAN[%d] %s\n", hBus, _TEXT(tempport));
	}
	else if (m_CanDevType == USB_PCAN)
	{
		strTemp = tempport;
		strTemp = strTemp.Mid(strTemp.Find('(') + 1, 3);

		strTemp.Replace('h', ' ');
		strTemp.Trim(' ');
		//SetInfoMsg(tempport);
		if (CanBps == 1000)			m_Baudrate = PCAN_BAUD_1M;
		else if (CanBps == 500)		m_Baudrate = PCAN_BAUD_500K;
		else if (CanBps == 250)		m_Baudrate = PCAN_BAUD_250K;
		else {
			SetInfoMsg("CAN Bps NG..");
			return;
		}

		m_HwType = PCAN_TYPE_ISA;
		//SetInfoMsg(strTemp);
		pcanHandleTemp = HexTextToInt(strTemp);
		m_PcanHandle = pcanHandleTemp;
		//bNonPnP = m_PcanHandle <= PCAN_DNGBUS1;
		//m_PcanHandle = 0x51;
		d_msg("m_IsFD %d m_Baudrate %d selectedIO %x  selectedInterrupt %x \n", m_IsFD, m_Baudrate, selectedIO, selectedInterrupt);
		// Connects a selected PCAN-Basic channel
		//
		if (m_IsFD)
			stsResult = m_objPCANBasic->InitializeFD(m_PcanHandle, txtBitrate.GetBuffer());
		else
			stsResult = m_objPCANBasic->Initialize(m_PcanHandle, m_Baudrate, m_HwType, selectedIO, selectedInterrupt);

		strErr.Format("Err %x", stsResult);
		if (stsResult != PCAN_ERROR_OK)
			if (stsResult != PCAN_ERROR_CAUTION)
				//::MessageBox(NULL, GetFormatedError(stsResult), "Error!", MB_ICONERROR);
				SetInfoMsg(strErr);
			else
			{
				SetInfoMsg("******************************************************");
				SetInfoMsg("The bitrate being used is different than the given one");
				SetInfoMsg("******************************************************");
				stsResult = PCAN_ERROR_OK;
			}
		else
		{

			hBus = 1;
			info.Format("%s 장치 정상 연결 됨 [%d]Kbps", tempport, CanBps);
		}
	}
	else {
		SetInfoMsg("CAN DEV NONE..");
	}

	if (hBus == -1)
	{
		AfxMessageBox("장치 설정을 확인!", MB_ICONSTOP);
	}
	else // open
	{
		GetDlgItem(IDC_BTN_CONNECT)->EnableWindow(FALSE);
		m_ComboPort.EnableWindow(FALSE);
		GetDlgItem(IDC_BTN_CLOSE)->EnableWindow(TRUE);
		GetDlgItem(IDC_CANBPS_CB)->EnableWindow(FALSE);
		GetDlgItem(IDC_TYPE_CB)->EnableWindow(FALSE);
		GetDlgItem(IDC_CHECK_AUTO_START)->EnableWindow(TRUE);
		openSuccess = true;

		int startup_transfer_mode = 1;    // Binary 전송
		int busoff_recovery = 1;        // Automatic
		long bitrate = 0;

		if (m_CanDevType == MW_USB2CAN)
		{
			if (CanBpsSet)
			{
				//bitrate = CanBps;
				{
					if (CAN_SetConfig(hBus, CanBps, 0, 0)) {}
					else SetInfoMsg("CAN 장치 설정 에러!");
				}
				CanBpsSet = false;
			}
			//CAN_SetTimeout(hBus, 33, 100, 1);
			//CAN_SetTimeout(hBus, 20, 1, 1);
			CAN_SetTimeout(hBus, 48, 2, 50);  // 50ms 
			CAN_GetConfig(hBus, &bitrate, 0, 0);
			CAN_SetTransferMode(hBus, 1);	// Binary packet 전송 모드로 설정

			CanBps = bitrate;
			info.Format("MW-USB2CAN 장치 정상 연결 됨 [%d]Kbps", CanBps);
		}
		SetInfoMsg(info);
		//SetInfoMsg("======== 시험 시작 전에 OCV와 접촉검사 진행 필요! ==========");
		SetWindowUI(DEV_ALL, 0);
		GetDlgItem(IDC_LIST_CH1)->SetFocus();		// 단축키 적용하기위해 마우스 포커스 지정..
		if (mmTimerID == -1)	mmTimerID = timeSetEvent(100, 0, TimeProc, (DWORD)this, TIME_PERIODIC);
		//m_isUpdateFW = false;		
#ifdef USE_RCV_THREAD
		m_isRecvCANThread = true;
		usleep(1000);
		m_pThreadRecvCAN_Data = AfxBeginThread(ThreadRecvCAN_Data, this);
		if (m_pThreadRecvCAN_Data != NULL) {

			d_msg("CAN Recv Thread 시작\n");
		}
		else m_isRecvCANThread = false;
#else
		if (mmRcvTimerID == -1)	mmRcvTimerID = timeSetEvent(10, 0, TimeRcvProc, (DWORD)this, TIME_PERIODIC);
#endif
		//m_pThreadSendCAN_Data = 
		m_isSendCANThread = true;
		usleep(1000);
		m_pThreadSendCAN_Data = AfxBeginThread(ThreadSendCAN_Data, this);

		if (m_pThreadSendCAN_Data != NULL) {

			d_msg("CAN Send Thread 시작\n");
		}
		else m_isSendCANThread = false;
	}
	d_msg("CanBps %d\n", CanBps);
}

void CDisChgTesterDlg::OnDropdownComboPort()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	// 자동 컴포트 추가.
	HKEY key;
	TCHAR szData[20], szName[100];
	DWORD dwIndex = 0, ze = 100, dwSize2 = 20, dwType = REG_SZ;
	INT nPortArray[50] = { 0, };
	int nIndex = 0, nCnt = 3;
	m_ComboPort.ResetContent();

	memset(szData, 0x00, 20);
	memset(szName, 0x00, 100);

	DWORD dwSize;

	::RegOpenKey(HKEY_LOCAL_MACHINE, _T("HARDWARE\\DEVICEMAP\\SERIALCOMM"), &key);
	while (ERROR_SUCCESS == ::RegEnumValue(key, dwIndex, szName, &dwSize, NULL, NULL, NULL, NULL))
	{
		dwIndex++;
		::RegQueryValueEx(key, szName, NULL, &dwType, (LPBYTE)szData, &dwSize2);
		m_ComboPort.AddString("COM" + CString(szData).Mid(3));

		memset(szData, 0x00, sizeof(szData));
		memset(szName, 0x00, sizeof(szName));
		dwSize = 100;
		dwSize2 = 20;
	}
	::RegCloseKey(key);

	TPCANChannelInformation* info;
	TPCANStatus stsResult;
	DWORD iChannelsCount;
	bool bIsFD;

	// Clears the Channel combioBox and fill it againa with
	// the PCAN-Basic handles for no-Plug&Play hardware and
	// the detected Plug&Play hardware
	//
	//cbbChannel.ResetContent();
	CString strInfo = "";
	// Includes all no-Plug&Play Handles
	//for (int i = 0; i < (sizeof(m_NonPnPHandles) / sizeof(TPCANHandle)); i++)
		//cbbChannel.AddString(FormatChannelName(m_NonPnPHandles[i]));
		//strInfo.Append(FormatChannelName(m_NonPnPHandles[i]));
	CComboBox* pComboBox = (CComboBox*)GetDlgItem(IDC_CANBPS_CB);
	stsResult = m_objPCANBasic->GetValue(PCAN_NONEBUS, PCAN_ATTACHED_CHANNELS_COUNT, (void*)&iChannelsCount, sizeof(iChannelsCount));
	if (stsResult == PCAN_ERROR_OK)
	{
		info = new TPCANChannelInformation[iChannelsCount];
		stsResult = m_objPCANBasic->GetValue(PCAN_NONEBUS, PCAN_ATTACHED_CHANNELS, (void*)info, iChannelsCount * sizeof(TPCANChannelInformation));
		if (stsResult == PCAN_ERROR_OK)
			// Include only connectable channels
			//
			for (int i = 0; i < (int)iChannelsCount; i++)
				if (info[i].channel_condition & PCAN_CHANNEL_AVAILABLE)
				{
					bIsFD = info[i].device_features & FEATURE_FD_CAPABLE;
					m_ComboPort.AddString(FormatChannelName(info[i].channel_handle, bIsFD));
					pComboBox->SetCurSel(0);
					OnCbnSelchangeCanbpsCb();
				}
		delete[] info;
		//d_msg("m_objPCANBasic %x %s\n", m_objPCANBasic, tempport);		
	}
}

void CDisChgTesterDlg::OnBnClickedBtnClose()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	if (dev1Set.start) {  // 해제시 1개라도 시험중이면 전체 중지 명령.
		//Wait(10);
		OnBnClickedBtnCh1Stop();
		Sleep(100);
	}
	GetDlgItem(IDC_BTN_CONNECT)->EnableWindow(TRUE);
	m_ComboPort.EnableWindow(TRUE);
	GetDlgItem(IDC_BTN_CLOSE)->EnableWindow(FALSE);
	GetDlgItem(IDC_CANBPS_CB)->EnableWindow(TRUE);
	GetDlgItem(IDC_CHECK_AUTO_START)->EnableWindow(FALSE);
	if (dev1Set.type != _TEST_DUTY_TYPE)	GetDlgItem(IDC_TYPE_CB)->EnableWindow(TRUE);

	SetWindowUI(DEV_ALL, 1);
	if (mmTimerID != -1) {
		timeKillEvent(mmTimerID);
		mmTimerID = -1;
	}
	if (mmRcvTimerID != -1) {
		timeKillEvent(mmRcvTimerID);
		mmRcvTimerID = -1;
	}
	m_isRecvCANThread = false;
	m_isSendCANThread = false;

	m_pThreadRecvCAN_Data = NULL;
	m_pThreadSendCAN_Data = NULL;
	Sleep(100);
	//TerminateThread(m_pThreadRecvCAN_Data->m_hThread, NULL);
	//WaitForSingleObject(m_pThreadRecvCAN_Data->m_hThread, INFINITE);	
	//m_pThreadRecvCAN_Data = NULL;
	//if (m_pThreadRecvCAN_Data->m_hThread != NULL) {
	//	CloseHandle(m_pThreadRecvCAN_Data->m_hThread);
	//	m_pThreadRecvCAN_Data = NULL;
	//}


	/*WaitForSingleObject(m_pThreadSendCAN_Data->m_hThread, INFINITE);

	if (m_pThreadSendCAN_Data->m_hThread != NULL) {
		CloseHandle(m_pThreadSendCAN_Data->m_hThread);
		m_pThreadSendCAN_Data = NULL;
	}*/
	//TerminateThread(m_pThreadSendCAN_Data->m_hThread, NULL);

	//m_isUpdateFW = true;
	dev1Set.rcvChkTimer = 5;
	RcvTimeErrCnt = 0;
	Sleep(100);
	openSuccess = false;
	if (m_CanDevType == MW_USB2CAN)
	{
		if (0 <= hBus) {
			CAN_SetTransferMode(hBus, 0);	// Packet 전송 금지
			CAN_Close(hBus);
			hBus = -1;
		}
	}
	else if (m_CanDevType == USB_PCAN)
	{
		m_objPCANBasic->Uninitialize(m_PcanHandle);
	}

	SetInfoMsg("CAN 장치 해제 됨");
	d_msg("Can Close [%d]\n", hBus);

	CString getStr = "";

	m_EditIfo.GetWindowTextA(getStr);

	if (getStr.IsEmpty() || getStr.GetLength() < 500) return;

	FILE* pFile = NULL;
	CString saveFile;
	if (GetFileAttributes(LOG_DIR) == -1) CreateDirectory(LOG_DIR, NULL);	// 디렉토리 생성.

	saveFile.Format("%s\%s%s_Info.txt", GetExePath(), LOG_DIR, GetTimeStamp(1));
	if (0 == fopen_s(&pFile, saveFile, "w+")) {
		//fprintf(pFile, saveFile);
		fprintf(pFile, getStr);
		fclose(pFile);
		//OnBnClickedBtnInfoClear();

		saveFile.Append(" 저장완료");
		SetInfoMsg(saveFile);
		//AfxMessageBox(_T(saveFile), MB_ICONINFORMATION);
	}
	else {
		SetInfoMsg(" 저장 실패!!");
	}
}

int CDisChgTesterDlg::SendCAN_Data(int len, char* data, long id)
{
	// TODO: 여기에 구현 코드 추가.	
	int ret = -1;
	if (openSuccess) {
		ret = PCAN_Send(m_PcanHandle, id, len, data, 1, 0);
		ret = CAN_Send(hBus, id, len, data, 1, 0);
	}
	return ret;
}

int CDisChgTesterDlg::SendControlCmd(long id, uint8_t cmd)
{
	// TODO: 여기에 구현 코드 추가.
	if (openSuccess == false) return -1;
	int ret = -1;
	int len = 0;
	static struct Exid canid = { 0, };
	char txData[64] = { 0x00, };  // 8byte * 3
	if (dev1Set.parallelEn)   id = id & 0xF0;
	canid.id = id;
	canid.cmd = cmd;
	int32_t cur = 0, volt = 0, power = 0, endCur = 0;
	uint32_t endTime = 0, endCap = 0, endWatth = 0, endPower = 0;
	static int TogVer = 1;
	if (cmd == CAN_READ_DATA_CMD) {}	//전압전류 값 요청..추가설정필요 없음.
	else if (cmd == CAN_CH_STOP_CMD)
	{
		//len = 1;
		//if (dev1Set.start == false && canid.id == dev1Set.id)	txData[0] = 0x0F;		//  채널별 중지.. 4ch => 0x0f
		//if (DEBUG_MASTER) {  // 마스터와 같이 stop 신호 주기적으로 전송
		//	if (dev1Set.start == true) txData[0] = ~(dev1Set.ch) & 0x0F;
		//}
	}
	else if (cmd == CAN_START_CHG_CMD || cmd == CAN_START_DISCHG_CMD)   // 충전/방전 공통 변수가 많아 같이 처리후 송신함.
	{
		len = 40;
		volt = dev1Set.volt;
		cur = dev1Set.cur;
		endCur = dev1Set.endCur;
		power = dev1Set.power;
		if (cmd == CAN_START_DISCHG_CMD)  // 방전일 경우 처리..
		{
			volt = dev1Set.dis_volt;
			cur = dev1Set.dis_cur;
			endCur = dev1Set.dis_endCur;
			power *= -1;
		}
		if (dev1Set.type == _5V_6A_TYPE) {  // 6A는 전류 0.1mA 단위로 된것 같음..
			cur *= 10;			endCur *= 10;
		}
		txData[0] = dev1Set.opModeCmd;
		txData[1] = dev1Set.stepNum;
		txData[2] = dev1Set.parallelEn;
		if (dev1Set.type == _160V_60A_32CH_TYPE) {  // 초기 전압 전송.
			if (dev1Set.startVolt != 0) {
				txData[3] = 0xE0;
				memcpy(&txData[4], &dev1Set.startVolt, sizeof(uint16_t));
			}
			power = dev1Set.activeCh;  // 파워 위치에 활성화 채널상태 전달됨.
		}
		memcpy(&txData[6], &dev1Set.msrTime, sizeof(uint16_t));
		memcpy(&txData[8], &volt, sizeof(int32_t));
		memcpy(&txData[12], &cur, sizeof(int32_t));
		memcpy(&txData[16], &power, sizeof(int32_t));
		memcpy(&txData[20], &dev1Set.endTime, sizeof(uint32_t));
		memcpy(&txData[24], &endCur, sizeof(uint32_t));
		memcpy(&txData[28], &dev1Set.endCap, sizeof(uint32_t));
		memcpy(&txData[32], &dev1Set.endWattH, sizeof(uint32_t));
		memcpy(&txData[36], &dev1Set.endPower, sizeof(uint32_t));

		d_msg("=== Op start %X Mode %X Volt %d cur %d EndCur %d EndTime %d MsrT %d ===\n", cmd, dev1Set.opModeCmd, volt, cur, endCur, dev1Set.endTime, dev1Set.msrTime);
	}
	else if (cmd == CAN_PROTECT_DATA_CMD)
	{
		len = 8 * 3;
		memcpy(&txData[0], &protectSet, sizeof(protectSet));
		if(dev1Set.type == _160V_60A_32CH_TYPE)		memcpy(&txData[0], &protectSet_160v32ch, sizeof(protectSet_160v32ch));
	}
	else if (cmd == CAN_START_PATTERN_CMD) {
		len = 16;
		txData[0] = dev1Set.opModeCmd;		// 0x11 CC, 0x14  CP,
		txData[1] = dev1Set.stepNum;
		txData[2] = dev1Set.parallelEn;					// 0 : Standalone   1 : Parallel
		txData[3] = 0x01;  // 0x01 경우 충방전 누적응답...
		memcpy(&txData[6], &dev1Set.msrTime, sizeof(uint16_t));
		memcpy(&txData[8], &dev1Set.volt, sizeof(uint16_t));		// 충전 CV 전압
		memcpy(&txData[10], &dev1Set.endCur, sizeof(uint16_t));		// 방전 CV 전압.
		d_msg("CAN_START_PATTERN_CMD %d  %d %d\n", len, dev1Set.volt, dev1Set.endCur);
	}
	else if (cmd == CAN_PATTERN_UPDATE_CMD)
	{
		len = 8;
		int32_t data = dev1Set.sendPattenData;
		//txData[0] = 0x11;  // 0x11 CC, 0x14  CP,
		dev1Set.sendPattenTime = dev1Set.endTime;
		memcpy(&txData[0], &dev1Set.sendPattenTime, sizeof(uint32_t));
		if (dev1Set.sendPattenCnt % 2 == 1) data = dev1Set.sendPattenData * -1;  // 홀수 경우 방전.
		memcpy(&txData[4], &data, sizeof(int32_t));
	}
	else if (cmd == CAN_START_CV_CMD)
	{
		len = 8;
		endCur = dev1Set.endCur;
		volt = 0;  // 전압 50mV 감소량.
		memcpy(&txData[0], &endCur, sizeof(uint32_t));
		memcpy(&txData[4], &volt, sizeof(uint32_t));
	}
	else if (cmd == CAN_CHANGE_CUR_CMD)
	{
		len = 8;
		endCur = dev1Set.cur;
		memcpy(&txData[0], &endCur, sizeof(uint32_t));
	}
	else if (cmd == CAN_STATE_CMD) {}	//보드 충방전 상태 요청 .추가설정필요 없음.
	else if (cmd == CAN_SET_CONTACT_CMD) {		//  접촉 검사 설정값 적용
		len = 14;	memcpy(&txData[0], &contactSet, len);
	}
	else if (cmd == CAN_REST_CMD) {
		len = 8;
		txData[0] = dev1Set.stepNum;
		txData[1] = dev1Set.parallelEn;
		memcpy(&txData[2], &dev1Set.msrTime, sizeof(uint16_t));
		memcpy(&txData[4], &dev1Set.endTime, sizeof(uint32_t));
	}
	else if (cmd == CAN_CHK_CONTACT_CMD) {}	//접촉 검사 결과 확인. .추가설정필요 없음.
	else if (cmd == CAN_RUN_OCV_CMD) {}	//OCV 실행. .추가설정필요 없음.
	else if (cmd == CAN_READ_OCV_CMD) {}	//OCV 값 요청. .추가설정필요 없음.
	else if (cmd == CAN_CELL_BYPASS_CMD) {
		len = 8;
		memcpy(&txData[0], &dev1Set.activeCh, sizeof(uint32_t));
		txData[4] = dev1Set.activeCmd1;
	}	
	else if (cmd == CAN_CHK_VERSION_CMD) {
		TogVer = ((CButton*)GetDlgItem(IDC_CHECK_VER_EX))->GetCheck();
		if (TogVer) {
			len = 1;
			txData[0] = 0x01;
		}
		else {
			len = 0;
			txData[0] = 0x00;
		}
		//txData[0] = 0x01;
	}	//FW버전 확인. .추가설정필요 없음.

	else if (cmd == CAN_CHK_COM_CMD) {}	//통신연결 확인. .추가설정필요 없음.
	else if (cmd == CAN_READ_TEMP_CMD) {
		if (TogVer) {
			len = 1;
			txData[0] = 0x03;		// 적전 공정 마지막 전압, CP 최대 전류 적용
		}
	}
	else if (cmd == CAN_SET_PACK_USER_DATA_CMD) {
		len = 8;
		//uint32_t cmd = 0x003EAD40;
		//uint32_t cmd = 0x0040AD3E;  // manufacturer name
		//cmd = 0x00004C3E;  // manufacturer name
		/*txData[0] = 0x00;
		txData[1] = 0x3E;
		txData[2] = 0xAD;
		txData[3] = 0x40;*/
		memcpy(&txData[0], &dev1Set.PackUserCmd, sizeof(uint32_t));
		txData[4] = dev1Set.PackUserCmdLen;
		txData[5] = dev1Set.PackUserReadLen;
		//d_msg("SET_PACK_USER_DATA_CMD %02X %02X %02X %02X\n", (uint8_t)txData[0], (uint8_t)txData[1], (uint8_t)txData[2], (uint8_t)txData[3]);
	}
	else if (cmd == CAN_READ_PACK_DATA_CMD) {
		len = 1;
		txData[0] = dev1Set.PackDataType;
	}
	else if (cmd == CAN_TIME_SYNC_CMD) {
		len = 8;
	}
	else if (cmd == CAN_SET_PACK_COM_CMD) {
		len = 24;

		txData[0] = dev1Set.PackBMS_CommType;  // 0x00 None, 0x10 i2c, 0x20 smbus
		txData[1] = dev1Set.PackBMS_CommAddr;  // 0x00 None, i2c 0x55, smbus 0x0B
		uint16_t comSpeed = 100;
		memcpy(&txData[2], &comSpeed, sizeof(uint16_t));
		txData[4] = dev1Set.PackDataType;  // 기본 , 사용자 설정.
		txData[5] = dev1Set.PackDataCnt;  // 갯수
		txData[6] = GetDlgItemInt(IDC_EDIT_PACK3); // 자동전송 유무 (0 폴링, 1은 자동전송)
		if (dev1Set.PackDataType == 0)
		{

		}
		else if (dev1Set.PackDataType == 1)  // 사용자 설정.
		{
			memcpy(&txData[8], (void*)&dev1Set.PackReadCmd[0], sizeof(uint8_t) * 12);
			/*txData[8] = 0x0A;	txData[9] = 0x2C;	txData[10] = 0x08;	txData[11] = 0x0C;
			txData[12] = 0x0A;	txData[13] = 0x2C;	txData[14] = 0x08;	txData[15] = 0x0C;
			txData[16] = 0x0A;	txData[17] = 0x2C;	txData[18] = 0x08;	txData[19] = 0x0C;*/
		}
		else if (dev1Set.PackDataType == 100)
		{
			txData[0] = 0x00;		// 통신설정 없음.
			txData[5] = 0;
		}
	}
	if (len > 8) {
		int sendCnt = len / 8;
		for (int i = 0; i <= sendCnt; i++)
		{
			if (i != sendCnt) SendCAN_Data(8, &txData[i * 8], canid.all + i);
			else {
				if (len % 8 != 0) SendCAN_Data(len % 8, &txData[i * 8], canid.all + i); // 마지막 길이가 남아 있으면 전송
			}
			//else SendCAN_Data(len % 8, &txData[i * 8], canid.all + i);
			//Wait(2);
		}
	}
	else {
		ret = SendCAN_Data(len, txData, canid.all);
	}
	d_msg(GetTimeStamp(0));
	d_msg("# SendControlCmd id [%02X] %d cmd[%02X] [%d] data [", id, id, cmd, len);
	for (int i = 0; i < len; i++) { d_msg(" %2.2X", (uint8_t)txData[i]); }
	d_msg(" ]\n");
	//CanRcvCnt = CAN_RCV_CHK_TIMER;
	return ret;
}

void CDisChgTesterDlg::OnBnClickedBtnCh1Start()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	CString strTemp;
	int32_t endCur, volt, cur, power;
	CString strBox;
#ifndef _DEBUG
	if (openSuccess == false) {
		SetInfoMsg("장치가 연결 되지 않음!");
		return;
	}
#endif

	if (dev1Set.type == _TEST_DUTY_TYPE) {
		char txData[8] = { 0, };
		int len = 3;
		uint8_t runPhase = 1;
		txData[0] = 0x10;
		txData[1] = 0x0;
		txData[2] = 0x0;
		
		if (dev1Set.testModeType == TEST_DUTY_160V60A) {
			len = 8;	
			runPhase = GetDlgItemInt(IDC_EDIT_CH_DROP_VOLT);
			txData[3] = runPhase;
			if(runPhase > 4 || runPhase == 0) {
				SetInfoMsg("Phase 설정 범위 초과!");
				return;
			}			
		}
		SendCAN_CmdData(dev1Set.id, CAN_TEST_MODE_CMD, txData, len);
		//SendControlCmd(dev1Set.id, CAN_TEST_MODE_CMD);
		strTemp.Format("ID %d 제어기 Phase %d Test Mode 시작!", dev1Set.id, runPhase);
		SetInfoMsg(strTemp);
		OnBnClickedBtnListClear();
		dev1Set.start = true;
		dev1Set.dspErrInfo = true;
		dev1Set.csvFileName.Format("%s_ID%d_TEST_MODE.csv", GetTimeStamp(1), dev1Set.id);

		dev1Set.csvFile.Format("날짜,%s,,장치ID,%d,공정,%s,모드,%s,,설정전압,%d,mV,설정전류,%d,mA,종지전류,%d,mA,\n", GetTimeStamp(1), dev1Set.id, opCmdMsg[dev1Set.opCmd], opModeMsg[dev1Set.opMode], 0, 1, 2);
		CString addText;
		addText.Format(",,채널 1,,\n");
		dev1Set.csvFile.Append(addText);
		addText.Format("Time,Cnt,step,상태,전압,전류,용량,WattH,시간,종료,에러,온도\n");		
		dev1Set.csvFile.Append(addText);
		m_EditCsvName.SetWindowTextA(dev1Set.csvFileName);
		((CButton*)GetDlgItem(IDC_CHECK_AUTO_MSR))->SetCheck(true);
		OnBnClickedCheckAutoMsr();
		GetDlgItem(IDC_BTN_CH1_START)->EnableWindow(FALSE);
		SetWindowUI(DEV_1, 1);
		return;
	}
	GetDevSetData(DEV_1, 0);		//다이얼로그 값 가져오기..	


	if (dev1Set.opCmd == DEV_CHG_MODE) {
		endCur = dev1Set.endCur;
		volt = dev1Set.volt;
		cur = dev1Set.cur;
		if (dev1Set.opModeCmd == CR_MODE) {
			SetInfoMsg("CR Mode Err..");
			return;
		}
	}
	else if (dev1Set.opCmd == DEV_DISCHG_MODE)
	{
		endCur = dev1Set.dis_endCur;
		volt = dev1Set.dis_volt;
		cur = dev1Set.dis_cur;
	}
	else {  // 패턴모드..
		endCur = dev1Set.endCur;
		volt = dev1Set.volt;
		cur = dev1Set.cur;
		dev1Set.sendPattenCnt = 10000;
		dev1Set.sendPattenTime = dev1Set.endTime;// dev1Set.msrTime;
		dev1Set.sendPattenData = dev1Set.cur;
	}

	power = dev1Set.power;
	if (!adminMode) {
		if (volt > SET_MAX_VOLT || volt < SET_MIN_VOLT) {
			strBox.Format("설정 전압값 문제\n %d ~ %d mV 설정요함", SET_MIN_VOLT, SET_MAX_VOLT);
			AfxMessageBox(strBox, MB_ICONSTOP);
			return;
		}
		if (cur > SET_MAX_CUR) {
			strBox.Format("설정 전류값 문제\n %d mA 초과됨.", SET_MAX_CUR);
			AfxMessageBox(strBox, MB_ICONSTOP);
			return;
		}
	}

	if (dev1Set.type != _5V_20A_PACK_TYPE)
	{
		if (dev1Set.start)
		{
			SetInfoMsg("충방전 진행중..!");
			return;
		}
	}
	else {
		SendControlCmd(dev1Set.id, CAN_CH_STOP_CMD);
		/*strTemp.Format("ID [%d] 정지 후 다시 진행!", dev1Set.id);
		SetInfoMsg(strTemp);*/
		Sleep(200);
	}
	if (dev1Set.type == _160V_60A_32CH_TYPE) {
		if (dev1Set.startVolt > 160 * 100) {
			SetInfoMsg("초기 충전 전압 160V 초과..!");
			return;
		}
		/*if (dev1Set.activeCh == 0)
		{
			SetInfoMsg("동작 cell이 없음..!");
			return;
		}*/

	}
	d_msg("start\n");
	if (dev1Set.type == _74V_80A_TYPE)
	{
		((CButton*)GetDlgItem(IDC_CHECK_AUTO_RELAY))->SetCheck(false);
	}

	// 관리자 모드 아닌 경우, 설정값 확인.
	/*if (!adminMode) {
		CString strBox;
		d_msg("admin %d %d\n", dev1Set.volt, dev1Set.dis_volt);
		if (dev1Set.volt > SET_MAX_VOLT || dev1Set.volt < SET_MIN_VOLT || dev1Set.dis_volt < SET_MIN_VOLT) {
			strBox.Format("제어기 ID %d\n설정 전압값 문제\n %d ~ %d mV 설정요함", dev1Set.id, SET_MIN_VOLT , SET_MAX_VOLT);
			AfxMessageBox(strBox, MB_ICONSTOP);
			return;
		}
		else if (dev1Set.endCur < SET_END_CUR) {
			strBox.Format("제어기 ID %d\n종지 전류값 문제\n %d mA 이상 설정요함", dev1Set.id, SET_END_CUR);
			AfxMessageBox(strBox, MB_ICONSTOP);
			return;
		}
	}*/

	//dev1Set.startDelay = true;


	if (dev1Set.opCmd == DEV_CHG_MODE)			SendControlCmd(dev1Set.id, CAN_START_CHG_CMD);
	else if (dev1Set.opCmd == DEV_DISCHG_MODE)	SendControlCmd(dev1Set.id, CAN_START_DISCHG_CMD);
	else if (dev1Set.opCmd == DEV_PATTERN_MODE) {
		SendControlCmd(dev1Set.id, CAN_START_PATTERN_CMD);
	}
	else {

	}

	OnBnClickedBtnListClear();
	dev1Set.start = true;
	dev1Set.dspErrInfo = true;
	dev1Set.csvFileName.Format("%s_ID%d_%s_%s#%3.3fV_%4.3f-%4.3fA.csv", GetTimeStamp(1), dev1Set.id, opCmdMsg[dev1Set.opCmd], opModeMsg[dev1Set.opMode], volt * MSR_POINT_UNIT, cur * MSR_POINT_UNIT, endCur * MSR_POINT_UNIT);


	dev1Set.csvFile.Format("날짜,%s,,장치ID,%d,공정,%s,모드,%s,,전압,%d,mV,전류,%d,mA, %d,mW,종지전류,%d,mA,\n", GetTimeStamp(1), dev1Set.id, opCmdMsg[dev1Set.opCmd], opModeMsg[dev1Set.opMode], volt, cur, power, endCur);


	CString addText, addTextGrp;
	addText.Format(",,채널 1,,\n");
	dev1Set.csvFile.Append(addText);

	if (dev1Set.type == _5V_20A_PACK_TYPE) {
		addText.Format("Time,Cnt,step,상태,전압,전류,용량,WattH,시간,종료,에러,온도,P-Bat,P-SOC,P-Volt,P-Cur,P-온도\n");
	}
	else if (dev1Set.type == _160V_60A_32CH_TYPE) {
		addText.Format("Time,Cnt,step,상태,전압,전류,용량,WattH,시간,종료,에러,IR,,");
		for (int i = 0; i < MAX_BMS_CH; i++) {
			addText.AppendFormat("ch%d 상태,", i + 1);
		}
		addText.AppendFormat(",");
		for (int i = 0; i < MAX_BMS_CH; i++) {
			addText.AppendFormat("ch%d 전압,", i + 1);
		}
		addText.AppendFormat("Run 전압,");

		for (int i = 0; i < MAX_BMS_CH; i++) {
			addText.AppendFormat("ch%d d-V,", i + 1);
		}
		addText.AppendFormat(",");
		for (int i = 0; i < MAX_BMS_CH; i++) {
			addText.AppendFormat("ch%d 온도,", i + 1);
		}
		addText.AppendFormat("\n");
		// 상태 요청 활성화.
		((CButton*)GetDlgItem(IDC_CHECK_AUTO_MSR))->SetCheck(true);
		dev1Set.autoMsrReq = true;
	}
	else {
		addText.Format("Time,Cnt,step,상태,전압,전류,용량,WattH,시간,종료,에러,온도,기타\n");
	}

	dev1Set.csvFile.Append(addText);

	if (dev1Set.setCur != _SET_200A && dev1Set.type == _5V_200A_TYPE)
	{
		memset(rcvMeasure, 0x00, sizeof(Rcv_Measure_t) * 20);
		dev1Set.csvGrpFileName.Format("%s_ID%d_%s_%s#%3.3fV_%4.3f-%4.3fA_Grp.csv", GetTimeStamp(1), dev1Set.id, opCmdMsg[dev1Set.opCmd], opModeMsg[dev1Set.opMode], volt * MSR_POINT_UNIT, cur * MSR_POINT_UNIT, endCur * MSR_POINT_UNIT);
		dev1Set.csvGrpFile.Format("날짜,%s,,장치ID,%d,공정,%s,모드,%s,,전압,%d,mV,전류,%d,mA, %d,mW,종지전류,%d,mA,\n", GetTimeStamp(1), dev1Set.id, opCmdMsg[dev1Set.opCmd], opModeMsg[dev1Set.opMode], volt, cur, power, endCur);

		for (int i = 1; i <= 11; i++)
		{
			addText.Format("Ch-BD %d,,,,,,", i);
			dev1Set.csvGrpFile.Append(addText);
		}
		dev1Set.csvGrpFile.Append("\n");
		//addTextGrp.Format("Time,Cnt,step,상태,전압,전류,용량,WattH,시간,종료,에러,온도");
		for (int i = 0; i <= 10; i++)
		{
			addText.Format("상태,시간,전압,전류,용량,WattH,");
			dev1Set.csvGrpFile.Append(addText);
		}
		dev1Set.csvGrpFile.Append("\n");
		dev1Set.lastId = 0;
	}

	m_EditCsvName.SetWindowTextA(dev1Set.csvFileName);
	if (!adminMode) {
		SetWindowUI(DEV_1, 1);
		SetWindowUI(DEV_ALL, -1);
	}

	strTemp.Format("제어기 ID %d[0x%X] %s 시작!", dev1Set.id, dev1Set.id, opCmdMsg[dev1Set.opCmd]);
	SetInfoMsg(strTemp);
	strTemp.Format("ID 0x%X", dev1Set.id);
	GetDlgItem(IDC_STATIC_CH_ID)->SetWindowText(strTemp);

	if (dev1Set.type == _5V_20A_PACK_TYPE)	dev1Set.PackDataSave = false;

}

void CDisChgTesterDlg::OnBnClickedBtnCh1Stop()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	CString strTemp;
	dev1Set.lastId = 0;

#ifdef CAN_PACKET_TEST
	if (m_isPacketTest) {
		m_isPacketTest = false;
		SetInfoMsg("packet Test Stop.");
		return;
	}
#endif

#ifndef _DEBUG
	if (openSuccess == false) {
		SetInfoMsg("장치가 연결 되지 않음!");
		return;
	}
#endif

	if (dev1Set.type == _TEST_DUTY_TYPE) {
		char txData[8] = { 0, };
		txData[0] = (char)0xF0;
		txData[1] = 0x0;
		txData[2] = 0x0;
		SendCAN_CmdData(dev1Set.id, CAN_TEST_MODE_CMD, txData, 3);
		//SendControlCmd(dev1Set.id, CAN_START_CHG_CMD);
		strTemp.Format("제어기 ID %d Test Mode 종료!", dev1Set.id);
		SetInfoMsg(strTemp);
		dev1Set.start = false;
		OnBnClickedBtnListSave();
		((CButton*)GetDlgItem(IDC_CHECK_AUTO_MSR))->SetCheck(false);
		OnBnClickedCheckAutoMsr();
		GetDlgItem(IDC_BTN_CH1_START)->EnableWindow(TRUE);
		SetWindowUI(DEV_1, 0);
		// 정지시 전류 하강 문제가 있어 삭제.. 23.06.27
		//Sleep(100);
		//SendControlCmd(dev1Set.id, CAN_CH_STOP_CMD);  // 에러 상태 초기화 하기 위해 테스트 모드에서도 정지 명령 추가.. 
		return;
	}
	dev1Set.start = false;
	GetDevSetData(DEV_1, 0);		//다이얼로그 값 적용.
	//dev1Set.stopCnt = DEV_STOP_READ_CNT;
	SendControlCmd(dev1Set.id, CAN_CH_STOP_CMD);
	if (!adminMode) {
		SetWindowUI(DEV_1, 0);
		SetWindowUI(DEV_ALL, -1);
	}

	strTemp.Format("제어기 ID %d %s 종료!", dev1Set.id, opCmdMsg[dev1Set.opCmd]);
	SetInfoMsg(strTemp);
	if (m_ChkboxSaveCsv.GetCheck()) OnBnClickedBtnListSave();

	dev1Set.sendPattenCnt = 0;		// 패턴 종료.

	// 직렬형 충방전기 Daq 릴레이 제어 초기화.
	if (dev1Set.type == _74V_80A_TYPE)
	{
		dev1Set.relayCmd = 0;

		int getTime = GetDlgItemInt(IDC_EDIT_CH1_END_TIME);  // 
		if (getTime > 0) 	dev1Set.relayCmdTime = getTime;
		else  dev1Set.relayCmdTime = 5;

		((CButton*)GetDlgItem(IDC_CHECK_AUTO_RELAY))->SetCheck(false);
	}
	if(FwCnt) FwCnt = 0;

#ifdef CYCLE_MODE
	if (dev1Set.cycleMode) dev1Set.cycleRestTime = REST_TIMER;
#endif
}

void CDisChgTesterDlg::OnBnClickedBtnT1()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.	
	GetDevSetData(DEV_1, 0);		//다이얼로그 값 적용.
	dev1Set.maxChCnt = 0;
	//dev1Set.lastId = 0;
#ifndef CAN_PACKET_TEST	
	SendControlCmd(dev1Set.id, CAN_CHK_VERSION_CMD);
#else
	int delay = dev1Set.msrTime;
	int cnt = GetDlgItemInt(IDC_EDIT_CH1_END_TIME);
	int chg = dev1Set.power;
	long id;
	int len, ext, rtr;
	char data[8] = { 0, };
	CString str = "";
	str.Format("packet Test 지연(측정시간): %d, 횟수(시간): %d, ID 변경(파워W): %d ", delay, cnt, chg);
	SetInfoMsg(str);
	GetDlgItem(IDC_BTN_T1)->EnableWindow(FALSE);
	m_isUpdateFW = true;
	m_isPacketTest = true;
	while (cnt--)
	{
		SendControlCmd(dev1Set.id, CAN_CHK_VERSION_CMD);
		if (chg) {
			dev1Set.id++;
			if (dev1Set.id > 4) dev1Set.id = 1;
		}
		usleep(delay);
		if (m_isPacketTest == false) cnt = 1;
	}
	usleep(1000);
	while (CAN_Recv(hBus, &id, &len, data, &ext, &rtr)) {}
	SetInfoMsg("Rx Clear.");
	usleep(1000);
	SetInfoMsg("packet Test End.");
	GetDlgItem(IDC_BTN_T1)->EnableWindow(TRUE);
	m_isUpdateFW = false;
#endif
}

void CDisChgTesterDlg::OnBnClickedBtnT2()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	GetDevSetData(DEV_1, 0);		//다이얼로그 값 갱신.
	//SendControlCmd(dev1Set.id, CAN_RUN_OCV_CMD);
	//dev1Set.sendTimerCnt = 2;			// 2초 후 OCV값 요청.
	//dev1Set.sendCmd = CAN_READ_OCV_CMD;
	SendControlCmd(dev1Set.id, CAN_READ_OCV_CMD);
}

// 각 채널별 설정값 int 형 리턴
int CDisChgTesterDlg::GetDevSetData(uint8_t ch, uint8_t type)
{
	// TODO: 여기에 구현 코드 추가.
	//UI 데이터 반영
	// 전압, 전류값 소수점자리(0.1mV) 및 전류 음수 표시 원래대로 적용. *1000
	int ret = -1;
	CString tempStr, voltStr, curStr, endCurStr, cellvoltStr;
	CComboBox* pComboBox;	
	// 동일 id 확인을 위해 조건 없이 id 갱신.
	m_ComboCh1_ID.GetWindowText(tempStr);
	dev1Set.id = _ttoi(tempStr);
	dev1Set.grpId = dev1Set.id & 0xF0;
	dev1Set.autoMsrReq = ((CButton*)GetDlgItem(IDC_CHECK_AUTO_MSR))->GetCheck();
	dev1Set.autoTempReq = ((CButton*)GetDlgItem(IDC_CHECK_AUTO_SEND))->GetCheck();
	dev1Set.autoRlyReq = ((CButton*)GetDlgItem(IDC_CHECK_AUTO_RELAY))->GetCheck();
	pComboBox = (CComboBox*)GetDlgItem(IDC_TYPE_CB);
	dev1Set.type = pComboBox->GetCurSel();
	getCanId = dev1Set.id;
	
	if (ch == 1)
	{
		pComboBox = (CComboBox*)GetDlgItem(IDC_COMBO_CH1_OP);
		dev1Set.opCmd = pComboBox->GetCurSel();  // 충방전
		GetDlgItemText(IDC_EDIT_CH1_VOLT, voltStr);
		GetDlgItemText(IDC_EDIT_CH1_CUR, curStr);
		GetDlgItemText(IDC_EDIT_CH1_END_CUR, endCurStr);
		if (dev1Set.opCmd == DEV_DISCHG_MODE) {
			dev1Set.dis_volt = (int)((float)atof(voltStr) * MSR_UNIT);
			dev1Set.dis_cur = (int)((float)atof(curStr) * MSR_UNIT * -1);
			dev1Set.dis_endCur = (int)((float)atof(endCurStr) * MSR_UNIT);  // 종료전류는 +값만..
			//dev1Set.dis_volt = GetDlgItemInt(IDC_EDIT_CH1_VOLT) * MSR_UNIT;
			//dev1Set.dis_cur = GetDlgItemInt(IDC_EDIT_CH1_CUR) * (MSR_UNIT * -1);			// 방전 경우 전류값 음수(-) 처리.
			//dev1Set.dis_endCur = GetDlgItemInt(IDC_EDIT_CH1_END_CUR) * MSR_UNIT;
		}
		else {
			dev1Set.volt = (int)((float)atof(voltStr) * MSR_UNIT);
			dev1Set.cur = (int)((float)atof(curStr) * MSR_UNIT);
			dev1Set.endCur = (int)((float)atof(endCurStr) * MSR_UNIT);
			//dev1Set.volt = GetDlgItemInt(IDC_EDIT_CH1_VOLT) * MSR_UNIT;
			//dev1Set.cur = GetDlgItemInt(IDC_EDIT_CH1_CUR) * MSR_UNIT;
			//dev1Set.endCur = GetDlgItemInt(IDC_EDIT_CH1_END_CUR) * MSR_UNIT;
		}
		dev1Set.power = GetDlgItemInt(IDC_EDIT_CH1_POWER) * MSR_UNIT;

		if (dev1Set.type == _160V_60A_32CH_TYPE) {
			/*BOOL trans = FALSE;
			dev1Set.activeCh = GetDlgItemInt(IDC_EDIT_CH1_POWER, &trans, FALSE);*/
		}


		pComboBox = (CComboBox*)GetDlgItem(IDC_COMBO_CH1_MODE);
		dev1Set.opMode = pComboBox->GetCurSel();  // cc, cv, cccv, cp, cpcv...
		if (dev1Set.opMode == 0) dev1Set.opModeCmd = CC_MODE;
		else if (dev1Set.opMode == 1) dev1Set.opModeCmd = CV_MODE;
		else if (dev1Set.opMode == 2) dev1Set.opModeCmd = CCCV_MODE;
		else if (dev1Set.opMode == 3) dev1Set.opModeCmd = CP_MODE;
		else if (dev1Set.opMode == 4) dev1Set.opModeCmd = CR_MODE;
		else if (dev1Set.opMode == 5) dev1Set.opModeCmd = CPCV_MODE;
		else d_msg("=== opMode Err %d === \n", dev1Set.opMode);

		dev1Set.endTime = GetDlgItemInt(IDC_EDIT_CH1_END_TIME) * MSR_UNIT;
		//dev1Set.stepNum = GetDlgItemInt(IDC_EDIT_CH1_STEP);
		dev1Set.stepNum = 0;
		dev1Set.msrTime = GetDlgItemInt(IDC_EDIT_CH1_MSR_TIME);
		//dev1Set.endTimerCnt = dev1Set.endTime * 60;
		//dev1Set.maxVolt = 1000; // 사용안함.
		dev1Set.parallelEn = ((CButton*)GetDlgItem(IDC_CHECK_GROUP))->GetCheck();;
		dev1Set.endCap = GetDlgItemInt(IDC_EDIT_CH1_END_CAP);
		dev1Set.endWattH = GetDlgItemInt(IDC_EDIT_CH1_END_WATTH);
		dev1Set.endPower = GetDlgItemInt(IDC_EDIT_CH1_END_POWER) * MSR_UNIT;
		if (dev1Set.type == _160V_60A_32CH_TYPE)
		{
			GetDlgItemText(IDC_EDIT_CH1_STEP, cellvoltStr);
			dev1Set.startVolt = (int)(((float)atof(cellvoltStr) * 100)+0.5f);
			//dev1Set.cellVolt = GetDlgItemInt(IDC_EDIT_CH1_STEP);
			//d_msg("cellVolt %s %d\n", cellvoltStr, dev1Set.cellVolt);
		}

		return ch;
	}
	else if (ch == 2)
	{
		return ch;
	}
	else return -1;
}

void DspCanData(long id, char data[])
{
	d_msg("%s %0.8X", GetTimeStamp(4), id);
	for (int i = 0; i < 8; i++) {
		d_msg(" %2.2X", (uint8_t)data[i]);
	}
	d_msg("\n");
	// 스레드에서 CString 사용 못함.. 메모리 위반 나옴.
#if 0
	CString msg = "";
	msg.Format("%s %0.8X", GetTimeStamp(4), id);
	for (int i = 0; i < 8; i++) {
		msg.AppendFormat(" % 2.2X", (uint8_t)data[i]);
	}
#ifdef DEBUG
	d_msg("%s\n", msg);
#endif
	canMsgLog.AppendFormat("%s\n", msg);
	return 0;
#endif

}

bool compare(const Msg& p1, const Msg& p2)
{
	/*uint8_t idNum1 = (p1.id & 0x0000FFF) >> 8;
	uint8_t idNum2 = (p2.id & 0x0000FFF) >> 8;*/
	int idNum1 = (p1.id & 0x00FFFFF);
	int idNum2 = (p2.id & 0x00FFFFF);
	//printf(" dont change.. %lx %lx\n", idNum1, idNum2);
	// 0xf 다음 0x01, 0x02, 0x3..까지 받을 경우는..정렬 하지 않음...
	// 시퀀스 넘버가 0, 1,2,....F..1,2 로 변환되는데 0xf 다음에는 정렬 예외 처리필요.
	//if ((idNum1 == 0x1 || idNum1 == 0x2 || idNum1 == 0x3 || idNum1 == 0x4 || idNum1 == 0x5 || idNum1 == 0x6 || idNum1 == 0x7) && idNum2 == 0xF) {
	//	//printf("dont change..\n");
	//	return p1.id > p2.id;
	//}
#if 0
	// 비교 데이터와 차이가 - 라면 정렬 안함.
	if ((0x0F <= idNum1 && idNum1 >= 0x01) && (0x0F <= idNum2 && idNum2 >= 0x01))
	{
		if (idNum1 - idNum2 < 0 || ((idNum1 == 0x1 || idNum1 == 0x2 || idNum1 == 0x3 || idNum1 == 0x4 || idNum1 == 0x5 || idNum1 == 0x6 || idNum1 == 0x7) && idNum2 == 0xF)) {
			//	//printf("[%d] dont change.. %lx %lx\n", cmpareCnt, p1.id, p2.id);
			return p1.id > p2.id;
		}
		else
			return p1.id < p2.id;
	}
	else if (p1.id == p2.id) {
		return p1.id > p2.id;
	}
	else {
		return p1.id < p2.id;
	}
#endif
	if (p1.id == p2.id) {
		return p1.id > p2.id;
	}
	else {
		// 직전 데이터가 1이상.. 차이가 - 일 경우 정렬 안함.
		if (idNum1 >= 1 && idNum2 >= 1) {
			if (idNum1 - idNum2 < 0) {
				return p1.id > p2.id;
			}
			else return p1.id < p2.id;
		}
		return p1.id < p2.id;
	}
	//return p1.id < p2.id;
}
#ifdef THRED_SORT
UINT ThreadRecvCAN_Data(LPVOID param)
{
#define MAX_CNT			128
#define CHK_ID          0xFFFFF00
	//const int PCAN_RCV_CHK_TIMER = 6;
	//const int MWCAN_RCV_CHK_TIMER = 6;
	const int RCV_CHK_TIMER = 6;
	CDisChgTesterDlg* pMain = (CDisChgTesterDlg*)param;

	TPCANMsg CANMsg;
	TPCANTimestamp CANTimeStamp;
	TPCANStatus stsResult = PCAN_ERROR_QRCVEMPTY;
	long id;
	int len, ext, rtr;
	char data[8] = { 0, };
	int chklen = 3;
	int parserId = dev1Set.id;
	int canId = 0;

	while (m_isRecvCANThread)
	{
		if (!openSuccess) {
			d_msg("openSuccess %d\n", openSuccess);
			return -1;
		}
		// 80A, 200A 패턴경우, 수신데이터 4프레임으로 수정.
		if (dev1Set.type == _74V_80A_TYPE || (dev1Set.type == _5V_200A_TYPE && dev1Set.opCmd == DEV_PATTERN_MODE))	chklen = 4;
		else if (dev1Set.type == _160V_60A_32CH_TYPE || dev1Set.testModeType == TEST_DUTY_160V60A)	chklen = 20;
		else chklen = 3;

		// USB_PCAN 은 기본2ms 지연, MW_USB2CAN은 4ms
		if (dev1Set.msrTime < 500) {
			rcvCAN_chkTimer = 3;  //60ms
			if (pMain->m_CanDevType == USB_PCAN) rcvCAN_chkTimer = 10; //1000ms
			//rcvCAN_chkTimer = 10;
		}
		else {
			rcvCAN_chkTimer = 10;		//200ms
			if (pMain->m_CanDevType == USB_PCAN) rcvCAN_chkTimer = 50; //100ms
			//rcvCAN_chkTimer = 50; //100ms
		}
		static int rcvCnt = 0, endCnt = rcvCAN_chkTimer;// parserCnt = 0, parserDataCnt = 0;
		static long msgHead[2] = { 0, };				// id , 전체 수신갯수		
		static Msg msg[MAX_CNT] = { 0, };
		bool rxOk = false;
		long chkid = 0, nextid = 0;
		long filterId = 0;
		static uint8_t SendData[512] = { 0, };
		uint32_t SendCnt = 0;
		static int chkcnt = 0;
		static int chkBmsCnt = 0;
		static int chkStartCnt = 0;
		bool RecvEn = false;
		//endCnt = rcvCAN_chkTimer;

		if (!pMain->m_isUpdateFW)
		{
			if (CanRcvCnt == 0)
			{
				try
				{
					do
					{

						if (pMain->m_CanDevType == USB_PCAN)		stsResult = pMain->m_objPCANBasic->Read(pMain->m_PcanHandle, &CANMsg, &CANTimeStamp);
						else if (pMain->m_CanDevType == MW_USB2CAN)	RecvEn = CAN_Recv(hBus, &id, &len, data, &ext, &rtr);
						else return -1;

						if (stsResult != PCAN_ERROR_QRCVEMPTY || RecvEn)
						{
							if (rcvCnt >= MAX_CNT - 2) {
								rcvCnt = 0;
								RecvEn = 0;
								endCnt = 1;
							}
							if (pMain->m_CanDevType == USB_PCAN)
							{
								id = CANMsg.ID;
								len = CANMsg.LEN;
								for (int i = 0; i < len; i++)	data[i] = CANMsg.DATA[i];
							}
							canId = (id >> 20);

							if (dev1Set.setCur == _SET_1KA || dev1Set.setCur == _SET_2KA) // 병렬경우 마지막 ID 기준으로 파싱.
							{
								if (canId >= dev1Set.lastId)  dev1Set.lastId = canId;
								parserId = dev1Set.lastId;
							}
							else parserId = dev1Set.id;

							//endCnt = 1;
							if (parserId == canId) // 동일 ID 라면 수신 처리 진행
							{
								for (int i = 0; i < len; i++) {
									msg[rcvCnt].data[i] = data[i];	// 수신데이터 저장.
								}
								msg[rcvCnt].id = id;
								msg[rcvCnt].len = len;
								rcvCnt++;
								endCnt = rcvCAN_chkTimer;
								if (((id >> 12) & 0x0FF) == (CAN_READ_DATA_CMD + CAN_ACK)) {
									//if (pMain->m_CanDevType == MW_USB2CAN)  
									chkcnt++;

									if (((id & 0xf) == chklen - 1) && setRcvType == 0) {
										endCnt = 1;
									}

								}
								if (((id >> 12) & 0x0FF) == (CAN_READ_PACK_DATA_CMD + CAN_ACK)) {
									chkBmsCnt++;
									if ((id & 0xf) == (chklen + 1) - 1) {
										//if(chkcnt>2) endCnt = 1;
									}
								}
								if (((id >> 12) & 0x0FF) == (CAN_START_CHG_CMD + CAN_ACK)) {

									/*chkStartCnt++;
									if (chkStartCnt-1 == dev1Set.maxChCnt) {
										endCnt = 1;
										d_msg("ev1Set.maxChCnt\n");
									}*/
									/*endCnt = 1;
									d_msg("ev1Set.maxChCnt\n");*/
								}
								//constexpr uint8_t CAN_START_CHG_CMD = 0x08;   // 충전 
								//constexpr uint8_t CAN_START_DISCHG_CMD = 0x09;   // 방전 
							}
							else {
								//1~2kA는 그룹으로 수신 처리 필요.
								if (dev1Set.setCur == _SET_1KA || dev1Set.setCur == _SET_2KA)
								{

									for (int i = 0; i < len; i++) {
										msg[rcvCnt].data[i] = data[i];	// 수신데이터 저장.
									}
									msg[rcvCnt].id = id;
									msg[rcvCnt].len = len;
									rcvCnt++;
									//endCnt = rcvCAN_chkTimer;
									//if (parserId == canId) endCnt = 1;
									//if (dev1Set.lastCh == (id >> 20) &0x0F) endCnt = 1;*/
								}
							}
							//endCnt = rcvCAN_chkTimer;

							//if (chkcnt >= chklen ) // 측정데이터 기준 3회 처리.
							//if (chkcnt % chklen == 0 ) // 측정데이터 기준 3회 처리.							
							//{
							//	//chkcnt = 0;
							//	//ResultCnt = 0;
							//	endCnt = 1;
							//	//break;
							//}	
							//if (rcvCnt >= MAX_CNT/2) endCnt = 1;

							DspCanData(id, data);

							//if (pMain->m_CanDevType == MW_USB2CAN)	 RecvEn = 0;  // USB-CAN 은 한번에 1개씩 처리해야 수신 누락 없음.
						}
						if (endCnt == 1) break;
					} while ((stsResult & PCAN_ERROR_QRCVEMPTY) != PCAN_ERROR_QRCVEMPTY || RecvEn);

				}
				catch (int e)
				{
					d_msg("CAN_Recv Err %d", e);
				}

				if (endCnt == 1 || chkcnt >= chklen)
				{
					//pMain->SendMessageA(WM_USER_CAN_RCV_ALL_MSG, (WPARAM)msgHead, (LPARAM)msgData); // CAN 전체 수신 완료후 전달.

				/*	for (int rcv = 0; rcv < rcvCnt; rcv++) {
						d_msg("%s %0.8X", GetTimeStamp(4), msg[rcv].id);
						for (int i = 0; i < msg[rcv].len; i++) {
							d_msg(" %2.2X", (uint8_t)msg[rcv].data[i]);
						}
						d_msg("\n");
					}*/
					try {
						//if (dev1Set.type != _5V_20A_PACK_TYPE)	
						sort(msg, msg + rcvCnt, compare);
					}
					catch (int e)
					{
						d_msg("CAN_Recv Err %d", e);
					}
					/*d_msgl("=== sort ===");
					for (int rcv = 0; rcv < rcvCnt; rcv++) {
						d_msg("%s %0.8X", GetTimeStamp(4), msg[rcv].id);
						for (int i = 0; i < msg[rcv].len; i++) {
							d_msg(" %2.2X", (uint8_t)msg[rcv].data[i]);
						}
						d_msg("\n");
					}*/

					for (int i = 0; i < rcvCnt; i++) {
						chkid = msg[i].id;
						filterId = chkid & CHK_ID;
						if (i < MAX_CNT - 1) nextid = msg[i + 1].id;
						if ((chkid & CHK_ID) == filterId) {
							if (pMain->m_DspCAN > 1) d_msgl("id %lx %lx", chkid, nextid);
							for (int j = 0; j < msg[i].len; j++) SendData[SendCnt++] = (char)msg[i].data[j];	// 수신데이터 저장.
							if ((nextid & CHK_ID) != filterId) {
								//dbgCnt++;
							/*	printf("[%d]Send Data => ID %lx Cnt %d [", dbgCnt, chkid, SendCnt);
								for (int i = 0; i < SendCnt; i++) printf("%x ", SendData[i]);
								printf("]\n");*/
								msgHead[0] = chkid;
								msgHead[1] = SendCnt;
								pMain->SendMessageA(WM_USER_CAN_RCV_ALL_MSG, (WPARAM)msgHead, (LPARAM)SendData); // CAN 전체 수신 완료후 전달.
								//pMain->OnUpdateRcvData((WPARAM)msgHead, (LPARAM)SendData);  // SendMessageA 가 연속으로 처리 되지 않아 직접 적용.
								SendCnt = 0;

							}
							//if (i == MAX_CNT - 1 && (nextid & CHK_ID) == filterId) {
							//	printf("Send Data2 => ID %lx Cnt %d [", chkid, SendCnt);
							//	for (int i = 0; i < SendCnt; i++) printf("%x ", SendData[i]);
							//	printf("]\n");
							//	msgHead[0] = chkid;
							//	msgHead[1] = SendCnt;
							//	pMain->SendMessageA(WM_USER_CAN_RCV_ALL_MSG, (WPARAM)msgHead, (LPARAM)SendData); // CAN 전체 수신 완료후 전달.
							//	SendCnt = 0;								
							//}							
						}
					}
					rcvCnt = 0;
					endCnt = 0;
					chkcnt = 0;
					chkBmsCnt = 0;
					chkStartCnt = 0;
					memset(msgHead, 0, sizeof(msgHead));
					memset(SendData, 0, sizeof(SendData));
					memset(msg, 0, sizeof(msg));
				}
			}
		}

		if (endCnt) {
			endCnt--;
			//d_msgl("endCnt %d rcvCnt %d chkcnt %d", endCnt, rcvCnt, chkcnt);			
		}
		//if (pMain->m_CanDevType == USB_PCAN)	Sleep(5);
		//else Sleep(20);	// 1ms 로 하면 패킷 수신누락있음??..	
		Sleep(1);

		//usleep(1000);
	}
	return 0;
}
#else
UINT ThreadRecvCAN_Data(LPVOID param)
{
	CDisChgTesterDlg* pMain = (CDisChgTesterDlg*)param;
	while (m_isRecvCANThread)
	{
		if (!openSuccess) {
			d_msg("openSuccess %d\n", openSuccess);
			return -1;
		}
		/*if (pMain->m_isUpdateFW) {
			d_msg("m_isUpdateFW %d\n", pMain->m_isUpdateFW);
			return -1;
		}*/
		long id;
		int len, ext, rtr;
		char data[8] = { 0, };
		static int rcvCnt = 0, endCnt = 0, parserCnt = 0, parserDataCnt = 0;
		static long msgHead[2] = { 0, };				// id , 전체 수신갯수
		static char msgData[4096] = { 0, };			// 전체 수신 데이터.
		//static long msgHeadParser[32][2] = { 0, };	// 파서용 id , 수신갯수
		//CString strTime, strDebug, strData;
		//static struct Exid RcvId = { 0, };

	/*	int ret = CAN_Recv(hBus, &id, &len, data, &ext, &rtr);
		if (ret  == 1) */
		bool rxOk = false;
		if (!pMain->m_isUpdateFW)
		{
			//CanRcvCnt--;
			if (CanRcvCnt == 0)
			{
				while (CAN_Recv(hBus, &id, &len, data, &ext, &rtr))
				{
					if (rcvCnt == 0) {
						memset(msgHead, 0, sizeof(msgHead));
						memset(msgData, 0, sizeof(msgData));
						//memset(msgHeadParser, 0, sizeof(msgHeadParser));
					}
					if (rcvCnt > 4000) 	rcvCnt = 0;

					for (int i = 0; i < len; i++) msgData[rcvCnt++] = data[i];	// 수신데이터 저장.
					msgHead[0] = id;
					msgHead[1] = rcvCnt;
					rxOk = true;
					parserCnt++;

#if 0
					ret = 0;
					for (int i = 0; i < len; i++) msgData[rcvCnt++] = data[i];	// 수신데이터 저장.
					msgHead[0] = id;
					msgHead[1] = rcvCnt;
					RcvId.all = id;

					d_msg("%s msgData size %d\n", GetTimeStamp(4), rcvCnt);
					if (!pMain->m_isUpdateFW) {
						if (rcvCnt == 0 || rcvCnt >= sizeof(msgData) - 2) {   // 첫번째 수신 경우 메세지 초기화.
							memset(msgHead, 0, sizeof(msgHead));
							memset(msgData, 0, sizeof(msgData));
							memset(msgHeadParser, 0, sizeof(msgHeadParser));
							endCnt = 0;
						}
						RcvId.all = id;
						if (RcvId.cnt == 0 && rcvCnt > 8) {		 //  응답 2개이상 수신시 메세지 해더 별도 저장 처리.
							msgHeadParser[parserCnt][0] = msgHead[0];
							if (parserCnt == 0) msgHeadParser[parserCnt][1] = msgHead[1];
							else msgHeadParser[parserCnt][1] = msgHead[1] - parserDataCnt;
							d_msg("[Parser] %d id %0.8X bf %0.8X cnt %d %d %d\n", rcvCnt, id, msgHeadParser[parserCnt][0], msgHeadParser[parserCnt][1], parserCnt, parserDataCnt);
							parserDataCnt += msgHeadParser[parserCnt][1];   // 전체 수신 데이터 길이 누적.
							parserCnt++;

							int total_cnt = 0;
							//d_msg("parserCnt %d\n", parserCnt);
							for (int i = 0; i < parserCnt; i++)
							{
								pMain->PostMessageA(WM_USER_CAN_RCV_ALL_MSG, (WPARAM)msgHeadParser[i], (LPARAM)&msgData[total_cnt]);
								total_cnt += msgHeadParser[i][1];
								//d_msg("parser send[%d] %x %d %d\n", i, msgHeadParser[i][0], msgHeadParser[i][1], total_cnt);
							}
							msgHead[1] = msgHead[1] - total_cnt;
							pMain->PostMessageA(WM_USER_CAN_RCV_ALL_MSG, (WPARAM)msgHead, (LPARAM)&msgData[total_cnt]); // 마지막 데이터 전달.

							d_msg("%s parserCnt %d\n", GetTimeStamp(4), parserCnt);
							parserCnt = 0;
							parserDataCnt = 0;
							rcvCnt = 0;
							endCnt = 0;
						}
						else {
							//pMain->PostMessageA(WM_USER_CAN_RCV_ALL_MSG, (WPARAM)msgHead, (LPARAM)msgData); // CAN 전체 수신 완료후 전달.
							endCnt = CAN_RCV_CHK_TIMER;  //  수신 대기 타이머..갱신.
						}
					}
					else {	 // 펌웨어 업데이트 경우
						CString strData;
						Exid canid = { 0, };
						canid.all = msgHead[0];
						strData.Format("ID %d [%X]", canid.id, canid.cmd);
						pMain->SetInfoMsg(strData);
						rcvCnt = 0;	endCnt = 0;
					}

#endif

#ifdef DEBUG
					//DspCanData(id, data);
#endif // DEBUG
				}
			}
		}
		else {
			while (CAN_Recv(hBus, &id, &len, data, &ext, &rtr)) {
				CString strData;
				Exid canid = { 0, };
				canid.all = id;
				if (canid.cmd == 0xDF) {
					strData.Format("ID %d [%X]", canid.id, canid.cmd);
					pMain->SetInfoMsg(strData);
				}
			}
		}

		if (!rxOk && parserCnt > 0)
		{
			pMain->SendMessageA(WM_USER_CAN_RCV_ALL_MSG, (WPARAM)msgHead, (LPARAM)msgData); // CAN 전체 수신 완료후 전달.
			rcvCnt = 0;
			parserCnt = 0;
			//rxOk = false;
		}
#if 0
		if (ret)
		{
			if (rcvCnt >= sizeof(msgData) - 2) { rcvCnt = 0;	endCnt = 0; }
			if (endCnt) {
				endCnt--;
				if (endCnt == 1 && rcvCnt != 0) {  // 수신데이터 전달..
					//pMain->SendMessage(WM_USER_CAN_RCV_ALL_MSG, (WPARAM)msgHead, (LPARAM)msgData); // CAN 전체 수신 완료후 전달.

					pMain->PostMessageA(WM_USER_CAN_RCV_ALL_MSG, (WPARAM)msgHead, (LPARAM)msgData); // CAN 전체 수신 완료후 전달.
					d_msg("end send\n");
#if 0
					if (parserCnt == 0)	pMain->PostMessageA(WM_USER_CAN_RCV_ALL_MSG, (WPARAM)msgHead, (LPARAM)msgData); // CAN 전체 수신 완료후 전달.
					else {   // 2개 데이터 수신경우 분리하여 메시지 전달
						int total_cnt = 0;
						//d_msg("parserCnt %d\n", parserCnt);
						for (int i = 0; i < parserCnt; i++)
						{
							pMain->PostMessageA(WM_USER_CAN_RCV_ALL_MSG, (WPARAM)msgHeadParser[i], (LPARAM)&msgData[total_cnt]);
							total_cnt += msgHeadParser[i][1];
							//d_msg("parser send[%d] %x %d %d\n", i, msgHeadParser[i][0], msgHeadParser[i][1], total_cnt);
						}
						msgHead[1] = msgHead[1] - total_cnt;
						pMain->PostMessageA(WM_USER_CAN_RCV_ALL_MSG, (WPARAM)msgHead, (LPARAM)&msgData[total_cnt]); // 마지막 데이터 전달.
						//d_msg("parser end send  %x %d \n", msgHead[0], msgHead[1]);
					}
#endif
					rcvCnt = 0;
					parserCnt = 0;
					parserDataCnt = 0;
				}
			}
		}
#endif
		usleep(CAN_RCV_TIME_THR);
		//Sleep(CAN_RCV_TIME_THR);
	}
	return 0;
}
#endif
UINT ThreadSendCAN_Data(LPVOID param)
{
	CDisChgTesterDlg* pMain = (CDisChgTesterDlg*)param;
	while (m_isSendCANThread)
	{
		if (!openSuccess) return -1;
		pMain->SendMessage(WM_USER_CAN_SEND_MSG, NULL, NULL);
		Sleep(CAN_SEND_TIME_THR);
	}
	return 0;
}

UINT ThreadUpdateFW(LPVOID param)
{
	//CCAN_TestDlg *pDlg = (CCAN_TestDlg *)ptr;
	CDisChgTesterDlg* pDlg = (CDisChgTesterDlg*)param;
	FILE* Afh = NULL;
	int len = 0, ext = 0, rtr = 0;
	unsigned int txCount = 0;
	long idRx = 0, bdLast = 0;
	char cbuffer[32 + 1] = { 0 };
	unsigned char sendData[8] = { 0 }, rcvBuf[8] = { 0 };
	unsigned char* pFileBuff = 0;
	uint32_t msgId = 0, AppCRC = 0, AppLength = 0, AppStartAddress = 0;// 0x00086000;
	errno_t error = 0;
	CString t = 0;
	int keyCnt = 0;
	char tempChar = 0, offsetAddrStr[5] = { 0 };;
	unsigned int* startAddrArr, * startAddrArrIdx, * endAddrArrIdx, startAddrNum = 0, offsetAddr = 0, apl = 0, tempCnt = 0;
	unsigned short offsetAddrIdx = 0;
	bool appLengthCntEnable = 0, offsetAddrGetEnable = 0;
	unsigned char idList[8] = { 0x11, 0x12, 0x13, 0x14, 0x21, 0x22, 0x23, 0x24 }; // 싸이클러 그룹타입.
	unsigned char idListSc[12] = { 0x11, 0x12, 0x21, 0x22, 0x31, 0x32, 0x41, 0x42, 0x51, 0x52, 0x61, 0x62 }; // 직렬형 32채널 ID...
	unsigned char idTx = 0;
	int sendDely = 700; // 250kbps 기본값으로 적용.
	static int sendCnt = 0;
	int chkId = 0;
	const int CanStd = 0;
	const int CanExt = 1;

	//idList[8] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };  // 일반타입.
	idTx = pDlg->GetDevID(1);
	if (dev1Set.type == _74V_80A_TYPE) {
		for (int i = 0; i < 8; i++) idList[i] = i + 1;
	}
	else {
		for (int i = 0; i < 8; i++) {
			if (idTx == idList[i]) chkId++;
		}
	}
	// 설정된 ID가 기본 지정된 id와 다르면 마지막에 해당 ID 추가함.		
	if (!chkId) idList[7] = idTx;
	// 병렬 설정시 마지막에 그룹 ID 적용.
	if (dev1Set.parallelEn && idTx >= 0x10) {
		idList[7] = (idTx & 0xF0);
	}

	//if (CanBps == 250) sendDely = 700;  //CAN 250kbps시 700, 750, 800 지연 500회 기준 2번 실패 동일함. 그 이하는 실패 많음.
	error = _wfopen_s(&Afh, CStringW(pDlg->strUpdateFilePath), L"rb");
	while (m_isUpdateFW_DataThread)
	{

		if (!Afh)
		{
			t.Format(_T("File Open error"));
			pDlg->SetInfoMsg(t);
			return 0;
		}

		do
		{
			tempChar = fgetc(Afh);

			if (tempChar == '$')
			{
				appLengthCntEnable = 0;
				startAddrNum++;
			}

			if (appLengthCntEnable)
				if (isdigit(tempChar) || isalpha(tempChar))
					apl++;

			if (offsetAddrGetEnable)
				if (isdigit(tempChar) || isalpha(tempChar))
				{
					offsetAddrStr[offsetAddrIdx] = tempChar;
					offsetAddrIdx++;
				}

			if (tempChar == 'A' && !appLengthCntEnable)
				offsetAddrGetEnable = 1;

			if (tempChar == ',')
			{
				offsetAddr = strtol(offsetAddrStr, NULL, 16);
				if (AppStartAddress == 0) {
					AppStartAddress = offsetAddr;
					//d_msg("AppStartAddress %X\n", AppStartAddress);
					t.Format("AppStartAddress %X\n", AppStartAddress);
					pDlg->SetInfoMsg(t);
				}
				if (apl)
					apl = (offsetAddr - AppStartAddress) * 4;

				appLengthCntEnable = 1;
				offsetAddrGetEnable = 0;
				offsetAddrIdx = 0;
			}
		} while (tempChar != EOF);

		appLengthCntEnable = 0;
		offsetAddrGetEnable = 0;
		offsetAddrIdx = 0;

		AppLength = apl / 2;
		//d_msg("AppLength %d(%X), offsetAddr %d(%X), AppStartAddress %d(%X)\n", AppLength, AppLength, offsetAddr, offsetAddr, AppStartAddress, AppStartAddress);
		apl = 0;

		pFileBuff = new unsigned char[AppLength];
		startAddrArr = new unsigned int[startAddrNum];
		startAddrArrIdx = new unsigned int[startAddrNum];
		endAddrArrIdx = new unsigned int[startAddrNum];

		fseek(Afh, 0L, SEEK_SET);

		do
		{
			tempChar = fgetc(Afh);

			if (tempChar == '$')
			{
				appLengthCntEnable = 0;

				if (tempCnt)
					endAddrArrIdx[tempCnt - 1] = apl / 2;
			}

			if (appLengthCntEnable)
			{
				if (isdigit(tempChar) || isalpha(tempChar))
				{
					if (isdigit(tempChar))
						tempChar -= '0';

					if (isalpha(tempChar))
						tempChar = tempChar - 'A' + 0xA;

					if (!(apl % 2))
						pFileBuff[apl / 2] = (tempChar << 4);
					else
						pFileBuff[apl / 2] |= tempChar;

					apl++;
				}
			}

			if (offsetAddrGetEnable)
				if (isdigit(tempChar) || isalpha(tempChar))
				{
					offsetAddrStr[offsetAddrIdx] = tempChar;
					offsetAddrIdx++;
				}

			if (tempChar == 'A' && !appLengthCntEnable)
				offsetAddrGetEnable = 1;

			if (tempChar == ',')
			{
				offsetAddr = strtol(offsetAddrStr, NULL, 16);
				startAddrArr[tempCnt] = offsetAddr;

				if (apl)
				{
					for (; apl < ((offsetAddr - AppStartAddress) * 4); apl++)
						if (!(apl % 2))
							pFileBuff[apl / 2] = (0xF << 4);
						else
							pFileBuff[apl / 2] |= 0xF;
				}

				startAddrArrIdx[tempCnt] = apl / 2;
				tempCnt++;

				appLengthCntEnable = 1;
				offsetAddrGetEnable = 0;
				offsetAddrIdx = 0;
			}
		} while (tempChar != EOF);

		endAddrArrIdx[tempCnt - 1] = AppLength;

		//crc32 계산
		AppCRC = crc32_fdl(0, (const uint8_t*)pFileBuff, AppLength);

		idTx = pDlg->GetDevID(1);
		bdLast = 32;// pDlg->GetDevID(2);
		t.Format("ID %d [%s] 업데이트 시작!", idTx, pDlg->strUpdateFileName);
		pDlg->SetInfoMsg(t);

		// 펌웨어 키 구버전 4byte / 신버전 8byte key
		keyCnt = 4;
		sendData[3] = 0x12;
		sendData[2] = 0xC3;
		sendData[1] = 0x72;
		sendData[0] = 0x5A;

		int sendlen = 8;
		unsigned char sendid[20] = { 0, };
		memcpy(sendid, idList, sendlen);
		// if (CAN_SetConfig(hBus, 500, 0, 0)); else AfxMessageBox("CAN2USB 장치를 설정할 수 없습니다."); Sleep(100);
		if (dev1Set.type == _160V_60A_32CH_TYPE) {
			sendlen = 12;				
			memcpy(sendid, idListSc, sendlen);
		}
		//CAN 통신 잡음을 없애기 위해서 전 체널을 교정 모드로 설정
		for (int i = 0; i < sendlen; i++)
		{
			//0x5E = CAN_CMD_CAL_START
			//if (CAN_Send(hBus, (0x5E << 12) + (idList[i] << 20), 0, 0, 1, 0) == 0)
			if (pDlg->SendCAN_DataEx((0x5E << 12) + (sendid[i] << 20), 0, 0, CanExt) == 0)
			{
				t.Format(_T("CAN 통신 에러!!!"));
				pDlg->SetInfoMsg(t);
				//pDlg->SetDlgItemTextA(IDC_STATIC_MSG, t);
				return 0;
			}

			Sleep(1);
		}

		//Boot loader 진입
		for (int i = 0; i < sendlen; i++)
		{
			//0x7F = FW update cmd
			t.Format(_T("Bd 0x%x reset"), sendid[i]);
			pDlg->SetInfoMsg(t);
			//CAN_Send(hBus, (0x7F << 12) + (idList[i] << 20), keyCnt, (char*)sendData, 1, 0);
			pDlg->SendCAN_DataEx((0x7F << 12) + (sendid[i] << 20), keyCnt, (char*)sendData, CanExt);

			Sleep(1);
		}

		Sleep(2000);	//Boot loader 실행 대기

		while (CAN_Recv(hBus, &idRx, &len, (char*)rcvBuf, &ext, &rtr))
		{
		}

		//msgId = 1, Flash erase KEY 0x04798682
		msgId = 1;
		sendData[0] = 0x82;
		sendData[1] = 0x86;
		sendData[2] = 0x79;
		sendData[3] = 0x04;

		//flash memory erase
		//CAN_Send(hBus, msgId, 4, (char*)sendData, 0, 0);
		pDlg->SendCAN_DataEx(msgId, 4, (char*)sendData, CanStd);
		t.Format(_T("Erase Flash Memory"));
		pDlg->SetInfoMsg(t);
		Sleep(1000);	//flash 지우는 동안 대기, 75msec 소요.

		/*//msgId = 2, AppStartAddress
		msgId = 2;
		sendData[0] = (AppStartAddress >> 0) & 0xFF;
		sendData[1] = (AppStartAddress >> 8) & 0xFF;
		sendData[2] = (AppStartAddress >> 16) & 0xFF;
		sendData[3] = (AppStartAddress >> 24) & 0xFF;

		//send AppStartAddress
		CAN_Send(hBus, msgId, 4, (char *)sendData, 0, 0);
		Sleep(5);*/

		//msgId = 3, AppLength, AppCRC 전송
		msgId = 3;
		sendData[0] = (AppLength >> 0) & 0xFF;
		sendData[1] = (AppLength >> 8) & 0xFF;
		sendData[2] = (AppLength >> 16) & 0xFF;
		sendData[3] = (AppLength >> 24) & 0xFF;

		sendData[4] = (AppCRC >> 0) & 0xFF;
		sendData[5] = (AppCRC >> 8) & 0xFF;
		sendData[6] = (AppCRC >> 16) & 0xFF;
		sendData[7] = (AppCRC >> 24) & 0xFF;

		//send App length, crc
		//CAN_Send(hBus, msgId, 8, (char*)sendData, 0, 0);
		pDlg->SendCAN_DataEx(msgId, 8, (char*)sendData, CanStd);
		Sleep(1);
		t.Format(_T("Download App. delay %d"), sendDely);
		pDlg->SetInfoMsg(t);

		txCount = 0;
		//d_msg("AppLength %d(%X), AppCRC %d(%X), AppStartAddress %d(%X)\n", AppLength, AppLength, AppCRC, AppCRC, AppStartAddress, AppStartAddress);
		t.Format("Len %d(%X), CRC %d(%X), StartAddr %d(%X)\n", AppLength, AppLength, AppCRC, AppCRC, AppStartAddress, AppStartAddress);
		pDlg->SetInfoMsg(t);
		for (unsigned int i = 0; i < startAddrNum; i++)
		{
			msgId = 2;
			sendData[0] = (startAddrArr[i] >> 0) & 0xFF;
			sendData[1] = (startAddrArr[i] >> 8) & 0xFF;
			sendData[2] = (startAddrArr[i] >> 16) & 0xFF;
			sendData[3] = (startAddrArr[i] >> 24) & 0xFF;

			//send AppStartAddress
			//CAN_Send(hBus, msgId, 4, (char*)sendData, 0, 0);
			pDlg->SendCAN_DataEx(msgId, 4, (char*)sendData, CanStd);
			Sleep(1);

			if (i)
				txCount = startAddrArrIdx[i];

			do
			{
				len = endAddrArrIdx[i] - txCount;

				if (len > 8)
					len = 8;

				for (int j = 0; j < len; j++)
				{
					sendData[j] = pFileBuff[txCount++];
				}

				msgId = 4;
				//CAN_Send(hBus, msgId, len, (char*)sendData, 0, 0);
				pDlg->SendCAN_DataEx(msgId, len, (char*)sendData, CanStd);
				//Sleep(1);
				//std::this_thread::sleep_for(std::chrono::microseconds(500));
				//usleep(1000);
				usleep(sendDely);		//170usec 이상 부터 정상 동작(500까지동작)  250kbps시 350으로 조정..
				/*	if ((txCount % 0x1000) == 0) {
						sendCnt++;
						t.Format(_T("Send Data %d"), sendCnt);
						pDlg->SetInfoMsg(t);
					}*/
			} while (txCount < endAddrArrIdx[i]);
		}

		t.Format(_T("Wait App. Loading"));
		pDlg->SetInfoMsg(t);
		Sleep(2000);

		//CAN_CMD_CAL_END, ack 비우기.
		while (CAN_Recv(hBus, &idRx, &len, (char*)rcvBuf, &ext, &rtr))
		{
		}


		//bNormalMode = true;
		fclose(Afh);
		delete pFileBuff;
		sendCnt = 0;


		pDlg->SetWindowUI(DEV_ALL, 0);
		pDlg->SetWindowUI(DEV_ALL, 21);

		//pDlg->KillThred(1);
		m_isUpdateFW_DataThread = false;
		WaitForSingleObject(ThreadUpdateFW, 150);
		//Sleep(10);		
		pDlg->m_isUpdateFW = false;
		if (FwCnt) {
			FwCnt--;
		}
		else pDlg->OnBnClickedBtnT1();
		t.Format(_T("Download Complete %d"), FwCnt);
		pDlg->SetInfoMsg(t);
	}
	return 0;
}

UINT ThreadUpdateFW_Stm32(LPVOID param)
{
	CDisChgTesterDlg* pDlg = (CDisChgTesterDlg*)param;
	FILE* Afh;
	int len = 0, ext, rtr, txCount = 0, AppLength;
	long idTx, idRx, bdLast;
	//char cbuffer[32 + 1];
	unsigned char sendData[8] = { 0 }, rcvBuf[8] = { 0 };
	unsigned char* pFileBuff;
	uint32_t msgId, AppCRC, AppStartAddress = 0x08004000;
	errno_t error;
	CString t;
	static int sendCnt = 0, testCnt = 0;
	const int CanStd = 0;
	const int CanExt = 1;

	error = _wfopen_s(&Afh, CStringW(pDlg->strUpdateFilePath), L"rb");

	if (!Afh)
	{
		t.Format(_T("File Open error"));
		pDlg->SetInfoMsg(t);
		return -1;
	}

	idTx = pDlg->GetDevID(1);
	bdLast = pDlg->GetDevID(2);

	if (dev1Set.type == _5V_20A_PACK_TYPE) {
		if (dev1Set.power != 0)  bdLast = dev1Set.power / 1000;
		else bdLast = 8;
	}

	//d_msg("fw %d %d\n", idTx, bdLast);
	if (bdLast < idTx) {
		pDlg->SetInfoMsg("업데이트 시작 ID 확인!");
		return -1;
	}
	if (bdLast == 0 || idTx == 0) {
		pDlg->SetInfoMsg("ID 0 확인!");
		return -1;
	}

	//CAN 통신 잡음을 없애기 위해서 전 체널을 교정 모드로 설정
	t.Format("ID %d-%d [%s] 업데이트 시작!", idTx, bdLast, pDlg->strUpdateFileName);
	pDlg->SetInfoMsg(t);
	for (int i = 1; i < 32; i++)
	{
		//0x5E = CAN_CMD_CAL_START
		//if (CAN_Send(hBus, (0x5E << 12) + (i << 20), 0, 0, 1, 0) == 0)
		if (pDlg->SendCAN_DataEx((0x5E << 12) + (i << 20), 0, 0, CanExt) == 0)
		{
			t.Format(_T("CAN 통신 에러!!!"));
			pDlg->SetInfoMsg(t);
			return -1;
		}
		Sleep(10);
	}

	fseek(Afh, 0L, SEEK_END);
	AppLength = ftell(Afh);
	fseek(Afh, 0L, SEEK_SET);

	//allocating file read buffer
	pFileBuff = new unsigned char[AppLength];

	//read file data
	fread(pFileBuff, 1, AppLength, Afh);

	//crc32 계산
	AppCRC = crc32_fdl(0, (const uint8_t*)pFileBuff, AppLength);

	do
	{
		error = 0;
		t.Format(_T("보드 %d 재시작.."), idTx);
		pDlg->SetInfoMsg(t);
		sendData[3] = 0x12;			sendData[2] = 0xC3;			sendData[1] = 0x72;			sendData[0] = 0x5A;

		//CAN_Send(hBus, (0x7F << 12) + (idTx << 20), 4, (char*)sendData, 1, 0);		
		pDlg->SendCAN_DataEx((0x7F << 12) + (idTx << 20), 4, (char*)sendData, CanExt);
		Sleep(10);
		idTx++;
	} while (idTx <= bdLast);

	Sleep(3000);	//Boot loader 실행 대기

	while (CAN_Recv(hBus, &idRx, &len, (char*)rcvBuf, &ext, &rtr))
	{
	}

	//msgId = 1, Flash erase KEY 0x04798682
	msgId = 1;
	sendData[0] = 0x82;
	sendData[1] = 0x86;
	sendData[2] = 0x79;
	sendData[3] = 0x04;

	//flash memory erase
	//CAN_Send(hBus, msgId, 4, (char*)sendData, 0, 0);
	pDlg->SendCAN_DataEx(msgId, 4, (char*)sendData, CanStd);

	pDlg->SetInfoMsg("플래쉬 메모리 삭제..");
	Sleep(5000);	//flash 지우는 동안 대기, 2.6sec

	//msgId = 2, AppStartAddress
	msgId = 2;
	sendData[0] = (AppStartAddress >> 0) & 0xFF;
	sendData[1] = (AppStartAddress >> 8) & 0xFF;
	sendData[2] = (AppStartAddress >> 16) & 0xFF;
	sendData[3] = (AppStartAddress >> 24) & 0xFF;

	//send AppStartAddress
	//CAN_Send(hBus, msgId, 4, (char*)sendData, 0, 0);
	pDlg->SendCAN_DataEx(msgId, 4, (char*)sendData, CanStd);
	Sleep(5);

	//msgId = 3, AppLength, AppCRC 전송
	msgId = 3;
	sendData[0] = (AppLength >> 0) & 0xFF;
	sendData[1] = (AppLength >> 8) & 0xFF;
	sendData[2] = (AppLength >> 16) & 0xFF;
	sendData[3] = (AppLength >> 24) & 0xFF;

	sendData[4] = (AppCRC >> 0) & 0xFF;
	sendData[5] = (AppCRC >> 8) & 0xFF;
	sendData[6] = (AppCRC >> 16) & 0xFF;
	sendData[7] = (AppCRC >> 24) & 0xFF;

	//send App length, crc
	//CAN_Send(hBus, msgId, 8, (char*)sendData, 0, 0);
	pDlg->SendCAN_DataEx(msgId, 8, (char*)sendData, CanStd);
	Sleep(5);

	pDlg->SetInfoMsg("파일 다운로드..");

	txCount = 0;
	msgId = 4;
	do
	{
		len = AppLength - txCount;
		if (len > 8)
			len = 8;

		for (int i = 0; i < len; i++)
		{
			sendData[i] = pFileBuff[txCount++];
		}

		//CAN_Send(hBus, msgId, len, (char*)sendData, 0, 0);
		pDlg->SendCAN_DataEx(msgId, len, (char*)sendData, CanStd);

		if ((txCount % 0x800) == 0) {
			Sleep(50);		//23msec
			sendCnt++;
			t.Format("%d / %d", sendCnt, AppLength / 0x800);
			pDlg->SetInfoMsg(t);
		}
		else	usleep(300);

	} while (txCount < AppLength);

	pDlg->SetInfoMsg("보드 재시작");
	Sleep(3000);

	for (int i = 1; i < 65; i++)
	{
		//0x5E = CAN_CMD_CAL_END
		//if (CAN_Send(hBus, (0x5F << 12) + (i << 20), 0, 0, 1, 0) == 0)
		pDlg->SendCAN_DataEx((0x5F << 12) + (i << 20), 0, 0, CanExt);
		/*{
			pDlg->SetInfoMsg("CAN 통신 에러!!!");
			return -1;
		}*/
		Sleep(5);
	}

	//CAN_CMD_CAL_END, ack 비우기.
	while (CAN_Recv(hBus, &idRx, &len, (char*)rcvBuf, &ext, &rtr))
	{
		Sleep(1);
	}
	idTx = pDlg->GetDevID(1);
	//bdLast = pDlg->GetDevID(2);

	t.Format("ID %d-%d [%s] 업데이트 완료!", idTx, bdLast, pDlg->strUpdateFileName);
	pDlg->SetInfoMsg(t);

	//bNormalMode = true;
	fclose(Afh);
	delete pFileBuff;
	sendCnt = 0;
	pDlg->SetWindowUI(DEV_ALL, 0);
	pDlg->SetWindowUI(DEV_ALL, 21);
	pDlg->m_isUpdateFW = false;

	return 1;
}

CString GetTimeStamp(uint8_t type)
{
	static uint32_t cnt = 0;
	CString strTime;
	SYSTEMTIME currentTime;
	GetLocalTime(&currentTime);
	if (type == 0)			strTime.Format("%d [%02d:%02d:%02d.%03d] ", cnt++, currentTime.wHour, currentTime.wMinute, currentTime.wSecond, currentTime.wMilliseconds);
	else if (type == 1)		strTime.Format("%02d-%02d_%02d-%02d-%02d", currentTime.wMonth, currentTime.wDay, currentTime.wHour, currentTime.wMinute, currentTime.wSecond);
	else if (type == 3)		strTime.Format("[%02d:%02d:%02d] ", currentTime.wHour, currentTime.wMinute, currentTime.wSecond);
	else if (type == 4)		strTime.Format("[%02d:%02d:%02d.%03d] ", currentTime.wHour, currentTime.wMinute, currentTime.wSecond, currentTime.wMilliseconds);
	return strTime;
}

void CDisChgTesterDlg::OnBnClickedBtnT3()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.		
	if (dev1Set.type == _TEST_DUTY_TYPE) {
		Setgpio = dev1Set.getGpio2;
		Setgpio ^= 1;
		dev1Set.txCanData[0] = 0x32;
		dev1Set.txCanData[1] = Setgpio;
		dev1Set.CanCmd = CAN_TEST_MODE_CMD;
		dev1Set.CanSenLen = 3;
		sendBtnCanTimer = 3;
		return;
	}
	GetDevSetData(DEV_1, 0);
	SendControlCmd(dev1Set.id, CAN_SET_CONTACT_CMD);
	int conTime = (contactSet.time + contactSet.recoverTime) / 1000;
	if (conTime < 0) conTime = 1;
	dev1Set.sendTimerCnt = conTime + 3;
	dev1Set.sendCmd = CAN_CHK_CONTACT_CMD;
}

LRESULT CDisChgTesterDlg::OnUpdateRcvData(WPARAM wParam, LPARAM lParam)
{
	// TODO: 여기에 구현 코드 추가.
	long* wp = (long*)wParam;
	char* lp = (char*)lParam;
	static long cnt = fFwMaxCnt;
	static int16_t temperature = 10000;
	CString updateCh1Data, updateChD2ata, updateData;
	static struct Exid canid = { 0, };
	canid.all = wp[0];
	int rcvLen = wp[1];
	float fvolt[4] = { 0.0f, }, fcap[4] = { 0.0f, }, fcur[4] = { 0.0f, }, fwatH = 0.0f;
	int32_t volt[4] = { 0, }, cap[4] = { 0, };
	int32_t cur[4] = { 0, };
	int dstIdx = 0;
	static float ch1BfVolt[4] = { 0.0f, }, ch2BfVolt[4] = { 0.0f, };
	static int autoTempCnt = 0;
	CString tempStr;
	CString msg;
	
	
	static int32_t bfTime = 0;
	const uint8_t sendId_1[4] = { 0x01, 0x02, 0x03, 0x04 };
	const uint8_t sendId_2[4] = { 0x05, 0x06, 0x07, 0x08 };

	// pack data
	static uint16_t P_battState, P_soc, P_volt, P_cur, P_temp, P_chgVolt, P_chgCur, P_cycCnt, P_fullCap;
	static uint16_t P_userData[20];
	static float kelvin = 0.0f, P_degC = 0.0f;
	static uint16_t cableVolt = 0, voutVolt = 0;
	bool rcvGrp = false;	// 그룹 ID로 수신 여부 확인.
	CString  GrpData1, GrpData2;
	//static bool savePackdata = false;

	if (!dev1Set.start) {
		m_ComboCh1_ID.GetWindowText(tempStr);
		dev1Set.id = _ttoi(tempStr);
	}

	//d_msg("gprid %x %x \n", dev1Set.grpId, (canid.id & 0xF0));
	if (dev1Set.id == canid.id) {
		msg.Format("%s OnUpdateRcvData ", GetTimeStamp(4));
		d_msg("%s", msg);
		canMsgLog.AppendFormat("%s", msg);
		//dev1Set.rcvChkTimer = 5;
	}
	else {
		//d_msg("%s OnUpdateRcvData id %d return\n", GetTimeStamp(4), canid.id);

		if (dev1Set.rcvChkTimer && m_isUpdateFW == false) {
			dev1Set.rcvChkTimer--;
			tempStr.Format("설정 ID %d[0x%X] 로 변경필요! Cmd %X ", canid.id, canid.id, canid.cmd);
			SetInfoMsg(tempStr);
		}

		// 20A는 진행..Can time out 문제로..retrun 미처리.
		if (dev1Set.type != _5V_20A_PACK_TYPE) {
			// 200A 싸이클러 그룹 수신 처리.
			if ((dev1Set.setCur == _SET_1KA || dev1Set.setCur == _SET_2KA) && dev1Set.grpId == (canid.id & 0xF0)) {
				msg.Format("%s UpdateRcvDataGp ", GetTimeStamp(4));
				d_msg("%s", msg);
				canMsgLog.AppendFormat("%s", msg);
				rcvGrp = true;
			}
			else {
				return 0;	// 해당되지 않는 id 필터링..
			}
		}
		else {

		}
	}

	msg.Format("id %X Cmd %X cnt %d data => ", canid.all, canid.cmd, rcvLen);
	for (int i = 0; i < rcvLen; i++) {
		msg.AppendFormat(" %2.2X", (uint8_t)lp[i]);
	}
	canMsgLog.AppendFormat("%s\n", msg);
#ifdef _DEBUG	
	d_msg("%s\n", msg);
#endif
	int idxId = canid.id & 0x0F;

	if (canid.cmd == CAN_READ_DATA_CMD + 0x80)
	{
		uint8_t opMode = (uint8_t)lp[36];
		uint8_t battState[4], faultCode[4];
		CString updateVolt, updateCur, updateCap, updateSt, updateErr, updateMode, updateWattH, updateTime, updateEnd, updateStep, updateTemp;
		CString strVolt, strCur, strCap, strCnt, strTemp, strBatState, strTime, strWattH, strEnd, strErr, strSt, strStep, msg, strCapVolt;
		CString strBmsVolt, strBmsState, strBmsTemp, strBmsDeltaV;
		static uint8_t bfBattState[4] = { 0 };
		CString batSt;
		uint8_t stepNum = 0;
		int32_t Time = 0, wattH = 0, capVolt = 0;
		int32_t discap = 0, discapW = 0;
		static uint8_t TogSend = 0;
		static uint8_t stopCnt = 4;
		
		// 160V60A 32ch
		uint8_t bmsState[MAX_BMS_CH] = { 0, };
		int16_t bmsTemp[MAX_BMS_CH] = { 0, };
		uint16_t bmsVolt[MAX_BMS_CH] = { 0, };		 // 실제 수신된 전압
		static int32_t bmsVoltOffset[MAX_BMS_CH] = { 0, };  // -1000mV offset 처리 
		int32_t bmsDeltaVolt[MAX_BMS_CH] = { 0, };

		// 24 일반싸이클러, 26 직렬형, 28 모듈형충방전기, 싸이클러 패턴 경우 32
		if (rcvLen == 24 || rcvLen == 26 || rcvLen == 28 || rcvLen == 32 || rcvLen == 160)   // Cycler 직렬형에 온도 추가되어 26도 있음.
		{
			opMode = (uint8_t)lp[0];
			stepNum = (uint8_t)lp[1];
			battState[0] = (uint8_t)lp[2];
			faultCode[0] = (uint8_t)lp[3];
			memcpy(&Time, &lp[4], sizeof(int32_t));
			memcpy(&volt[0], &lp[8], sizeof(int32_t));
			memcpy(&cur[0], &lp[12], sizeof(int32_t));
			memcpy(&cap[0], &lp[16], sizeof(uint32_t));
			memcpy(&wattH, &lp[20], sizeof(uint32_t));

			if (rcvLen == 32) {
				//memcpy(&capVolt, &lp[28], sizeof(int32_t));		
				//d_msg("capVolt %d\n", capVolt);
				if (dev1Set.type == _5V_200A_TYPE && dev1Set.opCmd == DEV_PATTERN_MODE)
				{
					// 방전 경우, 방전 누적값으로 변경 표시.
					if (cur[0] < 0) {
						memcpy(&discap, &lp[24], sizeof(int32_t));
						memcpy(&discapW, &lp[28], sizeof(int32_t));
						cap[0] = discap;
						wattH = discapW;
					}
				}
			}

			if (rcvLen == 26 && dev1Set.type == _74V_80A_TYPE) 	memcpy(&temperature, &lp[24], sizeof(int16_t));

			if (bfTime != Time && dev1Set.setCur == _SET_200A) // 개별 테스트시만 체크
			{
				int difTime = abs(Time - bfTime);
				// 자체 폴링경우 타이밍 문제로 마진 50% 적용.
				if (dev1Set.type != _74V_80A_TYPE && dev1Set.type != _160V_60A_32CH_TYPE)  // 자동 전송인..싸이클러류만 확인.
				{
					if (dev1Set.msrTime + (dev1Set.msrTime * 0.5) < difTime && difTime < dev1Set.msrTime * 5)  // 시간이 2~4개 빠질 경우...// 마진 20ms 
					{
						if (bfTime != 0) {
							if (adminMode) {
#ifdef DEBUG_MSG_01
								msg.Format("RcvTime Err bf %d now %d ", bfTime, Time);
								SetInfoMsg(msg);
#endif
							}
							RcvTimeErrCnt++;
							saveCanMsgCnt = 5;  // 에러이후 5초까지 저장.
						}
					}
				}
				bfTime = Time;
			}
			float fvoltV = (float)volt[0] / (MSR_UNIT * 10);
			float fcurA = (float)cur[0] / (MSR_UNIT * 10);
			float dcir = 0.0f;
			float ildeSumVolt = 0.0f;
			float setSumVolt = 0.0f;
			float runSumVolt = 0.0f;
			if (rcvLen == 160 && dev1Set.type == _160V_60A_32CH_TYPE)
			{				
				CString chVolt1 = "";
				CString chVolt2 = "";
				CString chVolt3 = "";
				CString chVolt4 = "";
				CString strVolt = "";
				CString strDeltVolt = "";
				uint16_t crCnt = 0;

				uint16_t bmsMaxTemp = 0;
				uint16_t bmsMaxTempch = 0;

				cap[0] = 0;
				wattH = 0;

				uint16_t rxIdx = 0;
				rxIdx = 16;
				int j = 0;
				for (int i = 0; i < MAX_BMS_CH/2; i++) {					
					bmsState[j++] = ((uint8_t)lp[i + rxIdx] & 0xF0)>>4;
					bmsState[j++] = (uint8_t)lp[i + rxIdx] & 0x0F;
					//d_msg("%d %X %X \n", i, bmsState[2*i], bmsState[2*i+1]);
				}
				memcpy(&bmsVolt[0], &lp[32], sizeof(uint16_t)* MAX_BMS_CH);
				memcpy(&bmsTemp[0], &lp[96], sizeof(int16_t)* MAX_BMS_CH);
				
				
				int idleChCnt = 0;
				int runChCnt = 0;
				uint32_t chkch = dev1Set.activeCh;
				for (int i = 0; i < MAX_BMS_CH; i++) 
				{
					int32_t volt = bmsVolt[i] - 10000;	//offset 처리.					
					//if(i<5)	d_msg("volt %d, %d\n", volt, bmsVolt[i]);
					if( i > 29) chVolt4.AppendFormat("%02d[%.1f] %X   ", i + 1, volt * 0.1f, bmsState[i]);
					else if (i > 19) chVolt3.AppendFormat("%02d[%.1f] %X   ", i + 1, volt * 0.1f, bmsState[i]);
					else if (i > 9) chVolt2.AppendFormat("%02d[%.1f] %X   ", i + 1, volt * 0.1f, bmsState[i]);
					else  chVolt1.AppendFormat("%02d[%.1f] %X   ", i + 1, volt * 0.1f, bmsState[i]);
					//chVolt.AppendFormat("%d[%.1f]%X   ", i+1,bmsVolt[i]*0.1f, bmsState[i]);
					//if (i != 0 && i % 9 == 0)	chVolt.AppendFormat("\r\n");
					if (bmsMaxTemp < bmsTemp[i]) {
						bmsMaxTemp = bmsTemp[i];
						bmsMaxTempch = i + 1;
					}
					if (bmsState[i] == 0x00)  // ilde
					{
						idleChCnt++;
						ildeSumVolt += (volt * 0.1f);
						if(bmsDeltaVolt[i] != 0) bmsDeltaVolt[i] = 0;
					}
					if (bmsState[i] == 0x03)  // run
					{
						runChCnt++;
						runSumVolt += (volt * 0.1f);									
					}					

					if ((chkch >> i) & 0x00000001)
					{	
						setSumVolt += (volt * 0.1f);
					}	

					bmsDeltaVolt[i] = abs(bmsVoltOffset[i] - volt);
					bmsVoltOffset[i] = volt;
				}
				//d_msg("bmsDeltaVolt %d %d\n", bmsDeltaVolt[0], bmsDeltaVolt[1]);

				chVolt4.AppendFormat("MaxTemp Ch%d [%.1fC]", bmsMaxTempch, bmsMaxTemp*0.1);
				if (idleChCnt || runChCnt || dev1Set.activeChCnt) {
					if(fabs(fcurA) > 0)	dcir = fabs((runSumVolt / 1000) - fvoltV) / fcurA;
					strVolt.Format("#Set [%d] %.3fV #Idle [%d] %.3fV #Run [%d] %.3fV #IR %.4fΩ", dev1Set.activeChCnt, (setSumVolt/1000), idleChCnt, (ildeSumVolt /1000), runChCnt, (runSumVolt/1000), dcir);
					chVolt4.Append(strVolt);
					//SetInfoMsg(strVolt);
				}
				GetDlgItem(IDC_STATIC_PACK_ITEM)->SetWindowText(chVolt1);
				GetDlgItem(IDC_STATIC_PACK_DATA)->SetWindowText(chVolt2);
				GetDlgItem(IDC_STATIC_PACK_DATA2)->SetWindowText(chVolt3);
				GetDlgItem(IDC_STATIC_PACK_DATA3)->SetWindowText(chVolt4);
				//if(crCnt) SetInfoMsg(strVolt);
				
			}

			fvolt[0] = (float)(volt[0] * 0.1);
			fcur[0] = (float)(cur[0] * 0.1);
			//fcap[0] = (float)cap[0];
			//fwatH = (float)(wattH);
			//d_msg("op %x, bat %x fault %x\n", opMode, battState[0], faultCode[0]);

			// 메세지 출력 위해 변경.
			uint8_t mMsg = 0;
			mMsg = opMode;
			// faultMsg, 문자열 배열 매칭 처리 위해..
			//if (fMsg > FMSG_IDX) {
			//	if ((uint8_t)fMsg == 0xF0) 	fMsg = FMSG_IDX + 1;   // 스케줄 에러
			//	if ((uint8_t)fMsg == 0x20)	fMsg = FMSG_IDX + 3;	// illegal?
			//	else 						fMsg = FMSG_IDX + 2;	// illegal?
			//}
			if (mMsg > 0x07) {
				if ((uint8_t)mMsg == 0x10) 	mMsg = 0x08;   // contack check
				else 						mMsg = 0x09;	// stop
			}
			//if (MAX_CH == 1) fvolt[1] = fcur[1] = fcap[1] = battState[1] = faultCode[1] = 0;
			//float fvoltV = (float)volt[0] / (MSR_UNIT * 10);
			//float fcurA = (float)cur[0] / (MSR_UNIT * 10);
			updateVolt.Format("[ 전압 V ]\n%.3f ", fvoltV);
			updateCur.Format("[ 전류 A ]\n%.3f ", fcurA);
			updateCap.Format("[ 용량 Ah ]\n %d", cap[0]);
			updateEnd.Format("[ 종료 ]\n %s", batStateMsg[battState[0]]);
			updateErr.Format("[ 에러 ]\n %s", faultCodeMsg(0, faultCode[0]));
			updateSt.Format("[ 상태 ]\n %s", opStateMsg[mMsg]);
			//updateStep.Format("[ Step ]\n %d", stepNum);
			updateWattH.Format("[ W ]\n %.1f ", (fvoltV * fcurA));
			updateTime.Format("[ Time ]\n %d", Time / MSR_UNIT);
			if (temperature != 10000) updateTemp.Format("[ 온도 ]\n %.1f", temperature * 0.1);
			if (faultCode[0] > 0)  // 에러시 정지.
			{
				//if (m_ChkAutoStop.GetCheck())  StopDev(canid.id);
				if (dev1Set.dspErrInfo) {  // 1회만 표시
					CString batErr;
					batErr.Format("ID %d 에러 [%s] 전압[ %.1f] 전류[ %.1f] 용량[ %d] 시간[%d] ", canid.id, faultCodeMsg(0, faultCode[0]), fvolt[0], fcur[0], cap[0], Time / 1000);
					SetInfoMsg(batErr);
					dev1Set.dspErrInfo = false;
				}
				/*updateData.Format("스케줄 에러 경우..설정값 확인 필요!");
				SetInfoMsg(updateData);*/
			}
			else
			{
				// 공정 정상 종료.  최대채널 1개일 경우 추가.
#if 0
				if (battState[0] == BAT_ST_END_CUR || (battState[0] == BAT_ST_END_VOLT) || (battState[0] == BAT_ST_END_TIME)
					|| (battState[0] == BAT_ST_END_CUR && MAX_CH == 1) || (battState[0] == BAT_ST_END_VOLT && MAX_CH == 1))
				{
					if (m_ChkAutoStop.GetCheck()) {
						batSt.Format("시험 종료 [상태  %s ] [에러  %s ]", batStateMsg[battState[0]], faultMsg[faultCode[0]]);
						SetInfoMsg(batSt);
						StopDev(canid.id);
					}
				}
#endif
				

			}
			// 싸이클러류 자동 정지 경우 처리..24.07.20
			// 에러 발생되더라도.. 자동 정지 처리..
			if (m_ChkAutoStop.GetCheck())
			{
#if 0   // 바로 종료
				if (battState[0] > 0x00)	StopDev(canid.id);
#else
				if (battState[0] > 0x00) {
					if (stopCnt) stopCnt--;
					if (stopCnt == 0) {   // 바로 정지하지 않고 2초 대기후 정지.
						batSt.Format("시험 종료 [상태  %s ] [에러  %s ]", batStateMsg[battState[0]], faultCodeMsg(0, faultCode[0]));
						SetInfoMsg(batSt);
						StopDev(canid.id);
					}
				}
				else stopCnt = 3;
#endif

			}
			


			if (dev1Set.id == canid.id)
			{
				//if (Time % 500 == 0 || opMode == 0) // 500ms 마다 갱신..
				if (dev1Set.updateUICnt > UPDATE_UI_CNT)
				{
					GetDlgItem(IDC_STATIC_CH1_VOLT)->SetWindowText(updateVolt);
					GetDlgItem(IDC_STATIC_CH1_CUR)->SetWindowText(updateCur);
					GetDlgItem(IDC_STATIC_CH1_CAP)->SetWindowText(updateCap);
					GetDlgItem(IDC_STATIC_CH1_END)->SetWindowText(updateEnd);
					GetDlgItem(IDC_STATIC_CH1_FAULT)->SetWindowText(updateErr);
					//GetDlgItem(IDC_STATIC_CH1_MODE)->SetWindowText(updateMode);
					GetDlgItem(IDC_STATIC_CH1_WATTH)->SetWindowText(updateWattH);
					GetDlgItem(IDC_STATIC_CH1_TIME)->SetWindowText(updateTime);
					GetDlgItem(IDC_STATIC_CH1_ST)->SetWindowText(updateSt);
					GetDlgItem(IDC_STATIC_CH1_STEP)->SetWindowText(updateStep);
					GetDlgItem(IDC_STATIC_CH1_TEMP)->SetWindowText(updateTemp);
					dev1Set.updateUICnt = 1;
					//d_msg("update \n");
				}
				if (dev1Set.start)
				{
					int listCnt = m_ListCh1.GetItemCount();
					strCnt.Format("%d", listCnt);
					m_ListCh1.InsertItem(listCnt, strCnt);
					m_ListCh1.EnsureVisible(listCnt, FALSE);		// 자동 스크롤 커서
					updateData.Format("%s, %d,", _TEXT(GetTimeStamp(4)), listCnt);
					dev1Set.csvFile.Append(updateData);
					int j = 1;
					for (int i = 0; i < MAX_CH; i++)
					{
						strVolt.Format("%.1f", fvolt[i]);
						strCur.Format("%.1f", fcur[i]);
						strCap.Format("%d", cap[i]);
						strWattH.Format("%d", wattH);
						strTime.Format("%d", Time);
						strEnd.Format("%02X", battState[i]);
						strErr.Format("%02X", faultCode[i]);
						strSt.Format("%02X", opMode);
						strStep.Format("%d", stepNum);
						strTemp.Format("%.1f", temperature * 0.1);
												
						m_ListCh1.SetItemText(listCnt, j++, strStep);
						m_ListCh1.SetItemText(listCnt, j++, strSt);
						m_ListCh1.SetItemText(listCnt, j++, strVolt);
						m_ListCh1.SetItemText(listCnt, j++, strCur);
						m_ListCh1.SetItemText(listCnt, j++, strCap);
						m_ListCh1.SetItemText(listCnt, j++, strWattH);
						m_ListCh1.SetItemText(listCnt, j++, strTime);
						m_ListCh1.SetItemText(listCnt, j++, strEnd);
						m_ListCh1.SetItemText(listCnt, j++, strErr);

						if (rcvLen == 26 && dev1Set.type == _74V_80A_TYPE) {
							m_ListCh1.SetItemText(listCnt, j++, strTemp);
						}
						else if (temperature != 10000)
						{
							m_ListCh1.SetItemText(listCnt, j++, strTemp);
						}

						if (rcvLen == 32 && dev1Set.type == _100V_500A_TYPE) {
							/*strCapVolt.Format("%d", capVolt);
							d_msg("%s\n", strCapVolt);*/
							//m_ListCh1.SetItemText(listCnt, j++, strCapVolt);
						}
						dev1Set.csvFile.Append(strStep); dev1Set.csvFile.Append(",");
						dev1Set.csvFile.Append(strSt); dev1Set.csvFile.Append(",");
						dev1Set.csvFile.Append(strVolt); dev1Set.csvFile.Append(",");
						dev1Set.csvFile.Append(strCur); dev1Set.csvFile.Append(",");						
						dev1Set.csvFile.Append(strCap); dev1Set.csvFile.Append(",");
						dev1Set.csvFile.Append(strWattH);	dev1Set.csvFile.Append(",");
						dev1Set.csvFile.Append(strTime); dev1Set.csvFile.Append(",");
						dev1Set.csvFile.Append(strEnd); dev1Set.csvFile.Append(",");
						dev1Set.csvFile.Append(strErr);	dev1Set.csvFile.Append(",");
						if (dev1Set.type == _160V_60A_32CH_TYPE) {
							strTemp.Format("%.4f", dcir);	
							m_ListCh1.SetItemText(listCnt, 10, strTemp);
							dev1Set.csvFile.Append(strTemp);	dev1Set.csvFile.Append(",,");							
							for (int j = 0; j < MAX_BMS_CH; j++) {

								strBmsVolt.AppendFormat("%.1f,", bmsVoltOffset[j] * 0.1);
								strBmsState.AppendFormat("%02X,", bmsState[j]);	
								strBmsTemp.AppendFormat("%.1f,", bmsTemp[j]*0.1);
								strBmsDeltaV.AppendFormat("%.1f,", bmsDeltaVolt[j] * 0.1);
							}
							dev1Set.csvFile.Append(strBmsState);
							dev1Set.csvFile.Append(",");
							dev1Set.csvFile.Append(strBmsVolt);										
							strTemp.Format("%.1f,", runSumVolt);
							dev1Set.csvFile.Append(strTemp);
							dev1Set.csvFile.Append(strBmsDeltaV);
							dev1Set.csvFile.Append(",");																				
							dev1Set.csvFile.Append(strBmsTemp);
						}
						else {
							if (temperature != 10000) {
								dev1Set.csvFile.Append(strTemp); dev1Set.csvFile.Append(",");
							}
							else {
								dev1Set.csvFile.Append("0"); dev1Set.csvFile.Append(",");
							}
						}
						// 자동 전송 처리로 배터리 데이터 수신될 경우로 조건 변경
						if (dev1Set.type == _5V_20A_PACK_TYPE && dev1Set.PackDataSave && dev1Set.PackDataType == 0)
						{
							strSt.Format("0x%04X", P_battState);
							strStep.Format("%d", P_soc);
							strVolt.Format("%d", P_volt);
							strCur.Format("%d", (int16_t)P_cur);
							strTemp.Format("%.1f", P_degC);

							dev1Set.csvFile.Append(strSt);		dev1Set.csvFile.Append(",");
							dev1Set.csvFile.Append(strStep);	dev1Set.csvFile.Append(",");
							dev1Set.csvFile.Append(strVolt);	dev1Set.csvFile.Append(",");
							dev1Set.csvFile.Append(strCur);		dev1Set.csvFile.Append(",");
							dev1Set.csvFile.Append(strTemp);	dev1Set.csvFile.Append(",");

							dev1Set.PackDataSave = false;
						}

						if (rcvLen == 32 && dev1Set.type == _100V_500A_TYPE) {
							if (temperature == 10000)	dev1Set.csvFile.Append(",");
							strCapVolt.Format("%d", capVolt);
							dev1Set.csvFile.Append(strCapVolt); dev1Set.csvFile.Append(",");
						}
#if 0
						if (dev1Set.type == _5V_200A_TYPE && dev1Set.setCur != _SET_200A) {
							for (int i = 1; i < 11; i++) {
								strVolt.Format("%.1f", rcvMeasure[i].fVoltage);
								strCur.Format("%.1f", rcvMeasure[i].fCurrent);
								strCap.Format("%d", rcvMeasure[i].fCap);
								strWattH.Format("%d", rcvMeasure[i].fWh);
								strTime.Format("%d", rcvMeasure[i].opTime);
								//if (rcvMeasure[i].opTime >= 50) 
								{
									dev1Set.csvFile.Append(strVolt);	dev1Set.csvFile.Append(",");
									dev1Set.csvFile.Append(strCur);		dev1Set.csvFile.Append(",");
									dev1Set.csvFile.Append(strCap);	dev1Set.csvFile.Append(",");
									dev1Set.csvFile.Append(strWattH);		dev1Set.csvFile.Append(",");
									dev1Set.csvFile.Append(strTime);	dev1Set.csvFile.Append(",");
								}
							}
						}
#endif

						dev1Set.csvFile.Append(",");
					}
					dev1Set.csvFile.Append("\n");
				}
				//else savePackdata = false;

				/*	if (bfBattState[0] != battState[0] || bfBattState[1] != battState[1]){
						batSt.Format("제어기 ID %d 상태 1[ %s, %.1fmV %.1fmA] 2[ %s %.1fmV %.1fmA]", dev1Set.id, batStateMsg[battState[0]], fvolt[0], fcur[0], batStateMsg[battState[1]], fvolt[1], fcur[1]);
						SetInfoMsg(batSt);
						bfBattState[0] = battState[0];
						bfBattState[1] = battState[1];
					}*/
				dev1Set.autoMsrReq = ((CButton*)GetDlgItem(IDC_CHECK_AUTO_MSR))->GetCheck();  // 실시간 반영하도록..
				dev1Set.autoTempReq = ((CButton*)GetDlgItem(IDC_CHECK_AUTO_SEND))->GetCheck();
				if (dev1Set.opCmd == DEV_PATTERN_MODE)
				{
					if (dev1Set.sendPattenCnt && dev1Set.autoMsrReq) {
						SendControlCmd(canid.id, CAN_PATTERN_UPDATE_CMD);
						dev1Set.sendPattenCnt--;
						if (dev1Set.sendPattenCnt == 0 && dev1Set.start)	OnBnClickedBtnCh1Stop();
					}
				}
				else {
					if (dev1Set.type == _TEST_DUTY_TYPE && dev1Set.autoMsrReq) {
						char txData[8] = { 0, };
						SendCAN_CmdData(canid.id, CAN_SENSING_CHECK_CMD, txData, 0);
					}
				}
				if (dev1Set.autoTempReq)
				{
					// 500ms 이상 1회 500ms 미만은 설정에 따라 500ms로 적용.
					if (dev1Set.msrTime >= 500) autoTempCnt = 0;
					else {
						//if (autoTempCnt++ >= 1000 / dev1Set.msrTime)  autoTempCnt = 0;
						if (autoTempCnt++ >= 500 / dev1Set.msrTime)  autoTempCnt = 0;
					}

					if (autoTempCnt == 0)
					{

						if (dev1Set.type == _100V_500A_TYPE || dev1Set.type == _5V_200A_TYPE || dev1Set.type == _5V_20A_PACK_TYPE)	TogSend++;
						else TogSend = 0;

						if (dev1Set.type == _160V_60A_32CH_TYPE) TogSend = 1;   // 강제 센싱 값 요청.

						if (dev1Set.SensingReq) {
							if (dev1Set.type == _5V_20A_PACK_TYPE) {}	// 사용안함 타이머에서 8채널 번갈아 가면서 요청.
							else {
								if (TogSend % 2 == 0)	SendControlCmd(canid.id, CAN_READ_TEMP_CMD);   // 통신 타임 방지용. 번갈아 가면서 온도, 센싱 요청.
								else					SendControlCmd(canid.id, CAN_SENSING_CHECK_CMD);
							}
						}
						else {
							//if (TogSend % 2 == 0)	
							SendControlCmd(canid.id, CAN_READ_TEMP_CMD);
						}
					}
				}

			}
			else   // 그룹 데이터 표시..
			{

			}

			if (dev1Set.setCur != _SET_200A && dev1Set.type == _5V_200A_TYPE)
			{
				if (idxId <= 10)
				{
					rcvMeasure[idxId].opTime = Time;
					rcvMeasure[idxId].opMode = opMode;
					rcvMeasure[idxId].opModeMsg = opStateMsg[mMsg];
					rcvMeasure[idxId].fVoltage = fvolt[0];
					rcvMeasure[idxId].fCurrent = fcur[0];
					rcvMeasure[idxId].Wh = wattH;
					rcvMeasure[idxId].Cap = cap[0];

					// 전류값 누적처리.
					if (idxId == 1) {
						rcvMeasure[11].opTime = Time;
						rcvMeasure[11].opMode = opMode;
						rcvMeasure[11].opModeMsg = opStateMsg[mMsg];
						rcvMeasure[11].fVoltage = rcvMeasure[idxId].fVoltage;
					}
					rcvMeasure[11].fCurrent += fcur[0];
					rcvMeasure[11].Wh += wattH;
					rcvMeasure[11].Cap += cap[0];

					if (canid.id == dev1Set.lastId)
					{
						for (int i = 1; i <= 11; i++) {
							strSt.Format("%02X,", rcvMeasure[i].opMode);
							strVolt.Format("%.1f,", rcvMeasure[i].fVoltage);
							strCur.Format("%.1f,", rcvMeasure[i].fCurrent);
							strCap.Format("%d,", rcvMeasure[i].Cap);
							strWattH.Format("%d,", rcvMeasure[i].Wh);
							strTime.Format("%d,", rcvMeasure[i].opTime);
							//if (rcvMeasure[i].opTime >= 50) 
							{
								dev1Set.csvGrpFile.Append(strSt);
								dev1Set.csvGrpFile.Append(strTime);
								dev1Set.csvGrpFile.Append(strVolt);
								dev1Set.csvGrpFile.Append(strCur);
								dev1Set.csvGrpFile.Append(strCap);
								dev1Set.csvGrpFile.Append(strWattH);
							}
							if (i == 11) {	// 누적값 초기화.
								rcvMeasure[11].fCurrent = 0;
								rcvMeasure[11].Wh = 0;
								rcvMeasure[11].Cap = 0;
							}
						}
						dev1Set.csvGrpFile.Append("\n");
					}

					if (dev1Set.updateUICnt == 1) {
						GrpData1 = "";
						GrpData2 = "";
						for (int i = 1; i <= 10; i++) {
							if (rcvMeasure[i].opMode == 0 && rcvMeasure[i].fVoltage == 0) {
								rcvMeasure[i].opModeMsg = "NC";
							}
							if (i <= 5)	GrpData1.AppendFormat("[%d %s %.1f / %.1f ]  ", i, rcvMeasure[i].opModeMsg, rcvMeasure[i].fVoltage, rcvMeasure[i].fCurrent / 1000.0f);
							else		GrpData2.AppendFormat("[%d %s %.1f / %.1f ]  ", i, rcvMeasure[i].opModeMsg, rcvMeasure[i].fVoltage, rcvMeasure[i].fCurrent / 1000.0f);
						}
						GetDlgItem(IDC_STATIC_PACK_ITEM)->SetWindowText(GrpData1);
						if (dev1Set.setCur == _SET_2KA) GetDlgItem(IDC_STATIC_PACK_DATA)->SetWindowText(GrpData2);
					}
				}
			}
			msg.Format("%s  %d", GetTimeStamp(3), RcvTimeErrCnt);
			GetDlgItem(IDC_STATIC_CON)->SetWindowText(msg);

		}
	}
	else if (canid.cmd == CAN_CHK_COM_CMD + 0x80 && rcvLen == 0)
	{
		updateData.Format("제어기 ID %d 연결 정상 응답\n", canid.id);
		SetInfoMsg(updateData);
	}
	else if (canid.cmd == CAN_CHK_VERSION_CMD + CAN_ACK)
	{
		char fwver[32] = { 0 };  // 마지막 쓰레기값 출력으로 총갯수에서 1 추가.
		//memcpy(fwver, lp, rcvLen);		
		if (rcvLen >= 8 && rcvLen <= 24 ) {  // 길이 8 이상 경우.
			//CString ver = (char*)lp;
			memcpy(fwver, lp, rcvLen);
			updateData.Format("[%d]ID 0x%X(%d) FW Ver : % s\n", rcvLen, canid.id, canid.id, fwver);
			SetInfoMsg(updateData);
		}
		if (cnt) {
			d_msg("FWver Cnt %d %d\r\n", cnt, FwCnt);
			cnt--;
		}
		dev1Set.maxChCnt++;
	}
	else if (canid.cmd == CAN_VOLT_SEL_CMD + 0x80 && rcvLen == 1) {
		updateData.Format("ID %d Ack : %d\n", canid.id, (uint8_t)lp[0]);
		SetInfoMsg(updateData);
	}
	else if (canid.cmd == CAN_RUN_OCV_CMD + 0x80 && rcvLen == 0) {}
	else if (canid.cmd == CAN_CH_STOP_CMD + 0x80 && rcvLen == 1) {}
	else if (canid.cmd == CAN_START_CHG_CMD + 0x80 && rcvLen == 0)
	{
		/*if (dev1Set.id == canid.id)			dev1Set.start = true;
		else if (dev2Set.id == canid.id)	dev2Set.start = true;		*/
		bfTime = 0;
	}
	else if (canid.cmd == CAN_READ_OCV_CMD + 0x80)
	{
		bool ok = (bool)lp[0];
		if (ok == true)
		{
			dstIdx = 0;
			if (rcvLen == 17) {
				for (int i = 1; i <= 13; i += 4) {
					memcpy(&volt[dstIdx], &lp[i], sizeof(uint32_t));
					fvolt[dstIdx] = (float)(volt[dstIdx] * 0.1);
					dstIdx++;
				}
				updateData.Format("제어기 ID %d OCV 측정 값(mV) -> 1[%.1f]  2[%.1f]  3[%.1f]  4[%.1f]", canid.id, fvolt[0], fvolt[1], fvolt[2], fvolt[3]);
			}
			else if (rcvLen == 9) {
				for (int i = 1; i <= 5; i += 4) {
					memcpy(&volt[dstIdx], &lp[i], sizeof(uint32_t));
					fvolt[dstIdx] = (float)(volt[dstIdx] * 0.1);
					dstIdx++;
				}
				updateData.Format("제어기 ID %d OCV 측정 값(mV) -> 1[%.1f]  2[%.1f]", canid.id, fvolt[0], fvolt[1]);
			}
			else if (rcvLen == 5) {		 // 대전 150A
				memcpy(&volt[0], &lp[1], sizeof(uint32_t));
				fvolt[0] = (float)(volt[0] * 0.1);
				updateData.Format("제어기 ID %d OCV 측정 값(mV) -> [%.1f]", canid.id, fvolt[0]);
			}
			SetInfoMsg(updateData);
		}
		else {
			updateData.Format("제어기 ID %d OCV 측정 실패", canid.id);
		}
	}
	else if (canid.cmd == CAN_SET_CONTACT_CMD + 0x80 && rcvLen == 0) {}
	else if (canid.cmd == CAN_CHK_CONTACT_CMD + 0x80) {
		uint8_t ret = (uint8_t)lp[0];
		//if (ret != 0x00) 
		{   // 접촉 검사 종료..
			uint8_t state[5] = { 0x00 };
			int volt, dvolt1, dvolt2, dvolt3, dvolt4;
			if (rcvLen == 16)
			{  // cycler
				if (dev1Set.type == _5V_20A_PACK_TYPE) {
					if (ret == 0x80) ret = 12;
					memcpy(&volt, &lp[4], sizeof(int32_t));
					memcpy(&dvolt1, &lp[8], sizeof(int16_t));
					memcpy(&dvolt2, &lp[10], sizeof(int16_t));
					memcpy(&dvolt3, &lp[12], sizeof(int16_t));
					memcpy(&dvolt4, &lp[14], sizeof(int16_t));
					updateData.Format("ID %d 접촉결과 [%s] OCV[%.1f] V1[%d] V2[%d] DC[%d] RV[%d]", canid.id, contactMsg2[ret], volt * 0.1f, (int16_t)dvolt1, (int16_t)dvolt2, (int16_t)dvolt3, (int16_t)dvolt4);
				}
				else {
					if (ret == 0x80) ret = 0x09;
					memcpy(&volt, &lp[4], sizeof(int32_t));
					memcpy(&dvolt1, &lp[8], sizeof(int16_t));
					memcpy(&dvolt2, &lp[10], sizeof(int16_t));
					memcpy(&dvolt3, &lp[12], sizeof(int16_t));
					memcpy(&dvolt4, &lp[14], sizeof(int16_t));
					updateData.Format("ID %d 접촉결과 [%s] OCV[%.1f] V1[%d] V2[%d] C1[%d] C2[%d]", canid.id, contactMsg[ret], volt * 0.1f, dvolt1, dvolt2, dvolt3, dvolt4);
					SetInfoMsg(updateData);
				}
				SetInfoMsg(updateData);
			}
			else if (rcvLen == 12)
			{
				if (ret == 0x80) ret = 0x09;
				memcpy(&dvolt1, &lp[2], sizeof(int16_t));
				memcpy(&volt, &lp[4], sizeof(int32_t));
				memcpy(&dvolt2, &lp[8], sizeof(int16_t));
				memcpy(&dvolt3, &lp[10], sizeof(int16_t));

				updateData.Format("ID %d 접촉결과 [%s] OCV[%.1f] V1[%d] V2[%d] C1[%d]", canid.id, contactMsg[ret], volt * 0.1f, (int16_t)dvolt1, (int16_t)dvolt2, (int16_t)dvolt3);
				SetInfoMsg(updateData);
				//d_msg("dvolt1 %d %d %d\n", dvolt1, dvolt2, dvolt3);
			}
			else d_msg("CAN_CHK_CONTACT_CMD err\n");
		}
	}
	else if (canid.cmd == CAN_FW_UP_CMD + 0x80 && rcvLen == 0)
	{
		updateData.Format("제어기 ID %d FW 업데이트 준비\n", canid.id);
		SetInfoMsg(updateData);
	}
	else if (canid.cmd == CAN_SENSING_CHECK_CMD + CAN_ACK)
	{
		CString str;
		int16_t Involt, Outvolt, BiasVolt, cellP, cellN, Temp, icur[4] = { 0 }, duty;
		int32_t Volt, allCur;
		int16_t PhaseTemp[4];
		int16_t PhaseCur[4];

		updateData = "None!";
		if (dev1Set.type == _100V_500A_TYPE || dev1Set.type == _TEST_DUTY_TYPE || dev1Set.type == _160V_60A_32CH_TYPE)
		{
			memcpy(&Volt, &lp[0], sizeof(int32_t));
			Volt = Volt & 0x00FFFFFF; // 3byte 처리
			if (Volt > 0x800000) Volt |= (0xff << 24); // 음수 처리

			//memcpy(&Involt, &lp[2], sizeof(int16_t));
			Involt = (uint8_t)lp[3];
			memcpy(&cellP, &lp[4], sizeof(int16_t));
			memcpy(&cellN, &lp[6], sizeof(int16_t));
			/*uint8_t endCode = (uint8_t)lp[6];
			uint8_t faultCode = (uint8_t)lp[7];*/
			memcpy(&allCur, &lp[8], sizeof(int32_t));
			memcpy(&Outvolt, &lp[12], sizeof(int16_t));
			memcpy(&Temp, &lp[14], sizeof(int16_t));
		
			
			updateData.Format("ID 0x%X(%d) 입력 %d V, 출력 %4.1f V, Phase %d mA data %d , %d ", canid.id, canid.id, Involt, Outvolt * 0.1, allCur, cellP, cellN);
			if (dev1Set.testModeType == TEST_DUTY_160V60A || dev1Set.type == _160V_60A_32CH_TYPE)
			{
				memcpy(&Involt, &lp[0], sizeof(int16_t));
				memcpy(&Outvolt, &lp[2], sizeof(int16_t));
				memcpy(&BiasVolt, &lp[4], sizeof(int16_t));

				memcpy(PhaseTemp, &lp[8], sizeof(int16_t)*4);

				memcpy(&duty, &lp[16], sizeof(int16_t));

				memcpy(PhaseCur, &lp[18], sizeof(int16_t) * 3);

				allCur = PhaseCur[0] + PhaseCur[1] + PhaseCur[2];
				updateData.Format("0x%X(%d) [%.2f%%] 입력 %3.2fV, 출력 %3.2fV, Bias %3.2fV, st[%X, %X]\r\nCur %.3fA, [%d, %d, %d mA], Temp %.1f, %.1f, %.1f, %.1f ", canid.id, canid.id, duty * 0.01f, Involt*0.01f, Outvolt * 0.01f, BiasVolt * 0.01f, (uint8_t)lp[6], (uint8_t)lp[7], allCur*0.001f, PhaseCur[0], PhaseCur[1], PhaseCur[2], PhaseTemp[0] * 0.1f, PhaseTemp[1] * 0.1f, PhaseTemp[2] * 0.1f, PhaseTemp[3] * 0.1f);	
			}
		}
		else if (dev1Set.type == _5V_200A_TYPE)
		{
			memcpy(&volt[0], &lp[0], sizeof(int16_t));
			memcpy(&Involt, &lp[2], sizeof(int16_t));
			memcpy(&Temp, &lp[4], sizeof(int16_t));
			uint8_t endCode = (uint8_t)lp[6];
			uint8_t faultCode = (uint8_t)lp[7];
			uint8_t faultmsg = faultCode;

			memcpy(&icur[0], &lp[8], sizeof(int16_t));
			memcpy(&icur[1], &lp[10], sizeof(int16_t));
			memcpy(&icur[2], &lp[12], sizeof(int16_t));
			memcpy(&icur[3], &lp[14], sizeof(int16_t));
			allCur = icur[0] + icur[1] + icur[2] + icur[3];
			//if (faultCode > 0x0E) 	faultmsg = 17;

			updateData.Format("ID 0x%X(%d) 입력 %.1f V, 개별전류 [%.1f] [%.1f] [%.1f] [%.1f] mA", canid.id, canid.id, Involt * 0.1f, (icur[0] * 0.1f), (icur[1] * 0.1f), (icur[2] * 0.1f), (icur[3] * 0.1f));

		}
		if (rcvGrp == false)  SetInfoMsg(updateData);
	}
	else if (canid.cmd == CAN_CELL_BYPASS_CMD + CAN_ACK)  // 각 셀 pass 설정. _160V_60A_32CH_TYPE
	{
		// 에러
		if ((uint8_t)lp[0] != 0x00)	updateData.Format("Cell Pass Err %X, %X", (uint8_t)lp[0], (uint8_t)lp[2]);
		else						updateData.Format("Cell Pass OK Cnt %d, %X", (uint8_t)lp[1], (uint8_t)lp[2]);
		SetInfoMsg(updateData);		
	}
	else if (canid.cmd == CAN_TEST_MODE_CMD + CAN_ACK && rcvLen == 8)
	{
		memcpy(&dev1Set.getDuty, &lp[1], sizeof(uint16_t));
		dev1Set.getGpio1 = (uint8_t)lp[3];
		dev1Set.getGpio2 = (uint8_t)lp[4];
		dev1Set.getGpio3 = (uint8_t)lp[5];
		d_msg("TEST_MODE cmd %x , duty %d, GPIO %d %d %d\n", (uint8_t)lp[0], dev1Set.getDuty, dev1Set.getGpio1, dev1Set.getGpio2, dev1Set.getGpio3);
		if (dev1Set.type == _TEST_DUTY_TYPE)
		{
			if (dev1Set.getGpio1 == 1)	GetDlgItem(IDC_BTN_CHG_CUR)->SetWindowText("LD-SW On");
			else if (dev1Set.getGpio1 == 0)	GetDlgItem(IDC_BTN_CHG_CUR)->SetWindowText("LD-SW Off");
			if (dev1Set.getGpio2 == 1)	GetDlgItem(IDC_BTN_T3)->SetWindowText("STA 2");
			else if (dev1Set.getGpio2 == 0)	GetDlgItem(IDC_BTN_T3)->SetWindowText("STA 1");
			if (dev1Set.getGpio3 == 1)	GetDlgItem(IDC_BTN_REST)->SetWindowText("FAN On");
			else if (dev1Set.getGpio3 == 0)	GetDlgItem(IDC_BTN_REST)->SetWindowText("FAN Off");
		}
	}
	else if (canid.cmd == CAN_READ_TEMP_CMD + 0x80) {
		memcpy(&temperature, &lp[0], sizeof(int16_t));
		memcpy(&cableVolt, &lp[4], sizeof(uint16_t));
		memcpy(&voutVolt, &lp[6], sizeof(uint16_t));

		updateData.Format("ID 0x%X(%d) Grp[%x] Cable[%d] %1.fmV Out %dmV", canid.id, canid.id, (uint8_t)lp[2], (uint8_t)lp[3], cableVolt * 0.1f, voutVolt);
		if (rcvGrp == false && dev1Set.type == _5V_200A_TYPE)  SetInfoMsg(updateData);

	}
	else if (canid.cmd == CAN_PATTERN_UPDATE_CMD + 0x80 && rcvLen == 1) {
		if ((uint8_t)lp[0] == 1 || (uint8_t)lp[0] == 2) SetInfoMsg("Ack OK");
		else {
			updateData.Format("Ack Err %X", (uint8_t)lp[0]);
			SetInfoMsg(updateData);
		}
	}
	else if (canid.cmd == CAN_PROTECT_DATA_CMD + 0x80) {
		SetInfoMsg("PROTECT Ack OK");
	}
	else if (canid.cmd == CAN_SET_PACK_USER_DATA_CMD + 0x80) {
		char strTemp[64] = { 0 };
		if (rcvLen > 64) rcvLen = 64;

		memcpy(strTemp, &lp[1], rcvLen - 1);
		updateData.Format("Read User BMS Ack [0x%02X] len[%d] data %s \n", (uint8_t)lp[0], rcvLen, strTemp);
		SetInfoMsg(updateData);
		updateData = "hex => ";
		for (int i = 1; i < rcvLen; i++) {
			updateData.AppendFormat("%02X ", (uint8_t)lp[i]);
		}
		SetInfoMsg(updateData);

	}
	else if (canid.cmd == CAN_READ_PACK_DATA_CMD + 0x80) {
		CString items, packData;
		items = "[Bat.Stat]    [SOC]       [Volt]       [Cur ]           [Temp]             [ChgVolt]      [ChgCur]      [CycCnt]     [FullCap]";
		dev1Set.PackDataType = (uint8_t)lp[0];
		updateData.Format("ID[%d] Read BMS Ack [0x%02X] len %d type %X", canid.id, (uint8_t)lp[1], rcvLen, dev1Set.PackDataType);
		TRACE("Read BMS Ack [0x%02X] len %d data %X", (uint8_t)lp[1], rcvLen, dev1Set.PackDataType);
		SetInfoMsg(updateData);
		if (dev1Set.PackDataType == 0)
		{
			memcpy(&P_battState, &lp[8], sizeof(int16_t));
			memcpy(&P_soc, &lp[10], sizeof(uint16_t));
			memcpy(&P_volt, &lp[12], sizeof(uint16_t));
			memcpy(&P_cur, &lp[14], sizeof(int16_t));
			memcpy(&P_temp, &lp[16], sizeof(int16_t));
			memcpy(&P_chgVolt, &lp[18], sizeof(uint16_t));
			memcpy(&P_chgCur, &lp[20], sizeof(uint16_t));
			memcpy(&P_cycCnt, &lp[22], sizeof(uint16_t));
			memcpy(&P_fullCap, &lp[24], sizeof(uint16_t));
			kelvin = P_temp * 0.1f;
			P_degC = kelvin - 273.15f;
			packData.Format("0x%04X        %d %%      %d mV    %d mA      %.1f K(%.1f)      %d mV       %d mA  \t     %d  \t       %d mAh", P_battState, P_soc, P_volt, (int16_t)P_cur, kelvin, P_degC, P_chgVolt, P_chgCur, P_cycCnt, P_fullCap);
			dev1Set.PackDataSave = true;
		}
		else if (dev1Set.PackDataType == 1)
		{
			memcpy(&P_userData, &lp[8], sizeof(int16_t) * 12);
			//dev1Set.PackReadCmd[i]
			//items.Format("[User01]   [User02]   [User03]   [User04]   [User05]   [User06]   [User07]    [User08]   [User09]     [User10]   [User11]   [User12]");
			packData = "";
			items = "";
			for (int i = 0; i < 12; i++) {
				items.AppendFormat("[ 0x%02X ]    ", dev1Set.PackReadCmd[i]);
				packData.AppendFormat("0x%04X     ", P_userData[i]);
			}
		}
		GetDlgItem(IDC_STATIC_PACK_ITEM)->SetWindowText(items);
		GetDlgItem(IDC_STATIC_PACK_DATA)->SetWindowText(packData);


	}
	else if (canid.cmd == CAN_START_PATTERN_CMD + 0x80) {
		if ((uint8_t)lp[0] == 1) SetInfoMsg("Start PatAck OK");
		else {
			updateData.Format("Start PatAck Err %X", (uint8_t)lp[0]);
			SetInfoMsg(updateData);
		}
	}

	//else {
#ifdef _SYS_LOG
	updateData.Format("해당 명령 처리가 없음. cmd %X len %d data %X %X\n", canid, rcvLen, lp[55], lp[63]);
	SetInfoMsg(updateData);
	debugMsg.Append(updateData);
	// 수신 hex 데이터 콘솔 출력
	updateData.Format("id %x cnt %d data =>", wp[0], wp[1]);
	debugMsg.Append(updateData);
	d_msg("id %x cnt %d data => ", wp[0], wp[1]);
	for (int i = 0; i < rcvLen; i++) {
		d_msg(" %2.2X", (uint8_t)lp[i]);
		updateData.Format(" %2.2X", (uint8_t)lp[i]);
		debugMsg.Append(updateData);
	}
	d_msg("\n");	debugMsg.Append("]\n");
#endif // _SYS_LOG
	//}
	//d_msg("%sOnUpdateRcvData id %d cmd %X len %d\n", GetTimeStamp(0), canid.id, canid.cmd, rcvLen);
	//if (dev1Set.start && dev1Set.autoMsrReq) SendControlCmd(canid.id, CAN_READ_TEMP_CMD);
	return 0;
}

void CDisChgTesterDlg::OnBnClickedBtnT4()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	GetDevSetData(DEV_1, 0);		//다이얼로그 값 적용.
	//GetDevSetData(DEV_2, 0);		//다이얼로그 값 적용.
	SaveSetParamFile(3);
	CString strTemp;
	strTemp.Format("제어기 ID %d 현재 설정 값 저장됨!", dev1Set.id);
	SetInfoMsg(strTemp);
	LoadSetParamFile(3);		// set.ini 파일 적용
}

LRESULT CDisChgTesterDlg::OnSendData(WPARAM wP, LPARAM lP)
{
	// TODO: 여기에 구현 코드 추가.
	if (m_isUpdateFW) return 1;
	if (sendRepCnt >= 9)    sendRepCnt = 1;  // 100ms *10 => 1초 주기로 초기화.
	else sendRepCnt++;
	// 100ms 주기로 순차적으로 전송.  제어기 1, 2 사이 400ms 간격으로 송신..
	if (sendRepCnt == 1) {
		if (dev1Set.startDelay) {			// 스타트요청 후 측정값 요청 명령
			SendControlCmd(dev1Set.id, CAN_START_CHG_CMD);
			dev1Set.startDelay = false;
		}
		else {
			//if (dev1Set.start && dev1Set.autoMsrReq) SendControlCmd(dev1Set.id, CAN_READ_TEMP_CMD);
			//else {				// 시험 정지 후 측정값 추가 요청...상태 동기화위해..
			//	if (dev1Set.stopCnt) {
			//		if (dev1Set.stopCnt == DEV_STOP_READ_CNT) SendControlCmd(dev1Set.id, CAN_CH_STOP_CMD);
			//		else SendControlCmd(dev1Set.id, CAN_READ_DATA_CMD);
			//		dev1Set.stopCnt--;
			//	}
			//}
		}
	}
	else if (sendRepCnt == 2) {
	}

	/*if (DEBUG_MASTER && dev1Set.opMode != DEV_WATCH_MODE && dev2Set.opMode != DEV_WATCH_MODE) {
		if (sendRepCnt == 4) {
			if (dev1Set.start)	SendControlCmd(dev1Set.id, CAN_CH_STOP_CMD);
		}
	}*/
	//d_msg("%s send\n",GetTimeStamp(4));
	/*else if (sendRepCnt == 3) SendControlCmd(3, CAN_READ_DATA_CMD);
	else if (sendRepCnt == 4) {
		SendControlCmd(dev1Set.id, CAN_CHK_VERSION_CMD);
		Sleep(10);
		SendControlCmd(dev2Set.id, CAN_CHK_VERSION_CMD);
	}*/
	//else if (sendRepCnt == 5)

	return LRESULT();
}

void CDisChgTesterDlg::OnBnClickedBtnListClear()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	m_ListCh1.DeleteAllItems();
	m_EditCsvName.SetWindowTextA(NULL);
}

void CDisChgTesterDlg::OnBnClickedBtnListSave()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	FILE* pFile = NULL;
	CString saveFile;
	GetDlgItemText(IDC_EDIT_CSV_NAME, dev1Set.csvFileName);

	saveFile.Format("%s%s", CSV_DIR, dev1Set.csvFileName);

	if (GetFileAttributes(CSV_DIR) == -1) CreateDirectory(CSV_DIR, NULL);	// 디렉토리 생성.

	if (0 == fopen_s(&pFile, saveFile, "w+")) {
		fprintf(pFile, dev1Set.csvFile);
		fclose(pFile);
		saveFile.Append(" 저장완료");
		SetInfoMsg(saveFile);
		dev1Set.csvFile.Empty();
		m_ListCh1.DeleteAllItems();
		m_EditCsvName.SetWindowTextA("");
	}
	else {
		if (dev1Set.csvFileName != "")	SetInfoMsg("제어기 1 파일 저장 실패!");
	}
	if (dev1Set.setCur != _SET_200A && dev1Set.type == _5V_200A_TYPE && dev1Set.csvGrpFile.GetLength() > 1000)
	{
		saveFile.Format("%s%s", CSV_DIR, dev1Set.csvGrpFileName);

		if (0 == fopen_s(&pFile, saveFile, "w+")) {
			fprintf(pFile, dev1Set.csvGrpFile);
			fclose(pFile);
			saveFile.Append(" 저장완료");
			SetInfoMsg(saveFile);
			dev1Set.csvGrpFile.Empty();
		}
		else {
			if (dev1Set.csvGrpFileName != "")	SetInfoMsg("그룹 파일 저장 실패!");
		}
	}
	if (dev1Set.csvGrpFile.GetLength() > 100) {
		dev1Set.csvGrpFile.Empty();
	}
}

void CDisChgTesterDlg::SetInfoMsg(CString msg)
{
	// TODO: 여기에 구현 코드 추가.
	m_EditIfo.ReplaceSel(GetTimeStamp(4));
	m_EditIfo.ReplaceSel(msg);
	m_EditIfo.ReplaceSel("\r\n");
}

void CDisChgTesterDlg::SetDevSetData(uint8_t dev)
{
	// TODO: 여기에 구현 코드 추가.
	//UI 데이터 표시에는
	// 전압, 전류값 소수점자리 및 전류 음수 표시 삭제함.
	CString str;
	//CComboBox* pComboBox;
	if (dev == DEV_1)  // 화면표시는 1000 단위 나눔..
	{
		//m_ComboCh1_ID.SetCurSel(dev1Set.id);
		str.Format("%d", dev1Set.id);
		m_ComboCh1_ID.SetWindowTextA(str);
		m_ComboCh1Mode.SetCurSel(dev1Set.opCmd);
		d_msg("dev1Set.opCmd %d\n", dev1Set.opCmd);
		if (dev1Set.opCmd == DEV_DISCHG_MODE) {
			str.Format("%.3f", dev1Set.dis_volt * MSR_POINT_UNIT);
			GetDlgItem(IDC_EDIT_CH1_VOLT)->SetWindowText(str);
			str.Format("%.3f", (dev1Set.dis_cur / -1) * MSR_POINT_UNIT);
			GetDlgItem(IDC_EDIT_CH1_CUR)->SetWindowText(str);
			str.Format("%.3f", (dev1Set.dis_endCur / -1) * MSR_POINT_UNIT);
			GetDlgItem(IDC_EDIT_CH1_END_CUR)->SetWindowText(str);
			d_msg("dis %d,  %d, %d\n", dev1Set.dis_volt, dev1Set.dis_cur, dev1Set.dis_endCur);
			//SetDlgItemInt(IDC_EDIT_CH1_VOLT, dev1Set.dis_volt/ MSR_UNIT);
			//SetDlgItemInt(IDC_EDIT_CH1_CUR, (dev1Set.dis_cur / -1)/ MSR_UNIT);  // 음수처리..
			//SetDlgItemInt(IDC_EDIT_CH1_END_CUR, dev1Set.dis_endCur/ MSR_UNIT);
		}
		else {
			str.Format("%.3f", dev1Set.volt * MSR_POINT_UNIT);
			GetDlgItem(IDC_EDIT_CH1_VOLT)->SetWindowText(str);
			str.Format("%.3f", dev1Set.cur * MSR_POINT_UNIT);
			GetDlgItem(IDC_EDIT_CH1_CUR)->SetWindowText(str);
			str.Format("%.3f", dev1Set.endCur * MSR_POINT_UNIT);
			GetDlgItem(IDC_EDIT_CH1_END_CUR)->SetWindowText(str);
			d_msg("chg %d,  %d, %d\n", dev1Set.volt, dev1Set.cur, dev1Set.endCur);
			//SetDlgItemInt(IDC_EDIT_CH1_VOLT, dev1Set.volt/ MSR_UNIT);
			//SetDlgItemInt(IDC_EDIT_CH1_CUR, dev1Set.cur/ MSR_UNIT);
			//SetDlgItemInt(IDC_EDIT_CH1_END_CUR, dev1Set.endCur/ MSR_UNIT);
		}
		SetDlgItemInt(IDC_EDIT_CH1_END_TIME, dev1Set.endTime / MSR_UNIT);

		//SetDlgItemInt(IDC_EDIT_CH1_STEP, dev1Set.stepNum);
		SetDlgItemInt(IDC_EDIT_CH1_MSR_TIME, dev1Set.msrTime);
		SetDlgItemInt(IDC_EDIT_CH1_POWER, dev1Set.endPower);
		//SetDlgItemInt(IDC_EDIT_CH1_END_CUR, dev1Set.endCur/MSR_UNIT);
		SetDlgItemInt(IDC_EDIT_CH1_END_CAP, dev1Set.endCap);
		SetDlgItemInt(IDC_EDIT_CH1_END_WATTH, dev1Set.endWattH);
		SetDlgItemInt(IDC_EDIT_CH1_END_POWER, dev1Set.endPower);
		if (dev1Set.type == _160V_60A_32CH_TYPE) {	
			CString strCh;
			strCh.Format("%X", dev1Set.activeCh);
			GetDlgItem(IDC_EDIT_CH1_POWER)->SetWindowText(strCh);			
		}
	}
	else if (dev == DEV_2)
	{
	}
}

void CDisChgTesterDlg::InitDevData()
{
	// TODO: 여기에 구현 코드 추가.
	dev1Set.id = 1;
	dev1Set.opCmd = DEV_CHG_MODE;
	dev1Set.start = false;
	dev1Set.cur = 43000;
	dev1Set.dis_cur = -43000;
	dev1Set.volt = 4200;
	dev1Set.dis_volt = 3200;
	dev1Set.maxVolt = 1000;
	dev1Set.endCur = 7800;
	dev1Set.dis_endCur = 0;
	dev1Set.endTime = 200;
	dev1Set.ch = 0x0F;
	dev1Set.csvFileName = "";
	dev1Set.csvFile = "";
	dev1Set.stopCnt = 0;

	// 접촉 검사 세팅
	contactSet.cur = 1000;
	contactSet.time = 1000;
	contactSet.recoverTime = 0;
	contactSet.passVolt = 100;
	contactSet.reverseVolt = 5000; //(-5mv)
	contactSet.dVoltMin = 10;  //0.01mV
	contactSet.dVoltMax = 30000; //300mV
	dev1Set.dutyRange = 1000;
	dev1Set.dutyMax = int(dev1Set.dutyRange * 0.1f);  // 10%	

	m_NonPnPHandles[0] = PCAN_ISABUS1;
	m_NonPnPHandles[1] = PCAN_ISABUS2;
	m_NonPnPHandles[2] = PCAN_ISABUS3;
	m_NonPnPHandles[3] = PCAN_ISABUS4;
	m_NonPnPHandles[4] = PCAN_ISABUS5;
	m_NonPnPHandles[5] = PCAN_ISABUS6;
	m_NonPnPHandles[6] = PCAN_ISABUS7;
	m_NonPnPHandles[7] = PCAN_ISABUS8;
	m_NonPnPHandles[8] = PCAN_DNGBUS1;

	m_IsFD = false;
	m_objPCANBasic = new PCANBasicClass();
	m_CanDevType = NONE_CAN;
}

void CDisChgTesterDlg::OnCbnSelchangeComboCh1Op()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	CString tmpStr, updateStr;

	dev1Set.opCmd = m_ComboCh1Mode.GetCurSel();	// 모드 선택에 따라 설정값 변경
	if (dev1Set.opCmd == DEV_DISCHG_MODE || dev1Set.opCmd == DEV_CHG_MODE)	SetWindowUI(DEV_1, 3);
	else																	SetWindowUI(DEV_1, 4);
	SetDevSetData(DEV_1);
	updateStr = "전압V\t     전류A\tPower";
	if (dev1Set.type == _5V_20A_PACK_TYPE)	updateStr = "전압V\t     전류A       파워W/Ω";
	else if (dev1Set.type == _160V_60A_32CH_TYPE) {
		updateStr = "전압V\t     전류A       동작Cell";
	}

	if (dev1Set.opCmd == DEV_DISCHG_MODE) {
		tmpStr.Format("ID %d 방전설정!", dev1Set.id);
	}
	else if (dev1Set.opCmd == DEV_CHG_MODE) {
		tmpStr.Format("ID %d 충전설정!", dev1Set.id);
	}
	else if (dev1Set.opCmd == DEV_PATTERN_MODE) {
		tmpStr.Format("ID %d 패턴설정!", dev1Set.id);
		updateStr = "전압V\t     패턴A";
	}
	SetInfoMsg(tmpStr);
	GetDlgItem(IDC_STATIC_CH3)->SetWindowText(updateStr);
	//d_msg("cbox v%d c%d dc%d\n", dev1Set.volt, dev1Set.cur, dev1Set.dis_volt);
}

void CDisChgTesterDlg::SetWindowUI(int dev, int type)
{
	// TODO: 여기에 구현 코드 추가.
	if (dev == DEV_1)
	{
		if (type == 1) {   // 시험시작.
			GetDlgItem(IDC_BTN_CH1_START)->EnableWindow(FALSE);
			GetDlgItem(IDC_COMBO_CH1_ID)->EnableWindow(FALSE);
			GetDlgItem(IDC_COMBO_CH1_OP)->EnableWindow(FALSE);
			GetDlgItem(IDC_EDIT_CH1_VOLT)->EnableWindow(FALSE);
			GetDlgItem(IDC_EDIT_CH1_CUR)->EnableWindow(FALSE);
			GetDlgItem(IDC_EDIT_CH1_END_CUR)->EnableWindow(FALSE);
			GetDlgItem(IDC_EDIT_CH1_END_TIME)->EnableWindow(FALSE);
			GetDlgItem(IDC_BTN_T1)->EnableWindow(FALSE);
			GetDlgItem(IDC_BTN_T2)->EnableWindow(FALSE);
			GetDlgItem(IDC_BTN_T3)->EnableWindow(FALSE);
			//GetDlgItem(IDC_BTN_T9)->EnableWindow(FALSE);
			GetDlgItem(IDC_BTN_CH1_STOP)->EnableWindow(TRUE);
			
			SetWindowUI(DEV_ALL, 2); //비활성.
			if (dev1Set.type == _TEST_DUTY_TYPE) {
				GetDlgItem(IDC_BTN_CUR)->EnableWindow(TRUE);		// duty 설정.
				GetDlgItem(IDC_BTN_T9)->EnableWindow(TRUE);		// 상태확인
				GetDlgItem(IDC_BTN_CHG_CUR)->EnableWindow(TRUE);	// LD-SW , 전류조정
				GetDlgItem(IDC_BTN_T3)->EnableWindow(TRUE);		// 접촉검사, STA1
				GetDlgItem(IDC_BTN_T10)->EnableWindow(TRUE);		// 접촉결과, 센싱확인
				GetDlgItem(IDC_BTN_REST)->EnableWindow(TRUE);		// rest, FAN
			}
			if (dev1Set.type == _160V_60A_32CH_TYPE) {
				GetDlgItem(IDC_EDIT_CH1_CUR)->EnableWindow(TRUE);
			}
		}
		else if (type == 0) {  // 시험중지..
			GetDlgItem(IDC_BTN_CH1_START)->EnableWindow(TRUE);
			GetDlgItem(IDC_COMBO_CH1_ID)->EnableWindow(TRUE);
			GetDlgItem(IDC_COMBO_CH1_OP)->EnableWindow(TRUE);
			GetDlgItem(IDC_EDIT_CH1_VOLT)->EnableWindow(TRUE);
			GetDlgItem(IDC_EDIT_CH1_CUR)->EnableWindow(TRUE);
			GetDlgItem(IDC_EDIT_CH1_END_CUR)->EnableWindow(TRUE);
			GetDlgItem(IDC_EDIT_CH1_END_TIME)->EnableWindow(TRUE);
			if (!dev1Set.start) {
				GetDlgItem(IDC_BTN_T1)->EnableWindow(TRUE);
				GetDlgItem(IDC_BTN_T2)->EnableWindow(TRUE);
				GetDlgItem(IDC_BTN_T3)->EnableWindow(TRUE);
				GetDlgItem(IDC_BTN_T9)->EnableWindow(TRUE);
			}
			//GetDlgItem(IDC_BTN_CH1_STOP)->EnableWindow(FALSE);

			SetWindowUI(DEV_ALL, 3); //활성.
			if (dev1Set.type == _TEST_DUTY_TYPE) {
				GetDlgItem(IDC_BTN_CUR)->EnableWindow(FALSE);		// duty 설정.
				GetDlgItem(IDC_BTN_T9)->EnableWindow(TRUE);		// 상태확인
				GetDlgItem(IDC_BTN_CHG_CUR)->EnableWindow(FALSE);	// LD-SW , 전류조정
				GetDlgItem(IDC_BTN_T3)->EnableWindow(FALSE);		// 접촉검사, STA1
				GetDlgItem(IDC_BTN_T10)->EnableWindow(TRUE);		// 접촉결과, 센싱확인
				GetDlgItem(IDC_BTN_REST)->EnableWindow(FALSE);		// rest, FAN
			}
		}
		else if (type == 0x03) {   // 방전모드
			//GetDlgItem(IDC_EDIT_CH1_END_CUR)->EnableWindow(FALSE);
		}
		else if (type == 0x04) {   // 방전모드
			//GetDlgItem(IDC_EDIT_CH1_END_CUR)->EnableWindow(TRUE);
		}
	}
	else if (dev == DEV_ALL)
	{
		if (type == 1) {		// can 연결 해제.
			SetWindowUI(DEV_1, 1);
			SetWindowUI(DEV_2, 1);
			//GetDlgItem(IDC_BTN_CH1_STOP)->EnableWindow(FALSE);
			//GetDlgItem(IDC_BTN_T4)->EnableWindow(FALSE);
			GetDlgItem(IDC_BTN_FILLE_OPEN)->EnableWindow(FALSE);
			GetDlgItem(IDC_BTN_UPDATE)->EnableWindow(FALSE);
			GetDlgItem(IDC_CHECK_AUTO_START)->EnableWindow(FALSE);
			if (dev1Set.type == _TEST_DUTY_TYPE) {
				GetDlgItem(IDC_BTN_CUR)->EnableWindow(FALSE);		// duty 설정.
				GetDlgItem(IDC_BTN_T9)->EnableWindow(FALSE);		// 상태확인
				GetDlgItem(IDC_BTN_CHG_CUR)->EnableWindow(FALSE);	// LD-SW , 전류조정
				GetDlgItem(IDC_BTN_T3)->EnableWindow(FALSE);		// 접촉검사, STA1
				GetDlgItem(IDC_BTN_T10)->EnableWindow(FALSE);		// 접촉결과, 센싱확인
				GetDlgItem(IDC_BTN_REST)->EnableWindow(FALSE);		// rest, FAN
			}
		}
		else if (type == 0) {	// can 연결 .
			SetWindowUI(DEV_1, 0);
			SetWindowUI(DEV_2, 0);
			GetDlgItem(IDC_BTN_CH1_STOP)->EnableWindow(TRUE);
			//GetDlgItem(IDC_BTN_T4)->EnableWindow(TRUE);
			if (dev1Set.opCmd == DEV_DISCHG_MODE) SetWindowUI(DEV_1, 3);
			else                     SetWindowUI(DEV_1, 4);
			GetDlgItem(IDC_BTN_FILLE_OPEN)->EnableWindow(TRUE);
			GetDlgItem(IDC_BTN_UPDATE)->EnableWindow(TRUE);
			if (dev1Set.type == _TEST_DUTY_TYPE) {
				GetDlgItem(IDC_BTN_CUR)->EnableWindow(FALSE);		// duty 설정.
				GetDlgItem(IDC_BTN_T9)->EnableWindow(TRUE);		// 상태확인
				GetDlgItem(IDC_BTN_CHG_CUR)->EnableWindow(FALSE);	// LD-SW , 전류조정
				GetDlgItem(IDC_BTN_T3)->EnableWindow(FALSE);		// 접촉검사, STA1
				GetDlgItem(IDC_BTN_T10)->EnableWindow(TRUE);		// 접촉결과, 센싱확인
				GetDlgItem(IDC_BTN_REST)->EnableWindow(FALSE);		// rest, FAN
			}

		}
		else if (type == 2) {   // csv 저장 , 클리어 버튼.
			GetDlgItem(IDC_BTN_LIST_SAVE)->EnableWindow(FALSE);  //비활성.
			GetDlgItem(IDC_BTN_LIST_CLEAR)->EnableWindow(FALSE);
		}
		else if (type == 3) {
			GetDlgItem(IDC_BTN_LIST_SAVE)->EnableWindow(TRUE);  // 활성..
			GetDlgItem(IDC_BTN_LIST_CLEAR)->EnableWindow(TRUE);
		}

		if (type == 11) { // can 업데이트 시작 시..
			GetDlgItem(IDC_BTN_FILLE_OPEN)->EnableWindow(FALSE);
			GetDlgItem(IDC_BTN_UPDATE)->EnableWindow(FALSE);

			GetDlgItem(IDC_BTN_CLOSE)->EnableWindow(FALSE);
		}
		else if (type == 21) {// can 업데이트 중료 후.
			GetDlgItem(IDC_BTN_FILLE_OPEN)->EnableWindow(TRUE);
			GetDlgItem(IDC_BTN_UPDATE)->EnableWindow(TRUE);
			GetDlgItem(IDC_BTN_CLOSE)->EnableWindow(TRUE);
		}
		else if (type == 0x30) {   // CV, 전류, 상태 확인, 전류조정, 접촉 감사, Rest 버튼 처리.
			GetDlgItem(IDC_BTN_CUR)->EnableWindow(FALSE);		// duty 설정.
			GetDlgItem(IDC_BTN_T9)->EnableWindow(FALSE);		// 상태확인
			GetDlgItem(IDC_BTN_CHG_CUR)->EnableWindow(FALSE);	// LD-SW , 전류조정
			GetDlgItem(IDC_BTN_T3)->EnableWindow(FALSE);		// 접촉검사, STA1
			GetDlgItem(IDC_BTN_T10)->EnableWindow(FALSE);		// 접촉결과, 센싱확인
			GetDlgItem(IDC_BTN_REST)->EnableWindow(FALSE);		// rest, FAN
		}
		else if (type == 0x31) {   // CV, 전류, 상태 확인, 전류조정, 접촉 감사, Rest 버튼 처리.
			GetDlgItem(IDC_BTN_CUR)->EnableWindow(TRUE);
			GetDlgItem(IDC_BTN_T9)->EnableWindow(TRUE);
			GetDlgItem(IDC_BTN_CHG_CUR)->EnableWindow(TRUE);
			GetDlgItem(IDC_BTN_T3)->EnableWindow(TRUE);
			GetDlgItem(IDC_BTN_T10)->EnableWindow(TRUE);
			GetDlgItem(IDC_BTN_REST)->EnableWindow(TRUE);
		}
		else if (type == 0xF0) {   // 테스트 모드			
			SetWindowUI(DEV_1, 1); //비활성.
			GetDlgItem(IDC_EDIT_CH1_END_CUR)->EnableWindow(FALSE);
			GetDlgItem(IDC_EDIT_CH1_POWER)->EnableWindow(TRUE);
			GetDlgItem(IDC_EDIT_CH1_VOLT)->ShowWindow(SW_HIDE);
			GetDlgItem(IDC_EDIT_CH1_CUR)->ShowWindow(SW_HIDE);
			//GetDlgItem(IDC_STATIC_CH4)->ShowWindow(SW_HIDE);
			GetDlgItem(IDC_EDIT_CH1_END_CUR)->ShowWindow(SW_HIDE);
			GetDlgItem(IDC_EDIT_CH1_END_CAP)->ShowWindow(SW_HIDE);
			GetDlgItem(IDC_EDIT_CH1_END_POWER)->ShowWindow(SW_HIDE);
			GetDlgItem(IDC_EDIT_CH1_END_WATTH)->ShowWindow(SW_HIDE);
			GetDlgItem(IDC_STATIC)->ShowWindow(SW_HIDE);
			GetDlgItem(IDC_CHECK_1K_2K)->ShowWindow(SW_HIDE);
			GetDlgItem(IDC_CHECK_AUTO_REQ)->ShowWindow(SW_HIDE);
			//GetDlgItem(IDC_BTN_T10)->ShowWindow(SW_HIDE);
			GetDlgItem(IDC_CHECK_AUTO_SEND)->ShowWindow(SW_HIDE);
			if (!adminMode) {
				GetDlgItem(IDC_TYPE_CB)->EnableWindow(FALSE);
				SetWindowUI(DEV_ALL, 0x30);
			}
		}
	}
}

void CDisChgTesterDlg::OnBnClickedBtnInfoClear()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	m_EditIfo.SetReadOnly(FALSE);
	m_EditIfo.SetSel(0, -1, TRUE);
	m_EditIfo.Clear();
	m_EditIfo.SetReadOnly(TRUE);
	if (dev1Set.type == _5V_20A_PACK_TYPE)	GetDlgItem(IDC_STATIC_PACK_DATA)->SetWindowTextA("#");
}

void CDisChgTesterDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	static int delayStep = 0, bfFwcnt = 0;
	static int _1secCnt = 0;
	CString strid;
	CString strTimer;
	if (nIDEvent == TIMER_1S_ID)
	{
		_1secCnt++;
		//CString strTimer;
		//GetDlgItem(IDC_STATIC_CON)->SetWindowText(GetTimeStamp(3));

		/*	d_msg("mode %d cur %d volt %d end %d ", dev1Set.opMode, dev1Set.cur, dev1Set.volt, dev1Set.endCur);
			d_msg("dis_cur %d dis_volt %d disEnd %d\n", dev1Set.dis_cur, dev1Set.dis_volt, dev1Set.endCur);
			d_msg("cycmode %d cycCnt %d cycTime %d\n", dev1Set.cycleMode, dev1Set.cycleCnt, dev1Set.cycleRestTime);*/
		if (dev1Set.start) {
		}
		else {   // OCV, 접촉 검사 등 결과 요청에 지연 송신 처리..

			if (dev1Set.sendTimerCnt)
			{
				dev1Set.sendTimerCnt--;
				if (dev1Set.sendTimerCnt == 0) {
					SendControlCmd(dev1Set.id, dev1Set.sendCmd);
					dev1Set.sendCmd = 0;
				}
			}

			if (m_ChkAutoStart.GetCheck() == true)
			{		// 자동동작 시
				if (restTime) {

					if (restTime % 2 == 0) {
						strTimer.Format("자동 동작 [%d/%d] 정지대기[%d]초", autoStartCnt, dev1Set.autoStartCnt, restTime);
						SetInfoMsg(strTimer);
					}

					restTime--;
					if (restTime == 0)	// 휴식시간 완료시
					{
						if (autoStartCnt)   // 자동 동작 횟수 
						{
							autoStartCnt--;
							if (autoModeTog) {
								autoModeTog--;
								if (dev1Set.opCmd == DEV_DISCHG_MODE) dev1Set.opCmd = DEV_CHG_MODE;
								else dev1Set.opCmd = DEV_DISCHG_MODE;
								m_ComboCh1Mode.SetCurSel(dev1Set.opCmd);
								OnCbnSelchangeComboCh1Op();
							}
							if (autoStartCnt == 0) {
								m_ChkAutoStart.SetCheck(0);
								SetInfoMsg("자동 동작 완료!");
							}
							else {
								strTimer.Format("공정 자동 시작[%d/%d] %d회 / 모드반복 %d회", autoStartCnt, dev1Set.autoStartCnt, (dev1Set.autoStartCnt - autoStartCnt), autoModeTog);
								SetInfoMsg(strTimer);
								restTime = dev1Set.restTime;
								OnBnClickedBtnCh1Start(); //충방전 시작.
							}
						}
					}
					else {

					}
				}
			}

		}

#ifdef CYCLE_MODE
		if (dev1Set.cycleMode && dev1Set.start == false) {
			if (dev1Set.cycleRestTime) {
				dev1Set.cycleRestTime--;
				if (dev1Set.cycleRestTime == 0) {
					dev1Set.cycleCnt++;
					dev1Set.cycleRestTime = REST_TIMER;
					OnBnClickedBtnCh1Start();
					if (dev1Set.cycleCnt == 2) dev1Set.cycleMode = FALSE;
				}
			}
		}

		//d_msg("chDelayTimer %d\n", chDelayTimer);
#endif
		if (m_isUpdateFW == false && FwCnt > 0) {
			if (FwCnt < fFwMaxCnt) {
				if (bfFwcnt != FwCnt) {
					bfFwcnt = FwCnt;
					delayStep = 2;
				}
			}
			if (delayStep) {
				delayStep--;
				if (delayStep == 1)			OnBnClickedBtnT1();
				else if (delayStep == 0)	OnBnClickedBtnUpdate();
				d_msg("delayStep %d\r\n", delayStep);
			}

		}
		if (saveCanMsgCnt)
		{
			saveCanMsgCnt--;
			// can log  저장.
			if (saveCanMsgCnt == 0) {
				if (canMsgLog.GetLength() > 100)
				{
					FILE* pFile = NULL;
					CString saveFile;
					saveFile.Format("log\\Can_%s_%d.log", GetTimeStamp(1), m_CanDevType);

					if (GetFileAttributes("log\\") == -1) CreateDirectory("log\\", NULL);	// 디렉토리 생성.

					if (0 == fopen_s(&pFile, saveFile, "w+")) {
						fprintf(pFile, canMsgLog);
						fclose(pFile);
						saveFile.Append(" Can-log 저장완료");
#ifdef DEBUG_MSG_01
						SetInfoMsg(saveFile);
#endif
					}
					else {
					}
				}
				canMsgLog = "";
			}

		}
		else {
			// 1분마다 초기화.
			if (canMsgLog.GetLength() > 100 && _1secCnt % 60 == 0) canMsgLog = "";
		}

		strid.Format("%X", dev1Set.lastId);
		GetDlgItem(IDC_STATIC_LAST_ID)->SetWindowText(strid);
	}
	if (nIDEvent == TIMER_500ms_ID)
	{

		m_ComboCh1_ID.GetWindowText(strid);
		int dspID = _ttoi(strid);
		if (dspID != dev1Set.id) {
			d_msgl("%d %d", dspID, dev1Set.id);
			dev1Set.id = dspID;
			strid.Format("ID 0x%X", dev1Set.id);
			GetDlgItem(IDC_STATIC_CH_ID)->SetWindowText(strid);
		}
	}
	if (nIDEvent == TIMER_100ms_ID)
	{
		int sendTimer = (dev1Set.msrTime / 100);  // 측정요청 데이터 ms
		//if (sendTimer == 10) sendTimer = 9;		// 1초 경우 100ms 전후 오차 발생되어 900ms 로 적용.
		if (sendTimer >= 1) {
			send100msTimer++;
			if (send100msTimer % sendTimer == 0) {
				if (dev1Set.autoMsrReq) OnBnClickedBtnT9();
			}
		}
		if (sendBtnCanTimer)	sendBtnCanTimer--;	// 버튼 전송			
	}
	CDialogEx::OnTimer(nIDEvent);
}

void CDisChgTesterDlg::OnDestroy()
{
	CDialogEx::OnDestroy();
	// TODO: 여기에 메시지 처리기 코드를 추가합니다.
	KillTimer(TIMER_1S_ID);
	KillTimer(TIMER_500ms_ID);
	openSuccess = false;
	m_isRecvCANThread = false;
	m_isSendCANThread = false;
	KillThred(1);
	d_msg("thred id %d\n", hThread.get_id());
	m_objPCANBasic->Uninitialize(m_PcanHandle);
	Sleep(500);

	if (0 <= hBus) {
		CAN_SetTransferMode(hBus, 0);
		CAN_Close(hBus);
		hBus = -1;
	}
#ifdef _SYS_LOG
	if (debugMsg.GetLength() < 100) return;
	FILE* pFile = NULL;
	CString saveFile;
	saveFile.Format("log\\%s.log", GetTimeStamp(1));

	if (GetFileAttributes("log\\") == -1) CreateDirectory("log\\", NULL);	// 디렉토리 생성.

	if (0 == fopen_s(&pFile, saveFile, "w+")) {
		fprintf(pFile, debugMsg);
		fclose(pFile);
		saveFile.Append(" log 저장완료");
		SetInfoMsg(saveFile);
	}
	else {
	}
#endif
}

void CDisChgTesterDlg::SaveSetParamFile(int dev)
{
	// TODO: 여기에 구현 코드 추가.
	CString strIni, strTemp;
	strIni.Format(_T("%s\\set.ini"), GetExePath());
	if (dev == DEV_1)
	{
		strTemp.Format("%d", dev1Set.id);
		::WritePrivateProfileString("Dev1", "id", strTemp, strIni);
		strTemp.Format("%d", dev1Set.volt);
		::WritePrivateProfileString("Dev1", "volt", strTemp, strIni);
		strTemp.Format("%d", abs(dev1Set.cur));
		::WritePrivateProfileString("Dev1", "cur", strTemp, strIni);
		strTemp.Format("%d", dev1Set.dis_volt);
		::WritePrivateProfileString("Dev1", "dis_volt", strTemp, strIni);
		strTemp.Format("%d", abs(dev1Set.dis_cur));
		::WritePrivateProfileString("Dev1", "dis_cur", strTemp, strIni);
		strTemp.Format("%d", dev1Set.endCur);
		::WritePrivateProfileString("Dev1", "endCur", strTemp, strIni);
		strTemp.Format("%d", dev1Set.endTime);
		::WritePrivateProfileString("Dev1", "endTime", strTemp, strIni);
		strTemp.Format("%d", dev1Set.type);
		::WritePrivateProfileString("Dev1", "type", strTemp, strIni);
		strTemp.Format("%d", dev1Set.testModeType);
		::WritePrivateProfileString("Dev1", "testModeType", strTemp, strIni);
	}
	else
	{
		SaveSetParamFile(DEV_1);
		CString tempport;
		m_ComboPort.GetWindowText(tempport);
		::WritePrivateProfileString("Sys", "port", tempport, strIni);
		CString tempbps;
		CComboBox* pComboBox = (CComboBox*)GetDlgItem(IDC_CANBPS_CB);
		pComboBox->GetWindowText(tempbps);
		::WritePrivateProfileString("Sys", "bps", tempbps, strIni);
		d_msg("SaveSetParamFile %d bsp %s \n", dev, tempbps);
	}
}

void CDisChgTesterDlg::LoadSetParamFile(int dev)
{
	// TODO: 여기에 구현 코드 추가.
	CString strIni, strTemp;
	strIni.Format(_T("%s\\set.ini"), GetExePath());
	if (dev == DEV_1)
	{
		dev1Set.id = GetPrivateProfileInt("Dev1", "id", 0, strIni);
		dev1Set.volt = GetPrivateProfileInt("Dev1", "volt", 0, strIni);
		dev1Set.cur = GetPrivateProfileInt("Dev1", "cur", 0, strIni);
		dev1Set.dis_volt = GetPrivateProfileInt("Dev1", "dis_volt", 0, strIni);
		dev1Set.dis_cur = (GetPrivateProfileInt("Dev1", "dis_cur", 0, strIni) * -1);
		dev1Set.endCur = GetPrivateProfileInt("Dev1", "endCur", 0, strIni);
		dev1Set.endTime = GetPrivateProfileInt("Dev1", "endTime", 0, strIni);
		dev1Set.stepNum = 0;
		dev1Set.parallelEn = 0;
		dev1Set.msrTime = 1000;
		dev1Set.power = 0;
		//dev1Set.endCur = 0;
		dev1Set.endCap = 0;
		dev1Set.endWattH = 0;
		dev1Set.endPower = 0;
		dev1Set.type = GetPrivateProfileInt("Dev1", "type", 0, strIni);
		dev1Set.autoStartCnt = GetPrivateProfileInt("Dev1", "autoStartCnt", 0, strIni);
		dev1Set.restTime = GetPrivateProfileInt("Dev1", "restTime", 0, strIni);
		dev1Set.autoModeTog = GetPrivateProfileInt("Dev1", "autoModeTog", 0, strIni);
		strTemp.Format("자동 반복 %d회, 충/방전 반복 %d회, 대기시간 %d초\r\n", dev1Set.autoStartCnt, dev1Set.autoModeTog, dev1Set.restTime);
		m_EditIfo.ReplaceSel(strTemp);
		dev1Set.testModeType = GetPrivateProfileInt("Dev1", "testModeType", 0, strIni);
		d_msg("dev1Set.type1 %d\n", dev1Set.type);
		d_msg("dis %d,  %d, %d\n", dev1Set.dis_volt, dev1Set.dis_cur, dev1Set.dis_endCur);
		d_msg("chg %d,  %d, %d\n", dev1Set.volt, dev1Set.cur, dev1Set.endCur);
	}
	else
	{
		LoadSetParamFile(DEV_1);
		char portNum[20] = { 0 , };
		GetPrivateProfileString("Sys", "port", NULL, portNum, 20, strIni);
		CString strPort;
		strPort.Format("%s", portNum);
		m_ComboPort.SetWindowTextA(strPort);
		char portBps[20] = { 0 , };
		CString strPortBps;
		GetPrivateProfileString("Sys", "bps", NULL, portBps, 20, strIni);
		strPortBps.Format("%s", portBps);
		CComboBox* pComboBox = (CComboBox*)GetDlgItem(IDC_CANBPS_CB);
		pComboBox->SetWindowTextA(portBps);
		CanBps = atoi(portBps);

		/*	if (GetPrivateProfileInt("Sys", "admin", 0, strIni) > 100) {
				CanBps =
			}*/

		int chkAdmin = GetPrivateProfileInt("Sys", "admin", 0, strIni);
		if (chkAdmin == ADMIN_PWD) 	adminMode = true;
		d_msg("LoadSetParamFile %d %s %d\n", dev, strPort, adminMode);
		int sendTimer = GetPrivateProfileInt("Sys", "sendTm", 0, strIni);

		if (sendTimer >= 100 && sendTimer != 0 && adminMode == true) 	CAN_SEND_TIME_THR = sendTimer / 10;

		int conTime = GetPrivateProfileInt("Con", "conTime", 0, strIni);
		int conCur = GetPrivateProfileInt("Con", "conCur", 0, strIni);
		int conReTime = GetPrivateProfileInt("Con", "conReTime", 0, strIni);
		int conPass = GetPrivateProfileInt("Con", "conPass", 0, strIni);
		if (conTime >= 100 && conCur >= 100 && (conTime + conCur) != 0 && adminMode == true)
		{
			contactSet.time = conTime;
			contactSet.cur = conCur;
			contactSet.recoverTime = conReTime;
			contactSet.passVolt = conPass;
		}
		if (adminMode == true) {
			strTemp.Format("관리자 모드, 송신 주기 %d ms, 수신형식 %d\r\n", CAN_SEND_TIME_THR * 10, setRcvType);			
			m_EditIfo.ReplaceSel(strTemp);
			//strTemp.Format("접촉 검사 시간 %dms 전류 %dmA ", contactSet.time, contactSet.cur);
			strTemp.Format("접촉 검사 시간 %dms 전류 %dmA 회복 %dms Pass %dmV 역전압 -%d(-%.1fmV) delta %d(%.1fmV) / %d(%.1fmV)\r\n", contactSet.time, contactSet.cur, contactSet.recoverTime, contactSet.passVolt, contactSet.reverseVolt, contactSet.reverseVolt * 0.01f, contactSet.dVoltMin, contactSet.dVoltMin * 0.01, contactSet.dVoltMax, contactSet.dVoltMax * 0.01);			
			m_EditIfo.ReplaceSel(strTemp);
		}
		int autoSend = GetPrivateProfileInt("Sys", "autoSend", 0, strIni);
		if (autoSend == 1) dev1Set.autoMsrReq = true;
		else dev1Set.autoMsrReq = false;
		int range = GetPrivateProfileInt("Test", "dutyRange", 0, strIni);
		if (range == 0) range = 1000;

		dev1Set.dutyRange = range;
		dev1Set.dutyMax = int(dev1Set.dutyRange * 0.1f);

		int canMsgEn = GetPrivateProfileInt("Sys", "DspCAN", 0, strIni);
		m_DspCAN = canMsgEn;

		setRcvType = GetPrivateProfileInt("Sys", "rcv_type", 0, strIni);
		d_msg("m_DspCAN %d setRcvType %d\n", m_DspCAN, setRcvType);
	}
	//d_msg("volt %d\n", dev1Set.volt);
}

void CDisChgTesterDlg::OnBnClickedBtnT5()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
}

void CDisChgTesterDlg::OnBnClickedCheckSaveCsv()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	CString strT;
	if (m_ChkboxSaveCsv.GetCheck()) strT.Format("시험 정지시 CSV파일 자동 저장됨.");
	else 							strT.Format("하단 CSV저장 버튼으로 수동 저장됨.");
	SetInfoMsg(strT);
}

void CDisChgTesterDlg::StopDev(int dev)
{
	// TODO: 여기에 구현 코드 추가.
	if (dev1Set.id == dev)		OnBnClickedBtnCh1Stop();
}

void CDisChgTesterDlg::OnBnClickedCheckAutoStop()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	CString strT;
	if (m_ChkAutoStop.GetCheck())	strT.Format("시험 시작 후 설정값에 따라 공정 자동 정지함.");
	else 							strT.Format("시험 시작 후 정지 버튼에 의해 정지함.");
	SetInfoMsg(strT);
}

BOOL CDisChgTesterDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	CString tempStr = "";
	if (pMsg->message == WM_KEYDOWN)
	{
		if (pMsg->wParam == VK_ESCAPE)			return TRUE;
		else if (pMsg->wParam == VK_RETURN) {
			if (dev1Set.start) CDisChgTesterDlg::OnBnClickedBtnCh1Stop();
			else CDisChgTesterDlg::OnBnClickedBtnCh1Start();
			return TRUE;
		}
		else if (pMsg->wParam == VK_UP) {
			//if (!dev1Set.start) 
			{
				int id = m_ComboCh1_ID.GetCurSel() + 1;
				//d_msg("VK_UP %d %d\n", id, m_ComboCh1_ID.GetCurSel());
				if (id > ID_CB_MAX) id = ID_CB_MAX;
				m_ComboCh1_ID.SetCurSel(id);
				dev1Set.id = id;
			}
			//else SetInfoMsg("충방전 동작중!");
			return TRUE;
		}
		else if (pMsg->wParam == VK_DOWN) {
			//if (!dev1Set.start) 
			{
				int id = m_ComboCh1_ID.GetCurSel() - 1;
				if (id < 0) id = 0;
				m_ComboCh1_ID.SetCurSel(id);
				//d_msg("VK_DOWN %d\n", id);
				dev1Set.id = id;
			}
			//else SetInfoMsg("충방전 동작중!");

			return TRUE;
		}
		else if (pMsg->wParam == VK_SPACE) {
			//CDisChgTesterDlg::OnBnClickedBtnCh1Stop();
			if (!dev1Set.start) {
				if (m_ComboCh1Mode.GetCurSel() == DEV_CHG_MODE) {
					m_ComboCh1Mode.SetCurSel(DEV_DISCHG_MODE);
				}
				else {
					m_ComboCh1Mode.SetCurSel(DEV_CHG_MODE);
				}
				dev1Set.opMode = CC_MODE;
				CDisChgTesterDlg::OnCbnSelchangeComboCh1Op();
			}
			else SetInfoMsg("충방전 동작중!");
			//d_msg("VK_SPACE %d\n", dev1Set.opCmd);
			return TRUE;
		}
		else if (pMsg->wParam == VK_F6) {
			if(dev1Set.type == _160V_60A_32CH_TYPE) return TRUE;
			CDisChgTesterDlg::OnBnClickedBtnChgCur();
			d_msg("F6\n");
			return TRUE;
		}
		else if (pMsg->wParam == VK_F5) {
			CDisChgTesterDlg::OnBnClickedBtnCh1Start();
			d_msg("F5\n");
			return TRUE;
		}
		else if (pMsg->wParam == VK_F8) {
			CDisChgTesterDlg::OnBnClickedBtnCh1Stop();
			d_msg("F8\n");
			return TRUE;
		}
		else if (pMsg->wParam == VK_F1) {
			SetInfoMsg(F1_HELP);
			return TRUE;
		}
		else if (pMsg->wParam == VK_F2) {
			SetInfoMsg(F2_INFO);
			return TRUE;
		}
	}
	return CDialogEx::PreTranslateMessage(pMsg);
}

void CDisChgTesterDlg::OnCbnDropdownComboCh1Op()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	GetDevSetData(DEV_1, 0);		//다이얼로그 값 적용.
}

void CDisChgTesterDlg::OnCbnDropdownComboCh2Op()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	GetDevSetData(DEV_2, 0);		//다이얼로그 값 적용.
}

void CDisChgTesterDlg::OnBnClickedBtnFilleOpen()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	CString t;
	if (dev1Set.type == _5V_20A_PACK_TYPE) t.Format("FW 파일 선택(*.bin) | *.bin");
	else t.Format("FW 파일 선택(*.hex) | *.hex");

	CFileDialog dlgFile(TRUE, NULL, NULL, OFN_EXPLORER | OFN_HIDEREADONLY, t);

	//"FW 파일 선택(*.bin * .hex) | *.bin; *.hex"
	OnBnClickedBtnT1();
	if (IDOK == dlgFile.DoModal()) {
		strUpdateFilePath = dlgFile.GetPathName();
		strUpdateFileName = dlgFile.GetFileName();
		SetInfoMsg("업데이트 선택 파일 : " + strUpdateFileName);
		((CButton*)GetDlgItem(IDC_CHECK_AUTO_MSR))->SetCheck(false);
	}
}

void CDisChgTesterDlg::OnBnClickedBtnUpdate()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	if (dev1Set.type == _5V_6A_TYPE) {
		AfxMessageBox("FW 업데이트 지원 안됨", MB_ICONSTOP);
		return;
	}
	m_isUpdateFW_DataThread = false;
	int startId = GetDevID(1);
	int endId = startId + 1;
	if (strUpdateFileName.GetLength() < 3) {
		SetInfoMsg("업데이트 파일 없음!");
		return;
	}
	else if (dev1Set.start) {
		SetInfoMsg("충방전 동작중!");
		//m_isUpdateFW_DataThread = false;
		return;
	}
	else if (endId == 0 || startId == 0) {
		SetInfoMsg("ID 0 확인!");
		return;
	}
	else if (endId < startId) {
		SetInfoMsg("업데이트 마지막 ID 가 더 작음!");
		return;
	}
	else {
		m_isUpdateFW_DataThread = true;
		dev1Set.parallelEn = ((CButton*)GetDlgItem(IDC_CHECK_GROUP))->GetCheck();

		if (dev1Set.type == _5V_20A_PACK_TYPE) {
			if (AfxBeginThread(ThreadUpdateFW_Stm32, this) != NULL) d_msg("STM32 FW 업데이트 시작\n");
		}
		else {
			if (AfxBeginThread(ThreadUpdateFW, this) != NULL) d_msg("DSP FW 업데이트 시작\n");
		}

		SetWindowUI(DEV_ALL, 1);
		SetWindowUI(DEV_ALL, 11);
		m_isUpdateFW = true;
	}
	/*else SetInfoMsg("업데이트 선택 파일 : " + strUpdateFileName);
	if (hThread.joinable()) hThread.join();
	hThread = std::thread(ThreadUpdateFW_Data, this);	*/
	//OnBnClickedBtnT1();
	//d_msg("FW 업데이트 시작11 %d\n", dev1Set.type);
}

int CDisChgTesterDlg::GetDevID(int num)
{
	// TODO: 여기에 구현 코드 추가.
	CString tempStr;
	if (num == 0) {
		return dev1Set.id;
	}
	else if (num == 1) {
		m_ComboCh1_ID.GetWindowText(tempStr);
		dev1Set.id = _ttoi(tempStr);
		return dev1Set.id;
	}
	else	return 0;
}

static uint32_t crc32_fdl(uint32_t crc, const uint8_t* buf, size_t len)
{
	uint32_t const CRC32_POLYNOMIAL = 0xedb88320ull;
	static uint32_t table[256] = { 0 };
	static int have_table = 0;

	const uint8_t* p, * q;

	if (have_table == 0) {
		int i, j;
		for (i = 0; i < 256; i++) {
			uint32_t rem;
			rem = i;
			for (j = 0; j < 8; j++) {
				if (rem & 1) {
					rem >>= 1;
					rem ^= CRC32_POLYNOMIAL;
				}
				else
					rem >>= 1;
			}
			table[i] = rem;
		}
		have_table = 1;
	}

	crc = ~crc;
	q = buf + len;
	for (p = buf; p < q; p++) {
		uint8_t octet;
		octet = *p;
		crc = (crc >> 8) ^ table[(crc & 0xff) ^ octet];
	}
	return ~crc;
}

void CDisChgTesterDlg::OnCbnSelchangeComboCh2Id()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
}

void CDisChgTesterDlg::KillThred(int num)
{
	// TODO: 여기에 구현 코드 추가.
	if (num == 1) {
		m_isUpdateFW_DataThread = false;
	}
}

void CDisChgTesterDlg::OnBnClickedBtnT9()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	int getData = GetDlgItemInt(IDC_EDIT_CH1_POWER);
	int getTime = GetDlgItemInt(IDC_EDIT_CH1_END_TIME);
	char txData[8] = { 0, };
	uint16_t static sendCmd = 0;
	CString info;
	//GetDevSetData(DEV_1, 0);		//다이얼로그 값 적용... 자동 측정값 요청..여기가 적용되면 충/방전 모드 전환시 전압값이 다르게 적용됨...
	SendControlCmd(dev1Set.id, CAN_READ_DATA_CMD);
	if (dev1Set.relayCmdTime) dev1Set.relayCmdTime--;

	if (dev1Set.autoRlyReq && dev1Set.type == _74V_80A_TYPE && dev1Set.relayCmdTime == 0) {

		if (getData == 0) sendCmd = 0;  // 1개씩 개별 제어할 경우.

		if (dev1Set.relayCmd > 15) {
			dev1Set.relayCmd = 0;
			sendCmd = 0;
		}

		if (dev1Set.relayCmd != 0) {
			SET_BIT(sendCmd, dev1Set.relayCmd - 1);
		}
		memcpy(txData, &sendCmd, 2);
		SendCAN_CmdData(dev1Set.id + 8, CAN_DAQ_RELAY_SET_CMD, txData, 2);
		info.Format("relay Set, %X, %X", (uint8_t)txData[0], (uint8_t)txData[1]);
		SetInfoMsg(info);
		dev1Set.csvFile.Append(GetTimeStamp(4));
		dev1Set.csvFile.Append(",,,,,,,,,,,,");
		dev1Set.csvFile.Append(info);
		dev1Set.csvFile.Append("\n");
		dev1Set.relayCmd++;
		if (getTime > 0) 	dev1Set.relayCmdTime = getTime;
		else  dev1Set.relayCmdTime = 5;
	}
	else {

	}
	if (!dev1Set.autoRlyReq && dev1Set.type == _74V_80A_TYPE) sendCmd = 0;

}

void CDisChgTesterDlg::OnCbnSelchangeCanbpsCb()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	CString str;
	CComboBox* pComboBox = (CComboBox*)GetDlgItem(IDC_CANBPS_CB);
	if (pComboBox->GetCurSel() != -1) {
		pComboBox->GetLBText(pComboBox->GetCurSel(), str);
		CanBps = atoi(str);
		CanBpsSet = true;
		d_msg("CanBps %d %d\n", CanBps, pComboBox->GetCurSel());
		//if(openSuccess)		SetInfoMsg("CAN 속도 변경 하려면 해제 후 다시 연결해야 함!");
	}
}

void CDisChgTesterDlg::Wait(DWORD dwMillisecond)
{
	// TODO: 여기에 구현 코드 추가.
	MSG msg;
	DWORD dwStart;
	dwStart = GetTickCount();

	while (GetTickCount() - dwStart < dwMillisecond)
	{
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
}

void CDisChgTesterDlg::OnBnClickedBtnT10()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	if (dev1Set.type == _TEST_DUTY_TYPE) {
		char txData[8] = { 0, };
		SendCAN_CmdData(dev1Set.id, CAN_SENSING_CHECK_CMD, txData, 0);
		return;
	}
	GetDevSetData(DEV_1, 0);
	SendControlCmd(dev1Set.id, CAN_CHK_CONTACT_CMD);
}

void CDisChgTesterDlg::OnBnClickedBtnT8()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	CString msg;
	static char selTog = 0;
	static char txData[8] = { 1, };
	selTog ^= 1;
	txData[0] = selTog;
	int len = 1;
	if (dev1Set.type == _160V_60A_32CH_TYPE) {
		txData[0] = 0x77;
		txData[1] = 0x88;
		txData[2] = 0x99;
		len = 3;
	}
	SendCAN_CmdData(dev1Set.id, CAN_VOLT_SEL_CMD, txData, len);
	msg.Format("Select %d", txData[0]);
	SetInfoMsg(msg);
	//충방전 기본설정
}

void CDisChgTesterDlg::OnEnChangeEditCh1Volt()
{
	// TODO:  RICHEDIT 컨트롤인 경우, 이 컨트롤은
	// CDialogEx::OnInitDialog() 함수를 재지정
	//하고 마스크에 OR 연산하여 설정된 ENM_CHANGE 플래그를 지정하여 CRichEditCtrl().SetEventMask()를 호출하지 않으면
	// 이 알림 메시지를 보내지 않습니다.

	// TODO:  여기에 컨트롤 알림 처리기 코드를 추가합니다.
}

void CDisChgTesterDlg::OnCbnSelchangeTypeCb()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	int cbType = m_ComboType.GetCurSel();

	CString msg = "사양 변경되어 충방전 전압/전류 설정 확인! ";
	if (cbType == _5V_200A_TYPE) {
		msg += "\nCan 500Kbps 변경필요!";
	}
	else if (cbType == _5V_6A_TYPE) {
		msg += "\nCan 1000Kbps 변경필요!";
	}
	else if (cbType == _100V_500A_TYPE) {
		msg += "\nCan 1000Kbps 변경필요!";
	}
	else if (cbType == _74V_80A_TYPE) {
		msg += "\nCan 250Kbps 변경필요!\n직렬형 충방전기";
	}
	else if (cbType == _TEST_DUTY_TYPE) {
		msg += "\nCan 1000Kbps 변경필요!\n모듈형 충방전기 PWM 테스트";
	}
	else if (cbType == _5V_20A_PACK_TYPE) {
		msg += "\nCan 1000Kbps 변경필요!\n팩 통신 싸이클러";
	}


	AfxMessageBox(msg, MB_ICONSTOP);
	dev1Set.type = cbType;
	LoadDevTypeData(cbType);
}

void CDisChgTesterDlg::LoadDevTypeData(int type)
{
	((CButton*)GetDlgItem(IDC_CHECK_AUTO_MSR))->SetCheck(false);
	// 기본 id 갱신.
	m_ComboCh1_ID.ResetContent();
	char* idList[8] = { "1", "2", "3", "4", "5", "6", "7", "8" };
	for (int i = 0; i < 8; i++)		m_ComboCh1_ID.AddString(idList[i]);

	if (type == _5V_200A_TYPE) {
		SET_MAX_VOLT = 4200;    // 4250.0 mV    => 4.25 V
		SET_MIN_VOLT = 3000;    // 3000.0 mV    => 3 V
		SET_END_CUR = 5000;    // 5000 mA
		SET_MAX_CUR = 200 * 1000; //200A
		//싸이클러 ID 0x11, 0x12, 0x13, 0x14, 0x21, 0x22, 0x23, 0x24 
		m_ComboCh1_ID.ResetContent();
		char* idList2[8] = { "17", "18", "19", "20", "33", "34", "35", "36" };
		for (int i = 0; i < 8; i++)		m_ComboCh1_ID.AddString(idList2[i]);

	}
	else if (type == _5V_6A_TYPE) {
		SET_MAX_VOLT = 4200;    // 4250.0 mV    => 4.25 V
		SET_MIN_VOLT = 3000;    // 3000.0 mV    => 3 V
		SET_END_CUR = 500;    // 500 mA
		SET_MAX_CUR = 6 * 1000; // 6A
	}
	else if (type == _100V_500A_TYPE) {
		SET_MAX_VOLT = 100 * 1000;    // 100000mv  => 100 V
		SET_MIN_VOLT = 30000;    // 3000.0 mV    => 30 V
		SET_END_CUR = 5000;    // 7800.0 mA    => 7.8 A
		SET_MAX_CUR = 500 * 1000; // 500A
	}
	else if (type == _74V_80A_TYPE) {
		SET_MAX_VOLT = 80 * 1000;    // 100000mv  => 100 V
		SET_MIN_VOLT = 10000;    // 
		SET_END_CUR = 5000;    // 7800.0 mA    => 7.8 A
		SET_MAX_CUR = 80 * 1000; // 500A
		SetInfoMsg("직렬형 충방전기");
		((CButton*)GetDlgItem(IDC_CHECK_AUTO_MSR))->SetCheck(true);
	}
	else if (type == _160V_60A_32CH_TYPE) {
		SET_MAX_VOLT = 160 * 1000;    // 100000mv  => 160 V
		SET_MIN_VOLT = 3000;    // 3000 mV    => 3 V
		SET_END_CUR = 5000;    // 7800.0 mA    => 7.8 A
		SET_MAX_CUR = 60 * 1000; // 60A
		SetInfoMsg("직렬형 160V60A 32채널 충방전기, CAN-1000Kbps");
		((CButton*)GetDlgItem(IDC_CHECK_AUTO_MSR))->SetCheck(true);
		m_ComboCh1_ID.ResetContent();
		char* idList2[12] = { "17", "18", "33", "34", "49", "50", "65", "66", "81", "82", "97", "98" };
		for (int i = 0; i < 12; i++)		m_ComboCh1_ID.AddString(idList2[i]);
		ID_CB_MAX = 12;
	}
	CString strid;
	strid.Format("%d", dev1Set.id);
	m_ComboCh1_ID.SetWindowTextA(strid);
	strid.Format("ID 0x%X", dev1Set.id);
	GetDlgItem(IDC_STATIC_CH_ID)->SetWindowText(strid);
}

void CDisChgTesterDlg::OnBnClickedBtnRest()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.		
	CString addText;
	if (dev1Set.type == _TEST_DUTY_TYPE) {
		sendBtnCanTimer = 3;
		Setgpio = dev1Set.getGpio3;
		Setgpio ^= 1;
		dev1Set.txCanData[0] = 0x33;
		dev1Set.txCanData[1] = Setgpio;
		dev1Set.CanCmd = CAN_TEST_MODE_CMD;
		dev1Set.CanSenLen = 3;
		sendBtnCanTimer = 3;
		return;
	}
	dev1Set.dspErrInfo = true;
	GetDevSetData(DEV_1, 0);		//다이얼로그 값 적용.
	SendControlCmd(dev1Set.id, CAN_REST_CMD);
	addText.Format("[%d] Start Rest!", dev1Set.id);
	//TRACE("[%d] Start Rest!", dev1Set.id);
	SetInfoMsg(addText);
	dev1Set.start = true;
	OnBnClickedBtnListClear();

	dev1Set.csvFileName.Format("%s_ID%d_OCV.csv", GetTimeStamp(1), dev1Set.id);

	dev1Set.csvFile.Format("날짜,%s,,장치ID,%d,공정, OCV\n", GetTimeStamp(1), dev1Set.id);

	addText.Format(",,채널 1,,\n");
	dev1Set.csvFile.Append(addText);

	if (dev1Set.type == _5V_20A_PACK_TYPE) {
		addText.Format("Time,Cnt,step,상태,전압,전류,용량,WattH,시간,종료,에러,온도,P-Bat,P-SOC,P-Volt,P-Cur,P-온도\n");
	}
	else {
		addText.Format("Time,Cnt,step,상태,전압,전류,용량,WattH,시간,종료,에러,온도,기타\n");
	}

	dev1Set.csvFile.Append(addText);

	m_EditCsvName.SetWindowTextA(dev1Set.csvFileName);

}

void CDisChgTesterDlg::OnBnClickedCheckAutoSend()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.

	dev1Set.autoTempReq = ((CButton*)GetDlgItem(IDC_CHECK_AUTO_SEND))->GetCheck();
	if (dev1Set.autoTempReq == true) {
		dev1Set.SensingReq ^= 1;
		CString msg;
		if (dev1Set.SensingReq) {
			msg.Format("채널보드 5초 타임아웃 방지로 온도/센싱값 요청함!\nSensing %d", dev1Set.SensingReq);
		}
		else
		{
			msg.Format("채널보드 5초 타임아웃 방지로 온도값 요청함!\nSensing %d", dev1Set.SensingReq);
		}
		AfxMessageBox(msg, MB_ICONSTOP);
	}
}

void CDisChgTesterDlg::OnBnClickedCheckAutoMsr()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	dev1Set.autoMsrReq = ((CButton*)GetDlgItem(IDC_CHECK_AUTO_MSR))->GetCheck();
	d_msg("auto send %d\n", dev1Set.autoMsrReq);
	if (dev1Set.autoMsrReq == true && dev1Set.type != _TEST_DUTY_TYPE) {
		CString msg = "채널보드 주기적인 전송이 없을 경우 사용!\n\r(Time-Out 방지는 비활성화 해도 됨!)";
		AfxMessageBox(msg, MB_ICONSTOP);
	}
	dev1Set.msrTime = GetDlgItemInt(IDC_EDIT_CH1_MSR_TIME);
}

void CDisChgTesterDlg::OnBnClickedBtnCur()
{
	//공정 중 CV 진입
	CString	strTemp, strDuty;
	char txData[8] = { 0, };
	float* pFloat;
	CString msg, curStr, voltStr;
	int32_t data = 0;
	if (dev1Set.type == _TEST_DUTY_TYPE)
	{
		if (dev1Set.testModeType == TEST_DUTY_160V60A)
		{
			GetDlgItemText(IDC_EDIT_CH1_POWER, strDuty);
			float getDuty = (float)atof(strDuty);
			if (getDuty > 80.0f || getDuty < 3.0f) {
				SetInfoMsg("duty 범위 초과, 3~80% 까지 가능.");
				return;
			}
			txData[0] = 0x20; 
			pFloat = (float*)&txData[4];
			*pFloat = (float)getDuty;			
			SendCAN_CmdData(dev1Set.id, CAN_TEST_MODE_CMD, txData, 8);		
			strTemp.Format("제어기 ID %d Set Duty %.3f", dev1Set.id, getDuty);
		}
		else {
			int getData = GetDlgItemInt(IDC_EDIT_CH1_POWER);
			if (!adminMode)
			{
				if (getData > dev1Set.dutyMax) {
					CString msg;
					msg.Format("설정 최대범위 초과 [%d]\r\n관리자 아닌 경우 제한됨!", dev1Set.dutyMax);
					AfxMessageBox(msg, MB_ICONSTOP);
					return;
				}
			}
			uint16_t pwm = getData * (FULL_DUTY / dev1Set.dutyRange);
			txData[0] = 0x20;
			memcpy(&txData[1], &pwm, sizeof(uint16_t));
			SendCAN_CmdData(dev1Set.id, CAN_TEST_MODE_CMD, txData, 3);
			//SendControlCmd(dev1Set.id, CAN_TEST_MODE_CMD);
			strTemp.Format("제어기 ID %d Set %d Duty %d", dev1Set.id, getData, pwm);
			

		}
		SetInfoMsg(strTemp);
		return;
	}
	else if (dev1Set.type == _100V_500A_TYPE || dev1Set.type == _5V_200A_TYPE)
	{
		// 센싱값
		char txData[8] = { 0, };
		SendCAN_CmdData(dev1Set.id, CAN_SENSING_CHECK_CMD, txData, 0);
		return;

	}
	else if (dev1Set.type == _160V_60A_32CH_TYPE) 
	{

		GetDlgItemText(IDC_EDIT_CH1_STEP, curStr);
		data = (int)((float)atof(curStr) * MSR_UNIT);

		//dev1Set.startVolt = (int)(((float)atof(cellvoltStr) * 100) + 0.5f);
		txData [0] = 1;
		memcpy(&txData[4], &data, sizeof(int32_t));
		SendCAN_CmdData(dev1Set.id, CAN_PATTERN_UPDATE_CMD, txData, 8);
		msg.Format("전압 갱신 %d mV", data);
		SetInfoMsg(msg);
		return;
	}

	if (dev1Set.type != _74V_80A_TYPE) {
		CString msg = "해당 기능 지원 안됨!";
		AfxMessageBox(msg, MB_ICONSTOP);
		return;
	}
	GetDevSetData(DEV_1, 0);
	if (dev1Set.endCur == 0 || dev1Set.endCur >= dev1Set.cur) {
		CString msg = "CV 종료전류가 없거나 설정 전류보다 높음! ";
		AfxMessageBox(msg, MB_ICONSTOP);
		return;
	}
	SendControlCmd(dev1Set.id, CAN_START_CV_CMD);
}

void CDisChgTesterDlg::OnBnClickedBtnPatn()
{	
	if (dev1Set.type == _160V_60A_32CH_TYPE)
	{
#ifndef _DEBUG
		CString msg = "[Cell Pass 명령 확인!]\r\n배터리 지그가 연결 상태인가?";
		int ret = AfxMessageBox(msg, MB_YESNO);
		
		if(ret == IDYES)		SendControlCmd(dev1Set.id, CAN_CELL_BYPASS_CMD);
		else SetInfoMsg("Cmd Cancel..");
#else
		SendControlCmd(dev1Set.id, CAN_CELL_BYPASS_CMD);
#endif
		return;
	}
	else {
		SetInfoMsg("Not support..");	// 사용안함..  패턴 선택후 시작으로 사용됨.
		return;
	}
	OnBnClickedBtnListClear();
	GetDevSetData(DEV_1, 0);

	if (dev1Set.type != _100V_500A_TYPE && dev1Set.type != _5V_200A_TYPE) {
		SetInfoMsg("Not support Patten");
		return;
	}

	if (dev1Set.opCmd != DEV_PATTERN_MODE) {
		SetInfoMsg("Not Patten Cmd");
		return;
	}
	if (dev1Set.opModeCmd != CC_MODE && dev1Set.opModeCmd != CP_MODE) {
		SetInfoMsg("chk. CC CP mode");
		return;
	}

	SendControlCmd(dev1Set.id, CAN_START_PATTERN_CMD);
	dev1Set.start = true;


}


void CDisChgTesterDlg::OnBnClickedBtnChgCur()
{
	//uint8_t gpio = dev1Set.getGpio1;
	char txData[8] = { 0, };
	CString msg, curStr, voltStr;
	int32_t data = 0;
	if (dev1Set.type == _TEST_DUTY_TYPE) {
		Setgpio = dev1Set.getGpio1;
		Setgpio ^= 1;
		dev1Set.txCanData[0] = 0x31;
		dev1Set.txCanData[1] = Setgpio;
		dev1Set.CanCmd = CAN_TEST_MODE_CMD;
		dev1Set.CanSenLen = 3;
		sendBtnCanTimer = 3;
		return;
	}

	if (dev1Set.type == _160V_60A_32CH_TYPE)
	{
		//if (dev1Set.start == false) return;
		GetDlgItemText(IDC_EDIT_CH1_CUR, curStr);
		data = (int)((float)atof(curStr) * MSR_UNIT);
		txData[0] = 2;
		//memcpy(&txData[0], &dev1Set.endTime, sizeof(uint32_t));
		memcpy(&txData[4], &data, sizeof(int32_t));
		SendCAN_CmdData(dev1Set.id, CAN_PATTERN_UPDATE_CMD, txData, 8);
		msg.Format("전류 갱신 %d mA",  data);
		SetInfoMsg(msg);
		return;
	}


	if (dev1Set.type == _5V_200A_TYPE || dev1Set.type == _100V_500A_TYPE)
	{
		if (dev1Set.start == false && dev1Set.opCmd != DEV_PATTERN_MODE) return;
		GetDlgItemText(IDC_EDIT_CH1_CUR, curStr);
		data = (int)((float)atof(curStr) * MSR_UNIT);
		memcpy(&txData[0], &dev1Set.endTime, sizeof(uint32_t));
		memcpy(&txData[4], &data, sizeof(int32_t));
		SendCAN_CmdData(dev1Set.id, CAN_PATTERN_UPDATE_CMD, txData, 8);
		msg.Format("패턴 갱신 %d ms, %d", dev1Set.endTime, data);
		SetInfoMsg(msg);
		return;
	}
	else {
		if (dev1Set.type == _5V_20A_PACK_TYPE)		// 전압, 전류 갱신 명령.
		{
			GetDlgItemText(IDC_EDIT_CH1_VOLT, voltStr);
			GetDlgItemText(IDC_EDIT_CH1_CUR, curStr);
			dev1Set.volt = (int)((float)atof(voltStr) * MSR_UNIT);
			dev1Set.cur = (int)((float)atof(curStr) * MSR_UNIT);
			memcpy(&txData[0], &dev1Set.volt, sizeof(uint32_t));
			data = GetDlgItemInt(IDC_EDIT_CH1_VOLT) * MSR_UNIT;
			memcpy(&txData[4], &dev1Set.cur, sizeof(int32_t));
			SendCAN_CmdData(dev1Set.id, CAN_PATTERN_UPDATE_CMD, txData, 8);
			msg.Format("전압 %dmV, 전류 %dmA 갱신", dev1Set.volt, dev1Set.cur);
			SetInfoMsg(msg);
			return;
		}
	}

	if (dev1Set.type != _74V_80A_TYPE) {
		CString msg = "해당 기능 지원 안됨!";
		AfxMessageBox(msg, MB_ICONSTOP);
		return;
	}
	GetDevSetData(DEV_1, 0);
	/*if (dev1Set.endCur == 0 || dev1Set.endCur >= dev1Set.cur) {
		CString msg = "CV 종료전류가 없거나 설정 전류보다 높음! ";
		AfxMessageBox(msg, MB_ICONSTOP);
		return;
	}*/
	SendControlCmd(dev1Set.id, CAN_CHANGE_CUR_CMD);

}


// Can 명령과 데이터 전송용도
int CDisChgTesterDlg::SendCAN_CmdData(long id, uint8_t cmd, char* data, uint8_t len)
{
	// TODO: 여기에 구현 코드 추가.
	if (openSuccess == false) return -1;
	struct Exid canid = { 0, };
	//char txData[8] = { 0x00, };  // 8byte * 3
	canid.id = id;
	canid.cmd = cmd;
	SendCAN_Data(len, &data[0], canid.all);
	d_msg("SendCAN_CmdData id %d cmd[%02X] data [", id, cmd);
	for (int i = 0; i < len; i++) { d_msg(" %2.2X", (uint8_t)data[i]); }
	d_msg(" ]\n");
	//sendBtnCanTimer = 3;
	return 0;
}

// 100ms
void CALLBACK TimeProc(UINT m_nTimerID, UINT uiMsg, DWORD dwUser, DWORD dw1, DWORD d2)
{
	CDisChgTesterDlg* pMain = (CDisChgTesterDlg*)dwUser;
	static uint8_t sendId = 1;
	static uint8_t sendCnt_id = 1;
	int sendTimer = (dev1Set.msrTime / 100);  // 측정요청 데이터 ms
	//if (sendTimer == 10) sendTimer = 9;		// 1초 경우 100ms 전후 오차 발생되어 900ms 로 적용.
//if (sendBtnCanTimer)	sendBtnCanTimer--;	// 버튼 전송
//int send2Sec = sendTimer/
	if (sendTimer >= 1) {
		send100msTimer++;
		if (send100msTimer % sendTimer == 0) {
			if (dev1Set.autoMsrReq) pMain->OnBnClickedBtnT9();
			else {
				//if (dev1Set.autoTempReq) pMain->SendControlCmd(dev1Set.id, CAN_READ_TEMP_CMD);   // 통신 타임 방지용.
			}

			/*	if (dev1Set.sendPattenCnt) {
					pMain->SendControlCmd(dev1Set.id, CAN_PATTERN_UPDATE_CMD);
					dev1Set.sendPattenCnt--;
				}*/
				//d_msgl("sendBtnCanTimer %d send100msTimer %d\n", sendBtnCanTimer, send100msTimer);					
				//send100msTimer = 0;
		}
		else {
			if (sendBtnCanTimer && send100msTimer > 1) {  // 자동 전송과 겹치지 않게 전송.
				pMain->SendCAN_CmdData(dev1Set.id, dev1Set.CanCmd, dev1Set.txCanData, dev1Set.CanSenLen);
				sendBtnCanTimer = 0;
			}
		}
	}
	dev1Set.updateUICnt++;  // UI 갱신 주기 300ms

	// 8채널 번갈아 가면서 명령
	if (dev1Set.type == _5V_20A_PACK_TYPE)
	{
		if (send100msTimer % 4 == 0) //400ms 주기.
		{
			if (dev1Set.SensingReq) {
				pMain->SendControlCmd(sendId, CAN_READ_TEMP_CMD);
				//sendId++;
				if (sendId >= 8) sendId = 1;
				else sendId++;
			}
		}

		if (dev1Set.autoDataReq) {
			if (sendCnt_id <= 8) pMain->SendControlCmd(sendCnt_id, CAN_READ_PACK_DATA_CMD);
		}
		// 100ms 주기로 10회 cnt
		if (sendCnt_id >= 10) sendCnt_id = 1;
		else sendCnt_id++;
	}
}
#ifndef USE_RCV_THREAD
void CALLBACK TimeRcvProc(UINT m_nTimerID, UINT uiMsg, DWORD dwUser, DWORD dw1, DWORD d2)
{
#define MAX_CNT			64
#define CHK_ID          0xFFFFF00
	//const int PCAN_RCV_CHK_TIMER = 6;
	//const int MWCAN_RCV_CHK_TIMER = 6;
	const int RCV_CHK_TIMER = 6;
	CDisChgTesterDlg* pMain = (CDisChgTesterDlg*)dwUser;

	TPCANMsg CANMsg;
	TPCANTimestamp CANTimeStamp;
	TPCANStatus stsResult = PCAN_ERROR_QRCVEMPTY;
	long id;
	int len, ext, rtr;
	char data[8] = { 0, };
	int chklen = 3;
	if (dev1Set.msrTime < 500) rcvCAN_chkTimer = 20;  //300ms
	else rcvCAN_chkTimer = 15;		//150ms

	if (dev1Set.type == _74V_80A_TYPE)	chklen = 4;
	else chklen = 3;

	while (1)
	{
		if (!openSuccess) {
			d_msg("openSuccess %d\n", openSuccess);
			return;
		}
		static int rcvCnt = 0, endCnt = 0;// parserCnt = 0, parserDataCnt = 0;
		static long msgHead[2] = { 0, };				// id , 전체 수신갯수		
		static Msg msg[MAX_CNT] = { 0, };
		bool rxOk = false;
		long chkid = 0, nextid = 0;
		long filterId = 0;
		static uint8_t SendData[512] = { 0, };
		uint32_t SendCnt = 0;
		static int chkcnt = 0;
		bool RecvEn = false;

		if (!pMain->m_isUpdateFW)
		{
			if (CanRcvCnt == 0)
			{
				try
				{
					do
					{
						if (pMain->m_CanDevType == USB_PCAN)		stsResult = pMain->m_objPCANBasic->Read(pMain->m_PcanHandle, &CANMsg, &CANTimeStamp);
						else if (pMain->m_CanDevType == MW_USB2CAN)	RecvEn = CAN_Recv(hBus, &id, &len, data, &ext, &rtr);
						else return;

						if (stsResult != PCAN_ERROR_QRCVEMPTY || RecvEn)
						{
							if (rcvCnt >= MAX_CNT - 2) {
								rcvCnt = 0;
							}
							if (pMain->m_CanDevType == USB_PCAN)
							{
								id = CANMsg.ID;
								len = CANMsg.LEN;
								for (int i = 0; i < len; i++)	data[i] = CANMsg.DATA[i];
							}

							if (dev1Set.id == id >> 20) // 동일 ID 라면 수신 처리 진행
							{
								for (int i = 0; i < len; i++) {
									msg[rcvCnt].data[i] = data[i];	// 수신데이터 저장.
								}
								msg[rcvCnt].id = id;
								msg[rcvCnt].len = len;
								rcvCnt++;
								//endCnt = PCAN_RCV_CHK_TIMER;
								if (((id >> 12) & 0x0FF) == (CAN_READ_DATA_CMD + CAN_ACK))	chkcnt++;
								endCnt = rcvCAN_chkTimer;
								d_msgl("## endCnt %d rcvCnt %d chkcnt %d", endCnt, rcvCnt, chkcnt);
							}
							//endCnt = rcvCAN_chkTimer;

							//if (chkcnt % chklen == 0) // 측정데이터 기준 3회 처리.
							//{
							//	//chkcnt = 0;
							//	//ResultCnt = 0;
							//	endCnt = 1;
							//}

							DspCanData(id, data);
						}
					} while ((stsResult & PCAN_ERROR_QRCVEMPTY) != PCAN_ERROR_QRCVEMPTY || RecvEn);

				}
				catch (int e)
				{
					d_msg("CAN_Recv Err %d", e);
				}

				if (endCnt == 1)
				{
					//pMain->SendMessageA(WM_USER_CAN_RCV_ALL_MSG, (WPARAM)msgHead, (LPARAM)msgData); // CAN 전체 수신 완료후 전달.

				/*	for (int rcv = 0; rcv < rcvCnt; rcv++) {
						d_msg("%s %0.8X", GetTimeStamp(4), msg[rcv].id);
						for (int i = 0; i < msg[rcv].len; i++) {
							d_msg(" %2.2X", (uint8_t)msg[rcv].data[i]);
						}
						d_msg("\n");
					}*/
					try {
						sort(msg, msg + rcvCnt, compare);
					}
					catch (int e)
					{
						d_msg("CAN_Recv Err %d", e);
					}
					/*d_msgl("=== sort ===");
					for (int rcv = 0; rcv < rcvCnt; rcv++) {
						d_msg("%s %0.8X", GetTimeStamp(4), msg[rcv].id);
						for (int i = 0; i < msg[rcv].len; i++) {
							d_msg(" %2.2X", (uint8_t)msg[rcv].data[i]);
						}
						d_msg("\n");
					}*/

					for (int i = 0; i < rcvCnt; i++) {
						chkid = msg[i].id;
						filterId = chkid & CHK_ID;
						if (i < MAX_CNT - 1) nextid = msg[i + 1].id;
						if ((chkid & CHK_ID) == filterId) {
							if (pMain->m_DspCAN > 1) d_msgl("id %lx %lx", chkid, nextid);
							for (int j = 0; j < msg[i].len; j++) SendData[SendCnt++] = (char)msg[i].data[j];	// 수신데이터 저장.
							if ((nextid & CHK_ID) != filterId) {
								//dbgCnt++;
							/*	printf("[%d]Send Data => ID %lx Cnt %d [", dbgCnt, chkid, SendCnt);
								for (int i = 0; i < SendCnt; i++) printf("%x ", SendData[i]);
								printf("]\n");*/
								msgHead[0] = chkid;
								msgHead[1] = SendCnt;
								pMain->SendMessageA(WM_USER_CAN_RCV_ALL_MSG, (WPARAM)msgHead, (LPARAM)SendData); // CAN 전체 수신 완료후 전달.
								//pMain->OnUpdateRcvData((WPARAM)msgHead, (LPARAM)SendData);  // SendMessageA 가 연속으로 처리 되지 않아 직접 적용.
								SendCnt = 0;

							}
							//if (i == MAX_CNT - 1 && (nextid & CHK_ID) == filterId) {
							//	printf("Send Data2 => ID %lx Cnt %d [", chkid, SendCnt);
							//	for (int i = 0; i < SendCnt; i++) printf("%x ", SendData[i]);
							//	printf("]\n");
							//	msgHead[0] = chkid;
							//	msgHead[1] = SendCnt;
							//	pMain->SendMessageA(WM_USER_CAN_RCV_ALL_MSG, (WPARAM)msgHead, (LPARAM)SendData); // CAN 전체 수신 완료후 전달.
							//	SendCnt = 0;								
							//}							
						}
					}
					rcvCnt = 0;
					endCnt = 0;
					chkcnt = 0;
					memset(msgHead, 0, sizeof(msgHead));
					memset(SendData, 0, sizeof(SendData));
					memset(msg, 0, sizeof(msg));
				}
			}
		}

		if (endCnt) {
			endCnt--;
			d_msgl("endCnt %d rcvCnt %d chkcnt %d", endCnt, rcvCnt, chkcnt);
		}
		Sleep(1);	// 1ms 로 하면 패킷 수신누락있음??..	
	}
	return;
}

#if 0
void CALLBACK TimeRcvProc(UINT m_nTimerID, UINT uiMsg, DWORD dwUser, DWORD dw1, DWORD d2)
{
#define MAX_CNT			50
#define CHK_ID          0xFFFFF00
	CDisChgTesterDlg* pMain = (CDisChgTesterDlg*)dwUser;
	//while (m_isRecvCANThread)
	if (openSuccess)
	{
		long id;
		int len, ext, rtr;
		char data[8] = { 0, };
		static int rcvCnt = 0, endCnt = 0;// parserCnt = 0, parserDataCnt = 0;
		static long msgHead[2] = { 0, };				// id , 전체 수신갯수
		//static char msgData[4096] = { 0, };			// 전체 수신 데이터.
		static Msg msg[MAX_CNT] = { 0, };
		bool rxOk = false;
		long chkid = 0, nextid = 0;
		long filterId = 0;
		static uint8_t SendData[256] = { 0, };
		uint32_t SendCnt = 0;
		static int dbgCnt = 0;
		if (!pMain->m_isUpdateFW)
		{
			//CanRcvCnt--;
			if (CanRcvCnt == 0)
			{
				while (CAN_Recv(hBus, &id, &len, data, &ext, &rtr))
				{
					if (rcvCnt == 0) {
						memset(msgHead, 0, sizeof(msgHead));
						memset(SendData, 0, sizeof(SendData));
						memset(msg, 0, sizeof(msg));
						dbgCnt = 0;
					}

					for (int i = 0; i < len; i++) msg[rcvCnt].data[i] = data[i];	// 수신데이터 저장.
					msg[rcvCnt].id = id;
					msg[rcvCnt].len = len;

					endCnt = CAN_RCV_CHK_TIMER;  //  수신 대기 타이머..갱신.	
					rcvCnt++;
					if (rcvCnt >= MAX_CNT - 2) {
						rcvCnt = 0;
					}
				}


#ifdef DEBUG
				DspCanData(id, data);

#endif // DEBUG


				if (endCnt == 1)
				{
					//pMain->SendMessageA(WM_USER_CAN_RCV_ALL_MSG, (WPARAM)msgHead, (LPARAM)msgData); // CAN 전체 수신 완료후 전달.

				/*	for (int rcv = 0; rcv < rcvCnt; rcv++) {
						d_msg("%s %0.8X", GetTimeStamp(4), msg[rcv].id);
						for (int i = 0; i < msg[rcv].len; i++) {
							d_msg(" %2.2X", (uint8_t)msg[rcv].data[i]);
						}
						d_msg("\n");
					}*/
					sort(msg, msg + rcvCnt, compare);
					/*d_msgl("=== sort ===");
					for (int rcv = 0; rcv < rcvCnt; rcv++) {
						d_msg("%s %0.8X", GetTimeStamp(4), msg[rcv].id);
						for (int i = 0; i < msg[rcv].len; i++) {
							d_msg(" %2.2X", (uint8_t)msg[rcv].data[i]);
						}
						d_msg("\n");
					}*/

					for (int i = 0; i < rcvCnt; i++) {
						chkid = msg[i].id;
						filterId = chkid & CHK_ID;
						if (i < MAX_CNT - 1) nextid = msg[i + 1].id;
						if ((chkid & CHK_ID) == filterId) {
							//printf("id %lx %lx\n", chkid, nextid);
							for (int j = 0; j < msg[i].len; j++) SendData[SendCnt++] = (char)msg[i].data[j];	// 수신데이터 저장.
							if ((nextid & CHK_ID) != filterId) {
								dbgCnt++;
								/*	printf("[%d]Send Data => ID %lx Cnt %d [", dbgCnt, chkid, SendCnt);
									for (int i = 0; i < SendCnt; i++) printf("%x ", SendData[i]);
									printf("]\n");*/
								msgHead[0] = chkid;
								msgHead[1] = SendCnt;
								pMain->SendMessageA(WM_USER_CAN_RCV_ALL_MSG, (WPARAM)msgHead, (LPARAM)SendData); // CAN 전체 수신 완료후 전달.
								SendCnt = 0;

							}
							//if (i == MAX_CNT - 1 && (nextid & CHK_ID) == filterId) {
							//	printf("Send Data2 => ID %lx Cnt %d [", chkid, SendCnt);
							//	for (int i = 0; i < SendCnt; i++) printf("%x ", SendData[i]);
							//	printf("]\n");
							//	msgHead[0] = chkid;
							//	msgHead[1] = SendCnt;
							//	pMain->SendMessageA(WM_USER_CAN_RCV_ALL_MSG, (WPARAM)msgHead, (LPARAM)SendData); // CAN 전체 수신 완료후 전달.
							//	SendCnt = 0;								
							//}

						}
					}
					//d_msgl("endCnt %d", endCnt);
					rcvCnt = 0;
					endCnt = 0;

				}
			}

		}
		if (endCnt) endCnt--;
		//Sleep(CAN_RCV_TIME_THR);
	}
	return;
}
#endif
#endif


void CDisChgTesterDlg::OnBnClickedBtnSetParm()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.

	SetDlg  Dlg;
	//memcpy(&Dlg.setParam, &pMainFrame->m_setDev, sizeof(Dlg.setParam));

	int ret = Dlg.DoModal();
	if (IDOK == ret) {
	}
#if 0
	TPCANMsg CANMsg;
	TPCANTimestamp CANTimeStamp;
	TPCANStatus stsResult;

	// We execute the "Read" function of the PCANBasic
	//
	stsResult = m_objPCANBasic->Read(m_PcanHandle, &CANMsg, &CANTimeStamp);

	if (stsResult != PCAN_ERROR_QRCVEMPTY) {
		// We process the received message
		//       
		ProcessMessage(CANMsg, CANTimeStamp);
		//d_msg("%ld\r\n", (CANTimeStamp.micros + 1000 * CANTimeStamp.millis + 0x100000000 * 1000 * CANTimeStamp.millis_overflow));
		d_msg("CANMsg [%d] ID %lx ", CANMsg.LEN, CANMsg.ID);
		for (int i = 0; i < CANMsg.LEN; i++)
			d_msg("%x ", CANMsg.DATA[i]);
		d_msg("\n");
}
#endif
	//return stsResult;

}


void CDisChgTesterDlg::OnBnClickedBtnSetProtec()
{
	// 5V200A 싸이클러만 우선 지원..
	if (dev1Set.type != _5V_200A_TYPE && dev1Set.type != _100V_500A_TYPE && dev1Set.type != _160V_60A_32CH_TYPE) {
		SetInfoMsg("not used..");
		return;
	}
	uint16_t dropVolt = GetDlgItemInt(IDC_EDIT_CH_DROP_VOLT);

	if (dev1Set.type == _100V_500A_TYPE) {
		protectSet.voltage_max = 120000;
		protectSet.current_max = 410000;	
		memcpy(&protectSet.voltage_vout_drop, &dropVolt, sizeof(uint16_t));
	}
	else {
		if (dropVolt > 255) dropVolt = 255;
		protectSet.voltage_vout_drop = dropVolt;
	}
	SendControlCmd(dev1Set.id, CAN_PROTECT_DATA_CMD);
}

CString CDisChgTesterDlg::FormatChannelName(TPCANHandle handle)
{
	// TODO: 여기에 구현 코드 추가.
	return FormatChannelName(handle, false);
}


CString CDisChgTesterDlg::FormatChannelName(TPCANHandle handle, bool isFD)
{
	// TODO: 여기에 구현 코드 추가.
	CString result;
	BYTE byChannel;

	// Gets the owner device and channel for a
	// PCAN-Basic handle
	//
	if (handle < 0x100)
		byChannel = (BYTE)(handle) & 0xF;
	else
		byChannel = (BYTE)(handle) & 0xFF;

	// Constructs the PCAN-Basic Channel name and return it
	//
	result.Format(isFD ? "%s:FD %d (%Xh)" : "%s %d (%Xh)", GetTPCANHandleName(handle), byChannel, handle);
	return result;
}


CString CDisChgTesterDlg::GetTPCANHandleName(TPCANHandle handle)
{
	// TODO: 여기에 구현 코드 추가.
	CString result = "PCAN_NONE";
	switch (handle)
	{
	case PCAN_ISABUS1:
	case PCAN_ISABUS2:
	case PCAN_ISABUS3:
	case PCAN_ISABUS4:
	case PCAN_ISABUS5:
	case PCAN_ISABUS6:
	case PCAN_ISABUS7:
	case PCAN_ISABUS8:
		result = "PCAN_ISA";
		break;

	case PCAN_DNGBUS1:
		result = "PCAN_DNG";
		break;

	case PCAN_PCIBUS1:
	case PCAN_PCIBUS2:
	case PCAN_PCIBUS3:
	case PCAN_PCIBUS4:
	case PCAN_PCIBUS5:
	case PCAN_PCIBUS6:
	case PCAN_PCIBUS7:
	case PCAN_PCIBUS8:
	case PCAN_PCIBUS9:
	case PCAN_PCIBUS10:
	case PCAN_PCIBUS11:
	case PCAN_PCIBUS12:
	case PCAN_PCIBUS13:
	case PCAN_PCIBUS14:
	case PCAN_PCIBUS15:
	case PCAN_PCIBUS16:
		result = "PCAN_PCI";
		break;

	case PCAN_USBBUS1:
	case PCAN_USBBUS2:
	case PCAN_USBBUS3:
	case PCAN_USBBUS4:
	case PCAN_USBBUS5:
	case PCAN_USBBUS6:
	case PCAN_USBBUS7:
	case PCAN_USBBUS8:
	case PCAN_USBBUS9:
	case PCAN_USBBUS10:
	case PCAN_USBBUS11:
	case PCAN_USBBUS12:
	case PCAN_USBBUS13:
	case PCAN_USBBUS14:
	case PCAN_USBBUS15:
	case PCAN_USBBUS16:
		result = "PCAN_USB";
		break;

	case PCAN_PCCBUS1:
	case PCAN_PCCBUS2:
		result = "PCAN_PCC";
		break;

	case PCAN_LANBUS1:
	case PCAN_LANBUS2:
	case PCAN_LANBUS3:
	case PCAN_LANBUS4:
	case PCAN_LANBUS5:
	case PCAN_LANBUS6:
	case PCAN_LANBUS7:
	case PCAN_LANBUS8:
	case PCAN_LANBUS9:
	case PCAN_LANBUS10:
	case PCAN_LANBUS11:
	case PCAN_LANBUS12:
	case PCAN_LANBUS13:
	case PCAN_LANBUS14:
	case PCAN_LANBUS15:
	case PCAN_LANBUS16:
		result = "PCAN_LAN";
		break;
	}
	return result;
}


DWORD CDisChgTesterDlg::HexTextToInt(CString ToConvert)
{
	// TODO: 여기에 구현 코드 추가.
	DWORD iToReturn = 0;
	int iExp = 0;
	char chByte;

	// The string to convert is empty
	//
	if (ToConvert == "")
		return 0;
	// The string have more than 8 character (the equivalent value
	// exeeds the DWORD capacyty
	//
	if (ToConvert.GetLength() > 8)
		return 0;
	// We convert any character to its Upper case
	//
	ToConvert = ToConvert.MakeUpper();

	try
	{
		// We calculate the number using the Hex To Decimal formula
		//
		for (int i = ToConvert.GetLength() - 1; i >= 0; i--) {
			chByte = ToConvert[i];
			switch (int(chByte)) {
			case 65:
				iToReturn += (DWORD)(10 * pow(16.0f, iExp));
				break;
			case 66:
				iToReturn += (DWORD)(11 * pow(16.0f, iExp));
				break;
			case 67:
				iToReturn += (DWORD)(12 * pow(16.0f, iExp));
				break;
			case 68:
				iToReturn += (DWORD)(13 * pow(16.0f, iExp));
				break;
			case 69:
				iToReturn += (DWORD)(14 * pow(16.0f, iExp));
				break;
			case 70:
				iToReturn += (DWORD)(15 * pow(16.0f, iExp));
				break;
			default:
				if ((int(chByte) < 48) || (int(chByte) > 57))
					return -1;
				iToReturn += (DWORD)(atoi(&chByte) * pow(16.0f, iExp));
				break;

			}
			iExp++;
		}
	}
	catch (...)
	{
		// Error, return 0
		//
		return 0;
	}

	return iToReturn;
}


void CDisChgTesterDlg::ProcessMessage(TPCANMsg theMsg, TPCANTimestamp itsTimeStamp)
{
	// TODO: 여기에 구현 코드 추가.
	TPCANMsgFD newMsg;
	TPCANTimestampFD newTimestamp;

	newMsg = TPCANMsgFD();
	newMsg.ID = theMsg.ID;
	newMsg.DLC = theMsg.LEN;
	for (int i = 0; i < ((theMsg.LEN > 8) ? 8 : theMsg.LEN); i++)
		newMsg.DATA[i] = theMsg.DATA[i];
	newMsg.MSGTYPE = theMsg.MSGTYPE;

	newTimestamp = (itsTimeStamp.micros + 1000 * itsTimeStamp.millis + 0x100000000 * 1000 * itsTimeStamp.millis_overflow);
	//ProcessMessage(newMsg, newTimestamp);
	//d_msg("%d %x\r\n ", newTimestamp, newMsg.ID);
	/*CString str;
	str.Format("stsResult %d %x\r\n", newTimestamp, newMsg.DLC);*/
	//IncludeTextMessage(str);
}

// PCAN 전송 함수.
int CDisChgTesterDlg::PCAN_Send(TPCANHandle Channel, long ID, int length, char data[64], int Ext, int RTR)
{
	// TODO: 여기에 구현 코드 추가.
	TPCANMsg CANMsg;
	CANMsg.ID = ID;
	CANMsg.LEN = (BYTE)length;

	if (Ext)	CANMsg.MSGTYPE = PCAN_MESSAGE_EXTENDED;
	else CANMsg.MSGTYPE = PCAN_MESSAGE_STANDARD;

	for (int i = 0; i < CANMsg.LEN; i++) CANMsg.DATA[i] = data[i];

	m_objPCANBasic->Write(Channel, &CANMsg);
	return 1;
}



void CDisChgTesterDlg::OnBnClickedBtnPack1()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	dev1Set.PackDataType = GetDlgItemInt(IDC_EDIT_PACK1);
	SendControlCmd(dev1Set.id, CAN_READ_PACK_DATA_CMD);
}


void CDisChgTesterDlg::OnBnClickedCheckAutoReq()
{
	if (dev1Set.type == _5V_20A_PACK_TYPE) {
		
		dev1Set.autoDataReq = ((CButton*)GetDlgItem(IDC_CHECK_AUTO_REQ))->GetCheck();
		//return;
	}
	else if (dev1Set.type == _160V_60A_32CH_TYPE) {
		dev1Set.bmsDataSave = ((CButton*)GetDlgItem(IDC_CHECK_AUTO_REQ))->GetCheck();		
	}
	else {
		SetInfoMsg("not Used..");
		return;
	}
	d_msg("bmsDataSave %d\n", dev1Set.bmsDataSave);
}


void CDisChgTesterDlg::OnBnClickedBtnPack2()
{

	uint16_t setType = GetDlgItemInt(IDC_EDIT_PACK1);
	//dev1Set.PackDataType = GetDlgItemInt(IDC_EDIT_PACK1);

	uint8_t smbusCmd[12] = { 0x16, 0x0D, 0x09, 0x0A, 0x08, 0x15, 0x14, 0x17, 0x10, 0xFF, 0xFF, 0xFF };


	dev1Set.PackBMS_CommType = 0x10;
	dev1Set.PackBMS_CommAddr = 0x55;
	dev1Set.PackDataCnt = 9;
	dev1Set.PackDataType = 1;
	if (setType == 0) {
		dev1Set.PackDataType = 0;
	}
	else if (setType == 1) {   // i2c
		/*	dev1Set.PackBMS_CommType = 0x10;
			dev1Set.PackBMS_CommAddr = 0x55;*/
		dev1Set.PackDataCnt = 12;
		//dev1Set.PackDataType = 1;
		for (int i = 0; i < 12; i++)
			dev1Set.PackReadCmd[i] = i * 2;
	}
	else if (setType == 2) {  // smbus
		dev1Set.PackBMS_CommType = 0x20;
		dev1Set.PackBMS_CommAddr = 0x0B;
		dev1Set.PackDataCnt = 9;
		//dev1Set.PackDataType = 1;
		for (int i = 0; i < 12; i++)
			dev1Set.PackReadCmd[i] = smbusCmd[i];
	}
	else if (setType == 100) {
		dev1Set.PackDataType = 100;
	}
	else {
		SetInfoMsg("not Used..");
		return;
	}
	SendControlCmd(dev1Set.id, CAN_SET_PACK_COM_CMD);

}


void CDisChgTesterDlg::OnBnClickedBtnPack3()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	CString strtemp;// , SendData, strhex;
	GetDlgItem(IDC_EDIT_PACK2)->GetWindowText(strtemp);
	dev1Set.PackUserCmdLen = GetDlgItemInt(IDC_EDIT_PACK3);
	dev1Set.PackUserReadLen = GetDlgItemInt(IDC_EDIT_PACK4);

	if (strtemp.GetLength() > 8) {
		SetInfoMsg("Over Cmd 4byte..");
		return;
	}
	else if (dev1Set.PackUserCmdLen > 4) {
		SetInfoMsg("Over Cmd length..");
		return;
	}
	else if (dev1Set.PackUserReadLen > 64) {
		SetInfoMsg("Over Read data length..");
		return;
	}

	dev1Set.PackUserCmd = strtoul(strtemp, NULL, 16);

	d_msg("val %d %x\n", dev1Set.PackUserCmd, dev1Set.PackUserCmd);
	SendControlCmd(dev1Set.id, CAN_SET_PACK_USER_DATA_CMD);
}


int CDisChgTesterDlg::SendCAN_DataEx(long id, int len, char* data, int ex)
{
	// ex 값이 1이면 확장, 0이면 표준.
	int ret = -1;
	if (openSuccess) {
		if (m_CanDevType == MW_USB2CAN)		ret = CAN_Send(hBus, id, len, data, ex, 0);
		else if (m_CanDevType == USB_PCAN)	ret = PCAN_Send(m_PcanHandle, id, len, data, ex, 0);
	}
	return ret;
}

void CDisChgTesterDlg::OnBnClickedCheck1k2k()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	static int cur = 0;
	CString strCur = "Set 200A";
	if (dev1Set.type != _5V_200A_TYPE) {
		SetInfoMsg("not Used..");
		return;
	}
	cur++;
	if (cur > 2) cur = 0;
	m_ComboCh1_ID.ResetContent();
	if (cur == 1) {
		strCur = "Set 1kA";
		char* idList2[10] = { "17", "18", "19", "20", "21" ,"33", "34", "35", "36", "37" };
		for (int i = 0; i < 10; i++)	m_ComboCh1_ID.AddString(idList2[i]);
		((CButton*)GetDlgItem(IDC_CHECK_GROUP))->SetCheck(1);
	}
	else if (cur == 2) {
		strCur = "Set 2kA";
		char* idList2[10] = { "17", "18", "19", "20", "21", "22", "23", "24", "25", "26" };
		for (int i = 0; i < 10; i++) m_ComboCh1_ID.AddString(idList2[i]);
		((CButton*)GetDlgItem(IDC_CHECK_GROUP))->SetCheck(1);

	}
	else {
		char* idList2[8] = { "17", "18", "19", "20", "33", "34", "35", "36" };
		for (int i = 0; i < 8; i++) m_ComboCh1_ID.AddString(idList2[i]);
		((CButton*)GetDlgItem(IDC_CHECK_GROUP))->SetCheck(0);
		GetDlgItem(IDC_STATIC_PACK_ITEM)->SetWindowText("#");
		GetDlgItem(IDC_STATIC_PACK_DATA)->SetWindowText("#");

	}

	((CButton*)GetDlgItem(IDC_CHECK_1K_2K))->SetWindowTextA(strCur);
	dev1Set.setCur = cur;
	d_msg("set cur %d\n", dev1Set.setCur);
	m_ComboCh1_ID.SetCurSel(0);
	GetDevSetData(DEV_1, 0);
	dev1Set.lastId = 0;
	memset(rcvMeasure, 0x00, sizeof(Rcv_Measure_t) * 20);
	SendControlCmd(dev1Set.id, CAN_TIME_SYNC_CMD);
}


void CDisChgTesterDlg::OnBnClickedCheckAutoStart()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	d_msg("auto start\n");
	CString msg;
	GetDevSetData(DEV_1, 0);		//다이얼로그 값 적용.
	if (m_ChkAutoStart.GetCheck()) {
		autoStartCnt = dev1Set.autoStartCnt;
		restTime = dev1Set.restTime;
		autoModeTog = dev1Set.autoModeTog;
		if (autoStartCnt == 0) {
			SetInfoMsg("자동 동작 설정이 안됨!");
			return;
		}
		msg.Format("자동 연속동작 횟수 %d회  /  휴식시간 %d초 / 모드반복 %d", autoStartCnt, restTime, autoModeTog);
		SetInfoMsg(msg);
		msg.Format("공정 자동 시작[%d/%d]회 / 모드반복 %d회", autoStartCnt, dev1Set.autoStartCnt, autoModeTog);
		SetInfoMsg(msg);
		OnBnClickedBtnCh1Start();
		m_ChkAutoStop.SetCheck(true);
		//AfxMessageBox(msg, MB_ICONSTOP);		
	}
	else {
		autoStartCnt = 0; restTime = 0;
		OnBnClickedBtnCh1Stop();
		SetInfoMsg("자동 동작 정지!");
		
		// 해제시 측정 상태 요청이 유지되어 연속적으로 정지를 보냄.
	/*	if (dev1Set.type == _160V_60A_32CH_TYPE) {
			((CButton*)GetDlgItem(IDC_CHECK_AUTO_MSR))->SetCheck(false);
			dev1Set.autoMsrReq = false;
		}*/
			
		m_ChkAutoStop.SetCheck(false);
	}
}


char* CDisChgTesterDlg::faultCodeMsg(int type, int code)
{
	// TODO: 여기에 구현 코드 추가.
	char* msg = "no Type";

	if (type == 0)
	{
		if (code <= 0x0E) msg = faultMsg[code];
		else {
			if		(code == 0x11) msg = "Phase1 OCP";
			else if (code == 0x12) msg = "Phase2 OCP";
			else if (code == 0x13) msg = "Phase3 OCP";
			else if (code == 0x14) msg = "Bias OVP";
			else if (code == 0x15) msg = "Bias UVP";
			else if (code == 0x20) msg = "Con-Cable";
			else if (code == 0x30) msg = "BMS Err";
			else if (code == 0x31) msg = "BMS Cell Err";
			else if (code == 0x40) msg = "Bypass Err";
			else if (code == 0xFE) msg = "ILLEGAL";
			else msg = "No msg";
		}
	}	
	return msg;
}



void CDisChgTesterDlg::OnCbnSelchangeComboCh1Mode()
{	
	if (dev1Set.type != _TEST_DUTY_TYPE) return;
	CString strPwm;
	dev1Set.testModeType = m_ComboCh1_Mode2.GetCurSel();
	if (dev1Set.testModeType == TEST_DUTY_NORMAL) {
		strPwm.Format("\t\t Duty [0~%d]", dev1Set.dutyRange);
		GetDlgItem(IDC_STATIC_CH3)->SetWindowText(strPwm);
		GetDlgItem(IDC_EDIT_CH1_POWER)->ModifyStyle(TRUE, ES_NUMBER);  // 숫자 입력 끄기.	
	}
	else if (dev1Set.testModeType == TEST_DUTY_160V60A) {		
		strPwm = "\t\tDuty [0~100%]";
		GetDlgItem(IDC_STATIC_CH3)->SetWindowText(strPwm);
		GetDlgItem(IDC_EDIT_CH1_POWER)->ModifyStyle(ES_NUMBER, FALSE);  // 숫자 입력 끄기.	
	}
	//d_msg("testModeType %d\n", dev1Set.testModeType);
}


void CDisChgTesterDlg::OnBnClickedBtnSet()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	SetChDlg  Dlg;
	uint32_t activeCh = 0;
	Dlg.setch1 = dev1Set.activeCh & 0x0000FFFF;
	Dlg.setch2 = (dev1Set.activeCh>>16) & 0x0000FFFF;
	
	//d_msg("activeCh %X %X %X\n", dev1Set.activeCh, Dlg.setch1, Dlg.setch2);

	//memcpy(&Dlg.setParam, &pMainFrame->m_setDev, sizeof(Dlg.setParam));

	int ret = Dlg.DoModal();
	if (IDOK == ret) {				
		//d_msg("chall %d ch1 %x  ch2 %x\n", Dlg.chkChAll, Dlg.setch1, Dlg.setch2);
		CString strCh;
		activeCh = (Dlg.setch2 << 16) | Dlg.setch1;
		SetDlgItemInt(IDC_EDIT_CH1_POWER, activeCh, FALSE);
		dev1Set.activeCh = activeCh;
		strCh.Format("%X", dev1Set.activeCh);		
		GetDlgItem(IDC_EDIT_CH1_POWER)->SetWindowText(strCh);		
		dev1Set.activeCmd1 = Dlg.setCmd1;             // 채널 제어 명령 옵션1		
		dev1Set.activeChCnt = Dlg.activechCnt;
		strCh.Format("Active Cnt %d, Ch %X cmd %x ", Dlg.activechCnt, dev1Set.activeCh, dev1Set.activeCmd1);
		SetInfoMsg(strCh);
		//d_msg("activeCh %X %X\n", activeCh, activeCh);
	}
	
}
