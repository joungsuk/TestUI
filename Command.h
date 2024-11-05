#pragma once
#pragma once
#ifndef __COMMAND_H__
#define __COMMAND_H__

//#define CYCLE_MODE
#define SET_BIT(data, loc)      ((data) |= (0x1 << (loc)))

// 명령관련..
struct Exid {
    union {
        struct {
            unsigned int cnt : 12;     // 프레임 순서
            unsigned int cmd : 8;     // 명령
            unsigned int id : 9;     // can id                        
        };                           // 합계 29비트
        unsigned int all;
    };
};

struct Msg
{
    long id;
    uint8_t len;
    char data[8];
};

struct DeviceSetParam {
    struct {
        long id = 0;
        uint8_t grpId = 0;      // 그룹.
        uint8_t opCmd = 0;   // 충전, 방전..
        uint8_t opMode = 0;   // CC, CV, CCCV, CP...
        uint8_t opModeCmd = 0;   // CC 0x11, CV 0x12, CCCV 0x13, CP 0x14, CPCV 0x16..
        uint8_t stepNum = 0;   // 시퀀스 넘버
        bool  parallelEn = false;   // 병렬 적용 여부
        uint16_t msrTime = 1000;   // 측정 주기
        uint8_t type = 0;          // 싸이클러 사양 및 종류
        uint8_t setCur = 0;     // 200, 1000, 2000A 확인.
        bool start = false;

        //공정 조건.
        int32_t volt = 0;
        int32_t dis_volt = 0;
        int32_t cur = 0;
        int32_t dis_cur = 0;
        int32_t power = 0;
        int32_t dis_power = 0;
    
        // 종료조건.
        int32_t endCur = 0;             // 충전 종지전류
        int32_t dis_endCur = 0;          // 방전 종지 전류
        uint32_t endTime = 0;           // 종료시간 설정.
        uint32_t dis_endTime = 0;           // 종료시간 설정.

        uint32_t endCap = 0;
        uint32_t dis_endCap = 0;
        uint32_t endWattH = 0;
        uint32_t dis_endWattH = 0;

        uint32_t endPower = 0;
        uint32_t dis_endPower = 0;

        uint32_t maxVolt = 0;           // 허용 최대 전압.
        //uint32_t endTimerCnt;          // 종료타이머
        uint8_t ch = 0;
        uint8_t maxChCnt = 0;               // 병렬시 최대 채널 갯수.
        uint8_t lastId = 0;                 // 병렬시 마지막 채널보드 확인.
        CString csvFileName = "";
        CString csvFile = "";
        CString csvGrpFileName = "";
        CString csvGrpFile = "";
        CString csvGrpData[20] = { "", };
        int stopCnt = 0;                // 시험 정지되더라도 설정 값 만큼 상태 요청함.  바로 정지하면 제어기 상태와 불일치함.
        int sendTimerCnt = 0;           // OCV, 접촉 검사등 결과 요청에 따른 시간지연이 필요할때 사용.
        int sendCmd = 0;
        bool startDelay = false;            // 시험중 다른 제어기 시작시 수신 타이밍 충돌되는 부분 지연처리.
        bool cycleMode = false;
        uint32_t cycleCnt = 0;
        uint32_t cycleRestTime = 0;
        uint32_t rcvChkTimer = 0;           // 처음 실행후 통신연결 후 수신 데이터 확인용.
        bool autoMsrReq = false;                // 주기적으로 측정데이터 요청..
        bool autoTempReq = false;               // 주기적으로 온도데이터 요청..
        bool autoRlyReq = false;                // 릴레이 순차적으로 제어 요청.
        bool autoDataReq = false;                // 자동 데이터 요청.
        bool bmsDataSave = false;               // BMS 데이터 저장 여부.
        bool dspErrInfo = false;                    // 정보창에 에러 표시 여부
        int dutyRange = 0;                  // PWM 조정 범위  0~100, 0~1000
        int dutyMax = 0;                    // PWM 최대 설정값 
        bool getGpio1 = false;                  // 보드 gpio 상태
        bool getGpio2 = false;
        bool getGpio3 = false;
        int getDuty = 0;
        char txCanData[8] = {  0, };
        uint8_t CanCmd = 0;
        uint8_t CanSenLen = 0;
        uint16_t sendPattenCnt = 0;         // 패턴 보내는 횟수
        uint32_t sendPattenTime = 0;        // 패턴 동작 시간 100~1000
        uint32_t sendPattenData = 0;        // 패턴 CC, CP 값.       
        bool SensingReq = false;
        uint16_t relayCmd = 0;              // daq 릴레이 제어
        uint16_t relayCmdTime = 0;          // daq 릴레이 제어 시간.
        uint8_t updateUICnt = 0;            // 화면 갱신 시간.  

        //  팩통신 설정 0x1C
        bool        PackDataSave = false;
        uint8_t     PackBMS_CommType = 0;    //통신 종류 None (0x00),  i2c (0x10), smbus (0x20)
        uint8_t     PackBMS_CommAddr = 0;    //통신 주소 None (0x00),  i2c (0x55), smbus (0x0B)
        uint8_t     PackDataType = 0;           // data type 설정.. 기본9항목(0x00),  사용자 설정항목(0x01)
        uint8_t     PackDataCnt = 0;           // 총 data 갯수  최대 12
        uint8_t     PackReadCmd[20] = { 0, };        // 설정 항목의 cmd 주소.

        //  내부 데이터 읽기 0x12
        uint32_t    PackUserCmd = 0;            // 사용자 cmd 주소. 4byte
        uint8_t     PackUserCmdLen = 0;         // cmd 길이 1~4byte
        uint8_t     PackUserReadLen = 0;        // read 응답 데이터 길이.

        uint32_t restTime;              // 자동 동작시 휴식 시간
        uint32_t autoStartCnt;          // 자동 동작 횟수
        uint32_t autoModeTog;           // 자동 동작 모드 반복횟수.
        uint32_t startVolt;             // 현재 배터리 전압...프리차징 위한 전압.
        uint32_t activeCh;              // 32채널 활성화 상태
        uint32_t activeChCnt;           // 32채널 갯수.
        uint8_t activeCmd1;             // 채널 제어 명령 옵션1

        uint8_t testModeType = 0;       // open loop 테스트 모드 장비 종류..
    };                                  
};

struct ContactParam {
    struct {
        uint16_t cur = 0;
        uint16_t time = 0;
        uint16_t recoverTime = 0;
        uint16_t passVolt = 0;
        uint16_t reverseVolt = 0;
        uint16_t dVoltMin = 0;
        uint16_t dVoltMax = 0;
    };
};

struct CycleParam {
    struct {
        uint8_t opMode = 0;
        int32_t volt = 0;
        int32_t cur = 0;
        int32_t endCur = 0;             // 충전 종지전류        
    };
};

typedef struct
{
    int32_t       voltage_max = 0;        //mV
    int32_t       voltage_min = 0;        //mV
    int32_t       current_max = 0;        //mA
    int32_t       current_min = 0;        //mA
    uint32_t      capacity_max = 0;       //mAh
    uint16_t      temp_bd_max = 0;        //0.1deg, battery temperature
    uint8_t       voltage_vout_drop = 0;  // mV
    uint8_t       spare = 0;  // mV
} SCH_Protection_t;


typedef struct
{
    int32_t       voltage_max = 0;        //mV
    int32_t       voltage_min = 0;        //mV
    int32_t       current_max = 0;        //mA
    int32_t       current_min = 0;        //mA
    uint16_t      cell_volt_max = 0;        //0.1deg, battery temperature
    uint16_t      cell_volt_min = 0;        //0.1deg, battery temperature
    uint16_t      temp_bd_max = 0;        //0.1deg, battery temperature
    uint16_t       voltage_vout_drop = 0;  // mV    
} SCH_Protection2_t;


typedef struct
{
    uint32_t opTime = 0;
    uint8_t opMode = 0;
    char* opModeMsg = "";
    float fOCV = 0;             // mV
    float fVoltage = 0;         // mV
    float fCurrent = 0;         // mA
    float fIPhase[4] = { 0 ,};  // mA    
    uint32_t Wh = 0;              // watth
    uint32_t Cap = 0;             //용량
    float fTemp = 0;            //Battery Temperature degree    
} Rcv_Measure_t;


/* 
0.0.1(22.01.17)  =>  충방전 테스트터 0.3.6(22.01.06) 베이스 코드로 싸이클러 테스트 프로그램 개발.

0.0.2(22.01.21)
 - 자동 상태요청 기능 추가.
 - 전압/전류 단위 수정.

 0.0.3(22.03.02)
 - CCCV 관려 UI 활성화.

 0.0.4(22.03.02)
 - 전압/전류 소수점 처리 추가.
 - UI 제어 수정

 0.0.5(22.03.04)
 - 상태 표시 관련 수정.
 

 0.0.6(22.03.04)
 - 방전 CCCV 관련 종료 전류 -처리 추가.
 - 방전시 종료 전류 조건 처리 추가.(endCur)

 0.0.7(22.04.12)
  - 자동 측정 요청 온도 요청으로변경.
  - CSV 저장 파일명 수정.  전압소수점 표시.
 
 0.0.8(22.05.26)
  - 사양 선택 기능 추가. 5v6A.
 
 0.0.9(22.05.31)
  - 단축키 추가. 
  id 변경 =>  Up/Down 키, 충/방전 선택 => space 키, 시작/정지 => enter 키
  F1 도움말 추가.

 0.1.0(22.06.27)
 - rest 버튼, 에러 표시 추가 (dspErrInfo)

 0.1.1
 - 타임아웃 방지 실시간 적용..(22.08.25)
 - 자동 상태 요청 기능 추가.  요청시간은 MSR Time 과 연동 (22.09.26)

  0.1.2
 - _74V_80A_TYPE(직렬형 충방전기) 추가
 - 자동 상태 요청 기능 추가.  요청시간은 MSR Time 과 연동 (22.09.26)

 Ver 0.1.3 (22.10.19)
  - 싸이클러 펌웨어 업데이트 추가.
  - CV 진입 명령 추가.

  Ver 0.1.4 (23.01.17)
  - Fw 버전 에러 추가.
  - Rest 저장 추가

  Ver 0.1.5 (23.01.30)
  - 가변 전류 명령 추가.

   Ver 0.1.6 (23.04.06)
  - PWM 테스트 모드 추가.
  

   Ver 0.1.7 (23.04.07)
  - 멀티미디어 타이머 적용.
  - GPIO 제어 추가.
  - 
   Ver 0.1.7 (23.04.14)
   - 테스트 모드 if 버그 수정..

    Ver 0.1.8 (23.04.24)
   - 측정 온도표시 추가, 상단 표시 간결화.
   - 수신 방식 수정 => 각 데이터 수신 후 정렬 처리 후 전달로 수정.
   - CAN CAN_SetTimeout 설정 삭제..이거때문에 가끔씩 누락 발생되는 것 같음..설명서에 자세한 내용은 없음..

    Ver 0.1.9 (23.05.12)
    - 패턴 동작 기능 추가.
    

    Ver 0.2.0 (23.06.01)
    - 버전 확장 체크시.. CP 최대 전류 옵션 처리 추가... 23.06.01
    - CP 입력창 활성..        
    - W 표시 추가.
    - 수신 스레드 0.1ms sleep 처리.
    - 200A 센싱 데이터 처리 추가

    Ver 0.2.1 (23.06.09)
    - 테스트 모드에서 CAN-Stop 처리 추가.
    - 테스트 모드에서 센싱확인 안되는 부분 수정..나중에 싸이클러 테스트모드와 구분 필요..
     # 06.27
    - 병렬 전압 선택 명령 추가.  SELECT_VOLT_CMD 로 숨김처리..

    Ver 0.2.2 (23.7.03)
    - 싸이클러 ID 적용
    - 주기적 요청에서 센싱 요청 토글 처리..
    (23.11.07)
    - CAN연결 해제 시 스레드 false 처리 추가. m_isRecvCANThread,  m_isSendCANThread
    - 펌웨어 업데이트 시 사용자 설정ID 와 기본ID(idList) 비교 후 다르면 마지막 인덱스에 해당 ID 추가 처리함.
       => 싸이클러 타입외 추가 단일 채널 충방전기 적용 위해 수정.

   Ver 0.2.3 (23.11.18)
    - 쓰레드 종료 처리 수정: 쓰레드 종료 sleep 후 Can port 를 닫음.
    - can 수신처리 수정: 일정갯수 이상되면 바로 파싱처리함.
    - CAN_PACKET_TEST 기능 추가 : 연속 전송 검증할때 사용.
    - UPDATE_UI_CNT 추가 : UI 데이터 300ms 마다 업데이트 처리  
    
    (23.11.29)
    - 디버깅 모드에서 CAN 데이터 확인위해 m_DspCAN 변수 추가

   Ver 0.2.4 (23.12.08)
    - 안전조건 설정 추가.
    - 전압선택 설정 추가.
    - 측정데이터 Cap 전압 저장 추가. (모듈라인 충방전 전용)

   Ver 0.2.5 (23.12.23)
   1) 12.18
    - can 스레드 수신 변경. 0.1ms 3회, if 처리.
   2) 12.23
    - 팩싸이클러 온도 자리 4자리 적용.

   Ver 0.2.6 
   [24.01.07]
   PCAN 장치 추가...펌웨어 업데이트 구현 필요.
   수신 방식 수정.. 50ms 기준으로 300ms 지연처리.
   canlog 에러 발생시 10초 까지 해당 로그 파일 저장.. 에러 없으면 60초마다 초기화.
   PCAN 경우, bps 콤보박스 커서 설정.
   CAN 수신 버퍼 크기 64, send 512 수정.
   CAN 수신 갯수 3배수 일경우, 바로 처리...측정데이터 
   메시지 정렬 방식 수정. -일 경우 예외 처리.
   [24.01.13]
   ###### 스레드 안에서 CString 변수로 CAN Log 저장 사용시 간헐적으로 메모리 위반 발생되어.. 저장 기능 삭제
   CAN Log 저장기능 사용하려면 다른 함수로 전달해서 해야 될듯..아님 윈도우 타이머에서 처리하던가..
   멀티미디어 타이머 사용하려다..2개 동시 작동되지 않아 보류..거기다 스레드 방식이라..에러 나올수 있음.
   
   [24.01.18]
   20A 싸이클러 멀티 아이디 지원으로 충방전 중에도 시작되게 수정.
    DEBUG_MSG_01 처리 추가.. CAN 데이터 확인용도, 릴리즈시 주석처리.

  Ver 0.2.7
   [24.01.25]
   접촉검사 기본값 수정. 설정값 표시 추가. 리커버리 시간포함하여 결과시간 계산으로 수정.
   [24.01.30]
   stm32 펌웨어 업데이트 기능 추가.
   [24.02.02]
   dev1Set.SensingReq 시 id에 따라 4 채널 온도요청 추가.  
   [24.02.05]
   전송 100ms 타이머에서 400ms 주기로 8채널 온도 처리 추가.

  Ver 0.2.8
   [24.02.26]
    배터리 BMS pack data 처리 관련 추가.
    배터리 처리 명령 (0x12 , 0x1C) UI 추가.

  Ver 0.2.9
   [24.03.14]
    *** usb-can 간헐적으로 데이터 누락되는 원인은 read timeout 설정인것 같음...100ms 적용. ***
    CAN_SetTimeout(hBus, 100, 100, 20);
   [24.03.15]
    1초 주기로 8채널 BMS 요청 기능 추가.
   [24.03.18]
    read timeout 200ms 로 조정.
    수신 sleep 1ms 적용
   [24.04.11]
    접촉검사 수정.
    펌웨어 업데이트 수정. if 처리가 잘못됨.
    5-20A PacK 아닌경우 팩 설정 UI 삭제.   
   [24.04.24]
    PCAN 펌웨어 업데이트 추가.
    ID가 0x10이상, 병렬 선택시 그룹 명령으로 펌웨어 진행.

  Ver 0.3.0
  [24.04.27]
   1) 그룹 수신 처리 추가.
    - dev1Set.setCur 통해 1k, 2k 시 그룹 수신 처리 추가.
   2) 방전 종료 전류 절대값 처리. (-)처리되면 채널보드에서 종료 안됨.

  [24.05.14]
   1) 패턴 방전 누적 표시 추가.
     => setRcvType 1로 설정 필요. 
     => ini  파일에 [sys] rcv_type=1 추가.
   
   [24.05.20]
   2) CAN 수신 처리 수정... 테스트 확인 필요.
     => MW_USB2CAN경우 CAN_SetTimeout 에서 read Time out을 10이내로 설정하면 간헐적으로 수신 못하는 경우 발생.
     => 설정값에 따라 강제 지연됨. 10으로 설정시 대략 12ms 지연 됨.
     => 현재는 50로 설정하고 cnt 처리로 적용
     # PCAN은 해당 지연이나 누락 문제 없어 타이머 파싱 적용.

    [24.05.30]
   2) pack 싸이클러 데이터 자동 전송에 따른 저장 조건 수정.  (dev1Set.PackDataSave) 
  
  Ver 0.3.1
   [24.06.03] 
   1) 5V200A만 안전조건 전달, 케이블 전압 표시
   2) 테스트 모드에서 1-2K, BMS 체크 박스 UI 숨김.
   
   [24.06.13]
   1) smbus 설정 관련 추가. 2 넣고 set pack 버튼 처리.
   
  Ver 0.3.2
  [24.07.20]
  1) 자동 동작 기능 추가.
    ini 파일 [Dev1] autoStartCnt,  restTime,  autoModeTog
  2) 모듈라인 안전조건 명령 추가.
    케이블 드랍 전압 적용.

  Ver 0.3.3
  [24.08.09]
  그룹 정지 에러(0x0D) ,phase OUT(0x11), OCP(0x12) 추가.
  
  Ver 0.3.4
  [24.09.11]
  160V60A 테스트 모드 추가.
  충/방전 기능 추가중..
  
  [24.09.20]
   정보 표시창 , 전압값 폰트 크기 수정.
   센서 정보 PWM 값 표시 추가.

  [24.10.12]
    160V60A 전류 갱신 추가.
  [24.10.13]
    전압갱신 추가.
  [24.10.14]
    전압 갱신 응답 추가

  Ver 0.3.5
   [24.10.17]
    BMS, I2C 에러 메세지 추가.
    
   [24.10.21]
    BMS 설정 전압 , 대기 전압, 공정 전압 추가.
    테스트 모드 진입 추가.

   Ver 0.3.6
   [24.10.22]
    OnBnClickedBtnT9 측정값 요청에서 UI값 계속 갱신되어 충/방전 변환시 업데이트 가 제대로 안되어 수정.
    충/방전 변환시 BMS 선택 채널값 갱신 추가.
   [24.10.24]
    직렬형 160V60A 32채널 펌웨어 업데이트 ID 수정.

   [24.10.26]
    자동 동작관련 수정..반복동작 해제시..종료가 0이 아니라 연속적으로 정지 명령 전달됨..
    반복동작 해제시. 자동상태요청 해제 ... 공정 시작시 다시 활성화.

   Ver 0.3.7 
   [24.10.29]
    온도값 저장 기능 추가.

   Ver 0.3.8
   [24.10.29]
    IR 값 표시 및 CSV 저장 추가.
    BMS 합산 전압 CSV 저장 추가.

   Ver 0.3.9
   [24.10.30]
    BMS cell delta-Volt CSV 저장 추가.

    Ver 0.3.9.1
   [24.10.30]
     방전 DCIR 표시 추가.

    Ver 0.3.9.2
    [24.10.30]
     자동 동작시 에러 발생되더라도 다시 시작되도록 수정.
    
    Ver 0.3.9.3
     [24.10.31]
     버전 응답 길이 24바이트까지 추가.

    Ver 0.4.0
     [24.11.05]
     콤보박스 ID 정리.
     F2 코드설명 추가.
     Cell Pass 16채널 선택 추가.

*/

#define USE_RCV_THREAD        //주석 처리되면 멀티미디어 타이머로 수신 처리됨...누락되는 경우 있음... 2개 TimeProc, TimeRcvProc 동시 작동 안됨... 보류.
#define THRED_SORT              // 다수 패킷수신시 정렬 처리 추가.

#define SELECT_VOLT_CMD         // 전압 선택 명령.. 주석처리시 숨김.

#ifdef _DEBUG
#define DEBUG_MSG_01           1   // 상황별 디버깅 메시지 확인용도... CAN 데이터 확인..
#else
#endif
//#define CAN_PACKET_TEST         // can 통신 연속 테스트시..

constexpr auto PROG_VER                 = "Ver 0.4.0 [24.11.05]";     
constexpr auto PROG_INFO                = "자동동작 중 에러시 진행됨.";
uint8_t MAX_CH                          = 1;            // 최대 채널 갯수
uint8_t MAX_CUR                         = 200;          // 최대 충/방전 전류
constexpr int ADMIN_PWD                 = 4414001;   // 관리자 패스워드 회사 전화번호
constexpr auto CSV_DIR                  = "csv\\";     // CSV 저장 폴더
constexpr auto LOG_DIR                  = "Log\\";	  // Log 저장 폴더
DWORD CAN_SEND_TIME_THR                 = 100;  // Can 송신 스레드 주기..100ms=> 1초 50ms => 0.5초
constexpr DWORD CAN_RCV_TIME_THR        = 30*1000;  //Sleep 으로 다시변경..  usleep 으로 변경..0.1ms   Can 수신 스레드 주기..7ms~8ms 수신 확인..
constexpr int CAN_RCV_CHK_TIMER         = 5;  // 5ms * 3 = 15ms 동안 수신 없으면 수신 완료 처리.
constexpr uint8_t DEV_STOP_READ_CNT     = 3;    // 정지 버튼 누르더라도 상태 요청하는 횟수.  3-1 =>2번
constexpr uint8_t TIMER_1S_ID           = 0x01;
constexpr uint8_t TIMER_100ms_ID        = 0x02;
constexpr uint8_t TIMER_500ms_ID        = 0x03;

constexpr uint8_t UPDATE_UI_CNT         = 3;    // 300ms

constexpr uint8_t DEV_1                 = 0x01; // 첫번째 장치 (좌측)
constexpr uint8_t DEV_2                 = 0x02; // 두번째 장치 (우측)
constexpr uint8_t DEV_ALL               = 0xFF; // 전체 

//constexpr uint8_t DEV_RESET_MODE        = 0x00;
constexpr uint8_t DEV_CHG_MODE       = 0x00;
//constexpr uint8_t DEV_CCCV_CHG_MODE     = 0x02;
constexpr uint8_t DEV_DISCHG_MODE    = 0x01;
constexpr uint8_t DEV_PATTERN_MODE   = 0x02;
//constexpr uint8_t DEV_WATCH_MODE        = 0x04;   // 마스터 연동 측정데이터만 표시하는 모드.

// CAN 송신명령.
constexpr uint8_t CAN_ACK                   = 0x80;
constexpr uint8_t CAN_START_CHG_CMD			= 0x08;   // 충전 
constexpr uint8_t CAN_START_DISCHG_CMD      = 0x09;   // 방전 
constexpr uint8_t CAN_STATE_CMD				= 0x02;
constexpr uint8_t CAN_CH_STOP_CMD			= 0x01;     // 각 채널 시작 정지 제어..
constexpr uint8_t CAN_READ_DATA_CMD			= 0x0D;
constexpr uint8_t CAN_PROTECT_DATA_CMD      = 0x04;
constexpr uint8_t CAN_SET_CONTACT_CMD		= 0x05;
constexpr uint8_t CAN_CHK_CONTACT_CMD		= 0x06;
constexpr uint8_t CAN_REST_CMD              = 0x07;
//constexpr uint8_t CAN_PATTERN_CMD           = 0x0B;
constexpr uint8_t CAN_PATTERN_UPDATE_CMD    = 0x0C;
constexpr uint8_t CAN_READ_TEMP_CMD         = 0x0E;   // 온도 요청
constexpr uint8_t CAN_RUN_OCV_CMD			= 0x26;
constexpr uint8_t CAN_READ_OCV_CMD			= 0x27;
constexpr uint8_t CAN_CHK_VERSION_CMD		= 0x7E;
constexpr uint8_t CAN_CHK_COM_CMD           = 0x32;
constexpr uint8_t CAN_FW_UP_CMD             = 0x7F;
constexpr uint8_t CAN_VOLT_SEL_CMD          = 0x71;

constexpr uint8_t CAN_START_CV_CMD          = 0x0F;   // 직렬형 충방전기 CV 진입 명령.
constexpr uint8_t CAN_CHANGE_CUR_CMD        = 0x10;   // 직렬형 충방전기 CC 전류 변경 명령.
constexpr uint8_t CAN_START_PATTERN_CMD     = 0x0B;   // 싸이클러 패턴 진입 명령.

constexpr uint8_t CAN_TEST_MODE_CMD         = 0x4F;   // 테스트모드 명령.
constexpr uint8_t CAN_SENSING_CHECK_CMD     = 0x60;   // 센싱 값 확인.
constexpr uint8_t CAN_CELL_BYPASS_CMD       = 0x61;   // 각 채널 셀 연결 설정 .
constexpr uint8_t CAN_TIME_SYNC_CMD         = 0x21;   // 싸이클러 time 동기화 명령.

//배터리 팩 관련 명령
constexpr uint8_t CAN_SET_PACK_USER_DATA_CMD     = 0x12;     // 사용자 데이터 요청 
constexpr uint8_t CAN_SET_PACK_COM_CMD      = 0x1C;     // 통신 설정
constexpr uint8_t CAN_READ_PACK_DATA_CMD    = 0x1D;     // 측정 데이터 요청


constexpr uint8_t CAN_DAQ_RELAY_SET_CMD     = 0x10;  // DAQ 15채널 릴레이 제어 명령

constexpr uint8_t CHK_CONTACT_RE_TIMER        = 4;  // 접촉 검사 결과 요청 4초... 상황에 따라 조정.

//배터리 상태 정의.
constexpr uint8_t BAT_ST_CC                 = 0x00;
constexpr uint8_t BAT_ST_CV                 = 0x01;
constexpr uint8_t BAT_ST_END_VOLT           = 0x02;
constexpr uint8_t BAT_ST_END_CUR            = 0x03;
constexpr uint8_t BAT_ST_END_TIME           = 0x04;
constexpr uint8_t BAT_ST_FAULT              = 0x05;

int SET_MAX_VOLT                  = 4250;    // 4250.0 mV    => 4.25 V
int SET_MIN_VOLT                  = 3000;    // 3000.0 mV    => 3 V
int SET_END_CUR                   = 7800;    // 7800.0 mA    => 7.8 A
int SET_MAX_CUR                   = 100000;  // 100000.0 mA  => 100 A

constexpr uint32_t FULL_DUTY                = 32767;

constexpr int REST_TIMER                    = 60;  // 60초    
constexpr int CH_DELAY_TIMER                = 60*60; //  3600 초  
constexpr auto DEBUG_MASTER                 = true;

constexpr auto CV_DOWN_VOLT                 = 0.3f;     // CV 구간 전압 하락 값.. 

constexpr uint8_t CC_MODE                   = 0x11;
constexpr uint8_t CV_MODE                   = 0x12;
constexpr uint8_t CCCV_MODE                 = 0x13;
constexpr uint8_t CP_MODE                   = 0x14;
constexpr uint8_t CR_MODE                   = 0x15;
constexpr uint8_t CPCV_MODE                 = 0x16;

constexpr auto MSR_UNIT                     = 1000;   // UI와 전송 데이터간 측정 단위 변경.
constexpr auto MSR_POINT_UNIT               = 0.001f;   // 소수점 표시 위한 변경.

constexpr uint8_t _5V_200A_TYPE              = 0;
constexpr uint8_t _5V_6A_TYPE                = 1;
constexpr uint8_t _100V_500A_TYPE            = 2;
constexpr uint8_t _74V_80A_TYPE              = 3;
constexpr uint8_t _TEST_DUTY_TYPE            = 4;           // 모듈형..PWM 테스트 모드..
constexpr uint8_t _5V_20A_PACK_TYPE          = 5;           // 대전 팩 통신 싸이클러 5V-20A
constexpr uint8_t _160V_60A_32CH_TYPE        = 6;           // 직렬형 충방전기 160V60A 32채널


constexpr uint8_t _SET_200A                 = 0;           // 200A, 1000A, 2000A 설정.
constexpr uint8_t _SET_1KA                  = 1;
constexpr uint8_t _SET_2KA                  = 2;

constexpr uint8_t MAX_BMS_CH                = 32;


constexpr uint8_t TEST_DUTY_NORMAL          = 0;
constexpr uint8_t TEST_DUTY_160V60A         = 1;
constexpr uint8_t TEST_DUTY_SPARE1          = 2;

uint8_t ID_CB_MAX                         = 10;       // 콤보박스 ID 갯수


constexpr auto F1_HELP = "ID변경=> Up/Down 키, 충/방전선택=>Space 키, 시작/정지=> Enter 키 , 패턴갱신 F6 키";     // CSV 저장 폴더
constexpr auto F2_INFO = " END Code => 00 None, 01 Time , 02 Volt,  07 Cell, 0A User, 0B Sch, 0C Fault\
\r\nFault Code => 00 None, 01 OVP (Cell), 02 UVP (Cell), 03 OCP 출력, 04 UCP 출력, 05 Over Capacity, 06 OVP 입력, 07 UVP 입력, 08 OTP BD, 09 OTP Cell, 0A CAN Timeout, 0B OVP 보드 출력, 0C Precharge fail, 0D Cable drop, 0E Cell Fault, 11 Phase1 OCP 출력, 12 Phase2 OCP 출력, 13 Phase3 OCP 출력, 14 OVP 바이어스, 15 UVP 바이어스, 30 BMS 통신 Err, 31 BMS Cell Err, 32 BMS 온도 Err, 40 ByPass 통신 Err, F0 Schedule, FE ILLEGAL\
\r\nCell Code=> 0 IDLE, 1 REST, 2 bypass, 3 RUN, 6 Time 정지, 7 Vol. 정지, 8 Cur. 정지, A 사용자 정지, B OVP (Cell), C UVP (Cell), D OTP (Cell), F 비활성화 Ch";

char* batStateMsg[] =
{
    "없음",
    "Time 정지",
    "Vol.정지",
    "Cur.정지",
    "Cap.정지",
    "Watt 정지",
    "WattHour 정지",
    "Dleta - Vp",
    "Temp.정지",
    "Pulse count",
    "사용자 정지",
    "SW_Schedule",
    "End Fault",
    "그룹 정지"
};

char* opCmdMsg[] =
{
    "충전",    
    "방전",
    "패턴"
};

char* opModeMsg[] =
{
    "CC",
    "CV",
    "CCCV",
    "CP",
    "CR",
    "CPCV"
};
char* opStateMsg[] =
{
    "IDLE",
    "READY",
    "DCIR",
    "REST",
    "CC  ",
    "CV  ",
    "CP  ",
    "CR  ",
    "Contact check",
    "Stop"
};
constexpr auto FMSG_IDX = 0x0E;
char* faultMsg[] =
{
    "없음",             //0
    "OVP(Cell)",    
    "UVP(Cell)",        //2
    "OCP(출력)",
    "UCP(출력)",        //4
    "Over Capacity",    
    "OVP(입력)",        //6
    "UVP(입력)",
    "OTP(BD)",          //08
    "OTP(Cell)",        //09
    "Timeout",          //0A    10  
    "OVP 보드 출력",    //0B    11
    "Precharge fail",   //0C    12
    "Cable drop",       //0D    13
    "FW-Ver Err",       //0E    14
    "space 0F",       //0F      14
    "space 10",       //0x10    14
    "Phase OCP",       //0x11    14
    "Phase Err",       //0x12    14

    "Schedule",         //0xF0  15
    "ILLEGAL",          //0xFF  16
    "Con-Cable",        //0x20  17
    "end of msg"
};

char* contactMsg[] =
{   
    "없음",          // 0
    "역 방향",       // 1
    "전류 에러",     // 2 
    "전압 에러",     // 3 
    "델타v 에러",    // 4 
    "배터리 없음",   // 5 
    "배터리 불량",   // 6
    "알수 없음",     // 7
    "스케줄 에러",   // 8 
    "정상"           // 9
};

char* contactMsg2[] =
{
    "없음",               // 0
    "역 방향",            // 1
    "최소전류 에러",      // 2 
    "최대전류 에러",      // 3 
    "전압 에러",          // 4 
    "최소전압 에러",      // 5 
    "최대전압 에러",      // 6 
    "배터리 없음",        // 7 
    "배터리 불량",        // 8
    "알수 없음",          // 9
    "스케줄 에러",        // 10 
    "생략",               // 11 
    "정상"                // 12
};


#endif
