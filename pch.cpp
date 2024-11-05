// pch.cpp: 미리 컴파일된 헤더에 해당하는 소스 파일

#include "pch.h"
#include <profileapi.h>
// 미리 컴파일된 헤더를 사용하는 경우 컴파일이 성공하려면 이 소스 파일이 필요합니다.

CString GetExePath(void)
{
	CString strResult;
	CString strPath;

	if (GetModuleFileName(nullptr, strPath.GetBuffer(_MAX_PATH + 1), MAX_PATH) != FALSE)
	{
		strPath.ReleaseBuffer();
		strResult = strPath.Left(strPath.ReverseFind('\\') + 1);
	}
	else
	{
		d_msg("GetExePath err\n");
	}
	return strResult;
}

bool GetFindFile(CString file)
{
	CString strIni;
	strIni.Format(_T("%s\\%s"), GetExePath(),file);
	CFileStatus fs;
	if (CFile::GetStatus(strIni, fs)) return true;
	else return false;	
}

void usleep(DWORD waitTime)
{
	LARGE_INTEGER perfCnt, start, now;

	QueryPerformanceFrequency(&perfCnt);
	QueryPerformanceCounter(&start);

	do
	{
		QueryPerformanceCounter((LARGE_INTEGER*)&now);
	} while ((now.QuadPart - start.QuadPart) / float(perfCnt.QuadPart) * 1000 * 1000 < waitTime);
}
