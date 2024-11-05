// pch.h: 미리 컴파일된 헤더 파일입니다.
// 아래 나열된 파일은 한 번만 컴파일되었으며, 향후 빌드에 대한 빌드 성능을 향상합니다.
// 코드 컴파일 및 여러 코드 검색 기능을 포함하여 IntelliSense 성능에도 영향을 미칩니다.
// 그러나 여기에 나열된 파일은 빌드 간 업데이트되는 경우 모두 다시 컴파일됩니다.
// 여기에 자주 업데이트할 파일을 추가하지 마세요. 그러면 성능이 저하됩니다.

#ifndef PCH_H
#define PCH_H

// 여기에 미리 컴파일하려는 헤더 추가
#include "framework.h"
#include <Windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

#ifdef _DEBUG
#ifdef UNICODE
#pragma comment(linker, "/entry:wWinMainCRTStartup /subsystem:console")
#else
#pragma comment(linker, "/entry:WinMainCRTStartup /subsystem:console")
#endif
#define d_msgl(...)        do  {_tprintf_s(_T("%s[%s:%d] "), GetTimeStamp(4), __FUNCTION__, __LINE__);  _tprintf(_T(__VA_ARGS__)); _tprintf(_T("\n"));} while(0)
#define d_msg(...)        do  {_tprintf(_T(__VA_ARGS__));} while(0)
#else
#define d_msgl(...)		do{}while(0)
#define d_msg(...)      do{}while(0)		
#endif

//#define _SYS_LOG

// xp stlye  
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
//#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define WM_USER_CAN_RCV_ALL_MSG				WM_USER+1000
#define WM_USER_CAN_SEND_MSG				WM_USER+1001

typedef enum
{
	NONE_CAN,
	MW_USB2CAN,
	USB_PCAN,
	DEV_CAN_MAX
}  CAN_Dev_Type_t;

CString GetExePath(void);				// 현재 실행 폴더 리턴
bool GetFindFile(CString file);			// 폴더내 파일 존재 확인.
void usleep(DWORD waitTime);
#endif //PCH_H
