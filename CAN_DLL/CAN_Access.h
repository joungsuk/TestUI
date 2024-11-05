
#ifdef CAN_ACCESS_EXPORTS
#define CAN_API __declspec(dllexport)
#else
#define CAN_API __declspec(dllimport)
#endif

#define CAN_HANDLE		long

#ifdef __cplusplus
extern "C" {
#endif

CAN_API CAN_HANDLE __stdcall CAN_OpenVcp(const char *comPortName, long comBaudrate);
CAN_API CAN_HANDLE __stdcall CAN_OpenFifo(const char *serialNumber);
CAN_API int  __stdcall CAN_Close(CAN_HANDLE handle);

CAN_API int  __stdcall CAN_IsOpened (CAN_HANDLE handle);
CAN_API int  __stdcall CAN_SetTimeout (CAN_HANDLE handle, unsigned long read_timeout, unsigned long write_timeout, unsigned long latency_timer);
CAN_API int  __stdcall CAN_CountRxQueue (CAN_HANDLE handle);
CAN_API void __stdcall CAN_Purge (CAN_HANDLE handle);

CAN_API int  __stdcall CAN_Recv (CAN_HANDLE handle, long *ID, int *length, char data[8], int *Ext, int *RTR);
CAN_API int  __stdcall CAN_Send (CAN_HANDLE handle, long ID, int length, char data[8], int Ext, int RTR);

CAN_API int  __stdcall CAN_GetConfig (CAN_HANDLE handle, long *bitrate, unsigned long *filterID, unsigned long *filterMask);
CAN_API int  __stdcall CAN_SetConfig (CAN_HANDLE handle, long bitrate, unsigned long filterID, unsigned long filterMask);

CAN_API int  __stdcall CAN_SetTransferMode (CAN_HANDLE handle, int mode);

CAN_API int   __stdcall CAN_Fifo_ScanSerialNumber ();
CAN_API char* __stdcall CAN_Fifo_GetSerialNumber (int index);
CAN_API int   __stdcall CAN_Fifo_SetRxEventNotification (CAN_HANDLE handle, void *hEvent);

#ifdef __cplusplus
}
#endif