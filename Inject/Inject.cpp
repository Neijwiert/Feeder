#include "stdafx.h"

#include <cstdlib>
#include <string>
#include <Windows.h>
#include <iostream>

#include <Util.h>

#include "NamedMutex.h"
#include "RemoteLibrary.h"

namespace Inject
{
	static const std::wstring MUTEX_GUID = L"INJECT-{37CD8E62-A704-45DB-BFDE-3D292DEC2AB3}";

	static const std::wstring APPLICATION_BATTLEYE_PATH = L"C:\\Program Files (x86)\\Steam\\steamapps\\common\\ARK\\ShooterGame\\Binaries\\Win64\\ShooterGame_BE.exe";

	static const std::wstring FEEDER_ASSEMBLY_NAME(L"Feeder.dll");

	static const std::string FEEDER_INJECTION_HOOK_METHOD_NAME = "hook";
	static const std::string FEEDER_INJECTION_UNHOOK_METHOD_NAME = "unhook";
}

int main()
{
	try
	{
		Inject::NamedMutex instanceMutex(Inject::MUTEX_GUID);
		if (instanceMutex.isAlreadyTaken())
		{
			return EXIT_FAILURE;
		}

		STARTUPINFO startupInfo;
		ZeroMemory(&startupInfo, sizeof(STARTUPINFO));

		startupInfo.cb = sizeof(STARTUPINFO);

		PROCESS_INFORMATION processInformation;
		ZeroMemory(&processInformation, sizeof(PROCESS_INFORMATION));

		if (CreateProcessW(Inject::APPLICATION_BATTLEYE_PATH.c_str(), NULL, NULL, NULL, TRUE, CREATE_SUSPENDED, NULL, NULL, &startupInfo, &processInformation) == FALSE)
		{
			std::wcout << L"Failed CreateProcessW\nSystem error code: %u" << GetLastError() << std::endl;

			std::wcin.get();

			return EXIT_FAILURE;
		}

		std::wstring executionPath;
		Shared::getExecutionPath(executionPath);

		const std::wstring feederPath = executionPath + L'\\' + Inject::FEEDER_ASSEMBLY_NAME;
		Inject::RemoteLibrary library(processInformation.dwProcessId, feederPath);
		library.inject();

		if (library.runRemoteFunction(Inject::FEEDER_INJECTION_HOOK_METHOD_NAME) == FALSE)
		{
			std::wcout << L"Failed to hook" << std::endl;

			CloseHandle(processInformation.hProcess);
			CloseHandle(processInformation.hThread);

			std::wcin.get();

			return EXIT_FAILURE;
		}

		if (ResumeThread(processInformation.hThread) == -1)
		{
			std::wcout << L"Failed ResumeThread\nSystem error code: %u" << GetLastError() << std::endl;

			CloseHandle(processInformation.hProcess);
			CloseHandle(processInformation.hThread);

			std::wcin.get();

			return EXIT_FAILURE;
		}

		std::cout << "Press any button to continue...\n";
		std::cin.get();

		if (library.runRemoteFunction(Inject::FEEDER_INJECTION_UNHOOK_METHOD_NAME) == FALSE)
		{
			std::wcout << L"Failed to unhook" << std::endl;

			CloseHandle(processInformation.hProcess);
			CloseHandle(processInformation.hThread);

			std::wcin.get();

			return EXIT_FAILURE;
		}

		library.eject();

		CloseHandle(processInformation.hProcess);
		CloseHandle(processInformation.hThread);

		return EXIT_SUCCESS;
	}
	catch (std::runtime_error er)
	{
		std::cout << er.what();

		std::cin.get();

		return EXIT_FAILURE;
	}
}

