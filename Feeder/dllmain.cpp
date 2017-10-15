#include "stdafx.h"

#include <metahost.h>
#include <string>
#include <iostream>

#include <InjectionInitialization.h>
#include <Util.h>

#include <mhook-lib/mhook.h>

#include "CLRHost.h"

namespace Feeder
{
	typedef BOOL(WINAPI *_CreateProcessA)(
		_In_opt_    LPCTSTR               lpApplicationName,
		_Inout_opt_ LPTSTR                lpCommandLine,
		_In_opt_    LPSECURITY_ATTRIBUTES lpProcessAttributes,
		_In_opt_    LPSECURITY_ATTRIBUTES lpThreadAttributes,
		_In_        BOOL                  bInheritHandles,
		_In_        DWORD                 dwCreationFlags,
		_In_opt_    LPVOID                lpEnvironment,
		_In_opt_    LPCTSTR               lpCurrentDirectory,
		_In_        LPSTARTUPINFO         lpStartupInfo,
		_Out_       LPPROCESS_INFORMATION lpProcessInformation);

	typedef BOOL(WINAPI *_CreateProcessW)(
		_In_opt_	LPCWSTR lpApplicationName,
		_Inout_opt_ LPWSTR lpCommandLine,
		_In_opt_	LPSECURITY_ATTRIBUTES lpProcessAttributes,
		_In_opt_	LPSECURITY_ATTRIBUTES lpThreadAttributes,
		_In_		BOOL bInheritHandles,
		_In_		DWORD dwCreationFlags,
		_In_opt_	LPVOID lpEnvironment,
		_In_opt_	LPCWSTR lpCurrentDirectory,
		_In_		LPSTARTUPINFOW lpStartupInfo,
		_Out_		LPPROCESS_INFORMATION lpProcessInformation);

	static _CreateProcessA TRUE_CREATE_PROCESSA;
	static _CreateProcessW TRUE_CREATE_PROCESSW;

	static BOOL WINAPI HookCreateProcessA(
		_In_opt_    LPCTSTR               lpApplicationName,
		_Inout_opt_ LPTSTR                lpCommandLine,
		_In_opt_    LPSECURITY_ATTRIBUTES lpProcessAttributes,
		_In_opt_    LPSECURITY_ATTRIBUTES lpThreadAttributes,
		_In_        BOOL                  bInheritHandles,
		_In_        DWORD                 dwCreationFlags,
		_In_opt_    LPVOID                lpEnvironment,
		_In_opt_    LPCTSTR               lpCurrentDirectory,
		_In_        LPSTARTUPINFO         lpStartupInfo,
		_Out_       LPPROCESS_INFORMATION lpProcessInformation
		)
	{
		if (lpCommandLine == NULL)
		{
			MessageBoxA(NULL, Shared::stringFormat("HookCreateProcessA: %s", lpApplicationName).c_str(), "", MB_OK);
		}
		else
		{
			MessageBoxA(NULL, Shared::stringFormat("HookCreateProcessA: %s|%s", lpApplicationName, lpCommandLine).c_str(), "", MB_OK);
		}

		return TRUE_CREATE_PROCESSA(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
	}

	static BOOL WINAPI HookCreateProcessW(
		_In_opt_	LPCWSTR lpApplicationName,
		_Inout_opt_ LPWSTR lpCommandLine,
		_In_opt_	LPSECURITY_ATTRIBUTES lpProcessAttributes,
		_In_opt_	LPSECURITY_ATTRIBUTES lpThreadAttributes,
		_In_		BOOL bInheritHandles,
		_In_		DWORD dwCreationFlags,
		_In_opt_	LPVOID lpEnvironment,
		_In_opt_	LPCWSTR lpCurrentDirectory,
		_In_		LPSTARTUPINFOW lpStartupInfo,
		_Out_		LPPROCESS_INFORMATION lpProcessInformation
		)
	{
		if (lpCommandLine == NULL)
		{
			MessageBoxA(NULL, Shared::stringFormat("HookCreateProcessA: %ls", lpApplicationName).c_str(), "", MB_OK);
		}
		else
		{
			MessageBoxA(NULL, Shared::stringFormat("HookCreateProcessA: %ls|%ls", lpApplicationName, lpCommandLine).c_str(), "", MB_OK);
		}

		return TRUE_CREATE_PROCESSW(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
	}

	extern "C"
	{
		
		__declspec(dllexport) HRESULT hook(LPVOID *argument)
		{
			HMODULE kernel32Module = GetModuleHandleW(L"Kernel32");
			if (kernel32Module == NULL)
			{
				std::wcout << L"Failed GetModuleHandleW\nSystem error code: %u" << GetLastError() << std::endl;

				std::wcin.get();

				return FALSE;
			}

			Feeder::TRUE_CREATE_PROCESSA = reinterpret_cast<Feeder::_CreateProcessA>(GetProcAddress(kernel32Module, "CreateProcessA"));
			if (Feeder::TRUE_CREATE_PROCESSA == NULL)
			{
				return FALSE;
			}

			Feeder::TRUE_CREATE_PROCESSW = reinterpret_cast<Feeder::_CreateProcessW>(GetProcAddress(kernel32Module, "CreateProcessW"));
			if (Feeder::TRUE_CREATE_PROCESSW == NULL)
			{
				return FALSE;
			}

			if (Mhook_SetHook((PVOID*)&Feeder::TRUE_CREATE_PROCESSA, HookCreateProcessA) == FALSE)
			{
				return FALSE;
			}

			if (Mhook_SetHook((PVOID*)&Feeder::TRUE_CREATE_PROCESSW, HookCreateProcessW) == FALSE)
			{
				Mhook_Unhook((PVOID*)&Feeder::TRUE_CREATE_PROCESSA);

				return FALSE;
			}

			return TRUE;
		}

		__declspec(dllexport) HRESULT unhook(LPVOID *argument)
		{
			return (Mhook_Unhook((PVOID*)&Feeder::TRUE_CREATE_PROCESSA) && Mhook_Unhook((PVOID*)&Feeder::TRUE_CREATE_PROCESSW));
		}
	}
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		case DLL_PROCESS_DETACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
			break;
	}

	return TRUE;
}

