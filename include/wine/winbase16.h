#ifndef _INCLUDE_WINE_WINBASE16_H
#define _INCLUDE_WINE_WINBASE16_H

#include "windef.h"
#include "pshpack1.h"
typedef struct _SEGINFO {
    UINT16    offSegment;
    UINT16    cbSegment;
    UINT16    flags;
    UINT16    cbAlloc;
    HGLOBAL16 h;
    UINT16    alignShift;
    UINT16    reserved[2];
} SEGINFO;


/* GetWinFlags */

#define WF_PMODE 	0x0001
#define WF_CPU286 	0x0002
#define	WF_CPU386	0x0004
#define	WF_CPU486 	0x0008
#define	WF_STANDARD	0x0010
#define	WF_WIN286 	0x0010
#define	WF_ENHANCED	0x0020
#define	WF_WIN386	0x0020
#define	WF_CPU086	0x0040
#define	WF_CPU186	0x0080
#define	WF_LARGEFRAME	0x0100
#define	WF_SMALLFRAME	0x0200
#define	WF_80x87	0x0400
#define	WF_PAGING	0x0800
#define	WF_HASCPUID     0x2000
#define	WF_WIN32WOW     0x4000	/* undoc */
#define	WF_WLO          0x8000

#include "poppack.h"

HMODULE16   WINAPI GetModuleHandle16(LPCSTR);
HLOCAL16    WINAPI LocalAlloc16(UINT16,WORD);
LPVOID      WINAPI LockResource16(HGLOBAL16);

#ifdef __WINE__
WORD        WINAPI AllocCStoDSAlias16(WORD);
WORD        WINAPI AllocDStoCSAlias16(WORD);
HGLOBAL16   WINAPI AllocResource16(HINSTANCE16,HRSRC16,DWORD);
WORD        WINAPI AllocSelector16(WORD);
WORD        WINAPI AllocSelectorArray16(WORD);
VOID        WINAPI DirectedYield16(HTASK16);
HGLOBAL16   WINAPI DirectResAlloc16(HINSTANCE16,WORD,UINT16);
HANDLE16    WINAPI FarGetOwner16(HGLOBAL16);
VOID        WINAPI FarSetOwner16(HGLOBAL16,HANDLE16);
FARPROC16   WINAPI FileCDR16(FARPROC16);
WORD        WINAPI FreeSelector16(WORD);
HANDLE16    WINAPI GetAtomHandle16(ATOM);
HANDLE16    WINAPI GetCodeHandle16(FARPROC16);
VOID        WINAPI GetCodeInfo16(FARPROC16,SEGINFO*);
DWORD       WINAPI GetCurrentPDB16(void);
HTASK16     WINAPI GetCurrentTask(void);
SEGPTR      WINAPI GetDOSEnvironment16(void);
HMODULE16   WINAPI GetExePtr(HANDLE16);
WORD        WINAPI GetExeVersion16(void);
WORD        WINAPI GetExpWinVer16(HMODULE16);
DWORD       WINAPI GetHeapSpaces16(HMODULE16);
INT16       WINAPI GetInstanceData16(HINSTANCE16,WORD,INT16);
BOOL16      WINAPI GetModuleName16(HINSTANCE16,LPSTR,INT16);
INT16       WINAPI GetModuleUsage16(HINSTANCE16);
UINT16      WINAPI GetNumTasks16(void);
DWORD       WINAPI GetSelectorLimit16(WORD);
FARPROC16   WINAPI GetSetKernelDOSProc16(FARPROC16 DosProc);
HINSTANCE16 WINAPI GetTaskDS16(void);
HQUEUE16    WINAPI GetTaskQueue16(HTASK16);
DWORD       WINAPI GetWinFlags16(void);
DWORD       WINAPI GlobalDOSAlloc16(DWORD);
WORD        WINAPI GlobalDOSFree16(WORD);
void        WINAPI GlobalFreeAll16(HGLOBAL16);
DWORD       WINAPI GlobalHandleNoRIP16(WORD);
HGLOBAL16   WINAPI GlobalLRUNewest16(HGLOBAL16);
HGLOBAL16   WINAPI GlobalLRUOldest16(HGLOBAL16);
VOID        WINAPI GlobalNotify16(FARPROC16);
WORD        WINAPI GlobalPageLock16(HGLOBAL16);
WORD        WINAPI GlobalPageUnlock16(HGLOBAL16);
BOOL16      WINAPI IsSharedSelector16(HANDLE16);
BOOL16      WINAPI IsTask16(HTASK16);
HTASK16     WINAPI IsTaskLocked16(void);
VOID        WINAPI LogError16(UINT16, LPVOID);
VOID        WINAPI LogParamError16(UINT16,FARPROC16,LPVOID);
WORD        WINAPI LocalCountFree16(void);
WORD        WINAPI LocalHandleDelta16(WORD);
WORD        WINAPI LocalHeapSize16(void);
BOOL16      WINAPI LocalInit16(HANDLE16,WORD,WORD);
HMODULE   WINAPI LoadLibraryEx32W16(LPCSTR,HANDLE16,DWORD);
FARPROC16   WINAPI LocalNotify16(FARPROC16);
HTASK16     WINAPI LockCurrentTask16(BOOL16);
VOID        WINAPI OldYield16(void);
VOID        WINAPI PostEvent16(HTASK16);
WORD        WINAPI PrestoChangoSelector16(WORD,WORD);
WORD        WINAPI SelectorAccessRights16(WORD,WORD,WORD);
VOID        WINAPI SetPriority16(HTASK16,INT16);
FARPROC16   WINAPI SetResourceHandler16(HINSTANCE16,SEGPTR,FARPROC16);
WORD        WINAPI SetSelectorLimit16(WORD,DWORD);
HQUEUE16    WINAPI SetTaskQueue16(HTASK16,HQUEUE16);
VOID        WINAPI SwitchStackTo16(WORD,WORD,WORD);
BOOL16      WINAPI WaitEvent16(HTASK16);
VOID        WINAPI WriteOutProfiles16(VOID);
VOID        WINAPI hmemcpy16(LPVOID,LPCVOID,LONG);
#endif  /* __WINE__ */

INT16       WINAPI AccessResource16(HINSTANCE16,HRSRC16);
ATOM        WINAPI AddAtom16(SEGPTR);
BOOL16      WINAPI CreateDirectory16(LPCSTR,LPVOID);
BOOL16      WINAPI DefineHandleTable16(WORD);
ATOM        WINAPI DeleteAtom16(ATOM);
BOOL16      WINAPI DeleteFile16(LPCSTR);
void        WINAPI ExitKernel16(void);
void        WINAPI FatalAppExit16(UINT16,LPCSTR);
ATOM        WINAPI FindAtom16(SEGPTR);
BOOL16      WINAPI FindClose16(HANDLE16);
HRSRC16     WINAPI FindResource16(HINSTANCE16,SEGPTR,SEGPTR);
BOOL16      WINAPI FreeModule16(HMODULE16);
void        WINAPI FreeProcInstance16(FARPROC16);
BOOL16      WINAPI FreeResource16(HGLOBAL16);
UINT16      WINAPI GetAtomName16(ATOM,LPSTR,INT16);
UINT16      WINAPI GetCurrentDirectory16(UINT16,LPSTR);
BOOL16      WINAPI GetDiskFreeSpace16(LPCSTR,LPDWORD,LPDWORD,LPDWORD,LPDWORD);
UINT16      WINAPI GetDriveType16(UINT16); /* yes, the arguments differ */
DWORD       WINAPI GetFileAttributes16(LPCSTR);
DWORD       WINAPI GetFreeSpace16(UINT16);
INT16       WINAPI GetModuleFileName16(HINSTANCE16,LPSTR,INT16);
UINT16      WINAPI GetPrivateProfileInt16(LPCSTR,LPCSTR,INT16,LPCSTR);
WORD        WINAPI GetPrivateProfileSectionNames16(LPSTR,UINT16,LPCSTR);
INT16       WINAPI GetPrivateProfileString16(LPCSTR,LPCSTR,LPCSTR,LPSTR,UINT16,LPCSTR);
FARPROC16   WINAPI GetProcAddress16(HMODULE16,SEGPTR);
UINT16      WINAPI GetProfileInt16(LPCSTR,LPCSTR,INT16);
INT16       WINAPI GetProfileString16(LPCSTR,LPCSTR,LPCSTR,LPSTR,UINT16);
UINT16      WINAPI GetSystemDirectory16(LPSTR,UINT16);
UINT16      WINAPI GetTempFileName16(BYTE,LPCSTR,UINT16,LPSTR);
LONG        WINAPI GetVersion16(void);
UINT16      WINAPI GetWindowsDirectory16(LPSTR,UINT16);
DWORD       WINAPI GlobalCompact16(DWORD);
UINT16      WINAPI GlobalFlags16(HGLOBAL16);
DWORD       WINAPI GlobalHandle16(WORD);
HGLOBAL16   WINAPI GlobalReAlloc16(HGLOBAL16,DWORD,UINT16);
WORD        WINAPI InitAtomTable16(WORD);
BOOL16      WINAPI IsBadCodePtr16(SEGPTR);
BOOL16      WINAPI IsBadHugeReadPtr16(SEGPTR,DWORD);
BOOL16      WINAPI IsBadHugeWritePtr16(SEGPTR,DWORD);
BOOL16      WINAPI IsBadReadPtr16(SEGPTR,UINT16);
BOOL16      WINAPI IsBadStringPtr16(SEGPTR,UINT16);
BOOL16      WINAPI IsBadWritePtr16(SEGPTR,UINT16);
BOOL16      WINAPI IsDBCSLeadByte16(BYTE);
HGLOBAL16   WINAPI LoadResource16(HINSTANCE16,HRSRC16);
UINT16      WINAPI LocalCompact16(UINT16);
UINT16      WINAPI LocalFlags16(HLOCAL16);
HLOCAL16    WINAPI LocalFree16(HLOCAL16);
HLOCAL16    WINAPI LocalHandle16(WORD);
SEGPTR      WINAPI LocalLock16(HLOCAL16);
HLOCAL16    WINAPI LocalReAlloc16(HLOCAL16,WORD,UINT16);
UINT16      WINAPI LocalShrink16(HGLOBAL16,UINT16);
UINT16      WINAPI LocalSize16(HLOCAL16);
BOOL16      WINAPI LocalUnlock16(HLOCAL16);
HGLOBAL16   WINAPI LockSegment16(HGLOBAL16);
FARPROC16   WINAPI MakeProcInstance16(FARPROC16,HANDLE16);
VOID        WINAPI OutputDebugString16(LPCSTR);
DWORD       WINAPI RegCreateKey16(HKEY,LPCSTR,LPHKEY);
DWORD       WINAPI RegDeleteKey16(HKEY,LPCSTR);
DWORD       WINAPI RegDeleteValue16(HKEY,LPSTR);
DWORD       WINAPI RegEnumKey16(HKEY,DWORD,LPSTR,DWORD);
DWORD       WINAPI RegEnumValue16(HKEY,DWORD,LPSTR,LPDWORD,LPDWORD,LPDWORD,LPBYTE,LPDWORD);
DWORD       WINAPI RegOpenKey16(HKEY,LPCSTR,LPHKEY);
DWORD       WINAPI RegQueryValue16(HKEY,LPSTR,LPSTR,LPDWORD);
DWORD       WINAPI RegQueryValueEx16(HKEY,LPSTR,LPDWORD,LPDWORD,LPBYTE,LPDWORD);
DWORD       WINAPI RegSetValue16(HKEY,LPCSTR,DWORD,LPCSTR,DWORD);
DWORD       WINAPI RegSetValueEx16(HKEY,LPSTR,DWORD,DWORD,LPBYTE,DWORD);
BOOL16      WINAPI RemoveDirectory16(LPCSTR);
BOOL16      WINAPI SetCurrentDirectory16(LPCSTR);
UINT16      WINAPI SetErrorMode16(UINT16);
BOOL16      WINAPI SetFileAttributes16(LPCSTR,DWORD);
UINT16      WINAPI SetHandleCount16(UINT16);
LONG        WINAPI SetSwapAreaSize16(WORD);
DWORD       WINAPI SizeofResource16(HMODULE16,HRSRC16);
void        WINAPI UnlockSegment16(HGLOBAL16);
BOOL16      WINAPI WritePrivateProfileString16(LPCSTR,LPCSTR,LPCSTR,LPCSTR);
BOOL16      WINAPI WriteProfileString16(LPCSTR,LPCSTR,LPCSTR);
VOID        WINAPI Yield16(void);
SEGPTR      WINAPI lstrcat16(SEGPTR,LPCSTR);
SEGPTR      WINAPI lstrcatn16(SEGPTR,LPCSTR,INT16);
SEGPTR      WINAPI lstrcpy16(SEGPTR,LPCSTR);
SEGPTR      WINAPI lstrcpyn16(SEGPTR,LPCSTR,INT16);
INT16       WINAPI lstrlen16(LPCSTR);
HINSTANCE16 WINAPI WinExec16(LPCSTR,UINT16);
LONG        WINAPI _hread16(HFILE16,LPVOID,LONG);
LONG        WINAPI _hwrite16(HFILE16,LPCSTR,LONG);
HFILE16     WINAPI _lcreat16(LPCSTR,INT16);
HFILE16     WINAPI _lclose16(HFILE16);
LONG        WINAPI _llseek16(HFILE16,LONG,INT16);
HFILE16     WINAPI _lopen16(LPCSTR,INT16);
UINT16      WINAPI _lread16(HFILE16,LPVOID,UINT16);
UINT16      WINAPI _lwrite16(HFILE16,LPCSTR,UINT16);
BOOL16      WINAPI WritePrivateProfileSection16(LPCSTR,LPCSTR,LPCSTR);
BOOL16      WINAPI WritePrivateProfileStruct16(LPCSTR,LPCSTR,LPVOID,UINT16,LPCSTR);
DWORD       WINAPI GetSelectorBase(WORD);
WORD        WINAPI SetSelectorBase(WORD,DWORD);

/* Extra functions that don't exist in the Windows API */

HPEN16      WINAPI GetSysColorPen16(INT16);
UINT      WINAPI WIN16_GetTempDrive(BYTE);
SEGPTR      WINAPI WIN16_LockResource16(HGLOBAL16);
LONG        WINAPI WIN16_hread(HFILE16,SEGPTR,LONG);
UINT16      WINAPI WIN16_lread(HFILE16,SEGPTR,UINT16);

#endif /* _INCLUDE_WINE_WINBASE16_H */
