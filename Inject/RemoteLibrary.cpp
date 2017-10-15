#include "stdafx.h"
#include "RemoteLibrary.h"

#include <Util.h>

namespace Inject
{
	RemoteLibrary::RemoteLibrary(DWORD processId, const std::wstring &filePath) :
		processId(processId),
		filePath(filePath),
		fileName(),
		injected(false),
		remoteProcessHandle(NULL),
		remoteModuleEntry(),
		moduleHandle(NULL)
	{
		ZeroMemory(&this->remoteModuleEntry, sizeof(MODULEENTRY32W));

		std::size_t found = this->filePath.find_last_of(L"/\\");
		this->fileName = this->filePath.substr(found + 1);
	}

	RemoteLibrary::RemoteLibrary(DWORD processId, const std::wstring &&filePath) :
		processId(processId),
		filePath(std::forward<const std::wstring>(filePath)),
		fileName(),
		injected(false),
		remoteProcessHandle(NULL),
		remoteModuleEntry(),
		moduleHandle(NULL)
	{
		ZeroMemory(&this->remoteModuleEntry, sizeof(MODULEENTRY32W));

		std::size_t found = this->filePath.find_last_of(L"/\\");
		this->fileName = this->filePath.substr(found + 1);
	}

	RemoteLibrary::RemoteLibrary(RemoteLibrary &&other) :
		processId(other.processId),
		filePath(other.filePath),
		fileName(other.fileName),
		injected(other.injected),
		remoteProcessHandle(other.remoteProcessHandle),
		remoteModuleEntry(),
		moduleHandle(other.moduleHandle)
	{
		memcpy(&this->remoteModuleEntry, &other.remoteModuleEntry, sizeof(MODULEENTRY32W));

		other.injected = false;
	}

	RemoteLibrary::~RemoteLibrary()
	{
		try
		{
			eject();
		}
		catch (std::runtime_error)
		{

		}
	}

	void RemoteLibrary::inject()
	{
		if (this->injected)
		{
			throw std::runtime_error("Already injected\n");
		}

		this->moduleHandle = LoadLibraryW(this->filePath.c_str());
		if (this->moduleHandle == NULL)
		{
			throw std::runtime_error(Shared::stringFormat("Failed to load the target library\nSystem error code: %u\n", GetLastError()));
		}

		this->remoteProcessHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, this->processId);
		if (this->remoteProcessHandle == NULL)
		{
			DWORD lastError = GetLastError();

			FreeLibrary(this->moduleHandle);

			throw std::runtime_error(Shared::stringFormat("Failed to open remote process handle\nSystem error code: %u\n", lastError));
		}

		BOOL remoteProcessWow64Process;
		if (IsWow64Process(this->remoteProcessHandle, &remoteProcessWow64Process) == FALSE)
		{
			DWORD lastError = GetLastError();

			CloseHandle(this->remoteProcessHandle);
			FreeLibrary(this->moduleHandle);

			throw std::runtime_error(Shared::stringFormat("Failed to check if the remote process is a WoW64 process\nSystem error code: %u\n", lastError));
		}

		if (isCurrentProcessWow64Process() != remoteProcessWow64Process)
		{
			CloseHandle(this->remoteProcessHandle);
			FreeLibrary(this->moduleHandle);

			throw std::runtime_error("This process' architecture does not match the remote process' architecture\n");
		}

		std::size_t filePathByteSize = this->filePath.length() * sizeof(wchar_t) + sizeof(wchar_t);
		LPVOID remoteMemoryAddress = VirtualAllocEx(this->remoteProcessHandle, NULL, filePathByteSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
		if (remoteMemoryAddress == NULL)
		{
			DWORD lastError = GetLastError();

			CloseHandle(this->remoteProcessHandle);
			FreeLibrary(this->moduleHandle);

			throw std::runtime_error(Shared::stringFormat("Failed to allocate remote memory\nSystem error code: %u\n", lastError));
		}

		if (WriteProcessMemory(this->remoteProcessHandle, remoteMemoryAddress, this->filePath.c_str(), filePathByteSize, NULL) == FALSE)
		{
			DWORD lastError = GetLastError();

			VirtualFreeEx(this->remoteProcessHandle, remoteMemoryAddress, 0, MEM_RELEASE);
			CloseHandle(this->remoteProcessHandle);
			FreeLibrary(this->moduleHandle);

			throw std::runtime_error(Shared::stringFormat("Failed to write to remote memory\nSystem error code: %u\n", lastError));
		}

		HANDLE remoteThreadHandle = CreateRemoteThread(this->remoteProcessHandle, NULL, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(getLoadLibraryAddress()), remoteMemoryAddress, NULL, 0);
		if (remoteThreadHandle == NULL)
		{
			DWORD lastError = GetLastError();

			VirtualFreeEx(this->remoteProcessHandle, remoteMemoryAddress, 0, MEM_RELEASE);
			CloseHandle(this->remoteProcessHandle);
			FreeLibrary(this->moduleHandle);

			throw std::runtime_error(Shared::stringFormat("Failed to create remote thread\nSystem error code: %u\n", lastError));
		}

		DWORD waitResult = WaitForSingleObject(remoteThreadHandle, INFINITE);

		if (VirtualFreeEx(this->remoteProcessHandle, remoteMemoryAddress, 0, MEM_RELEASE) == FALSE)
		{
			DWORD lastError = GetLastError();

			CloseHandle(remoteThreadHandle);
			CloseHandle(this->remoteProcessHandle);
			FreeLibrary(this->moduleHandle);

			throw std::runtime_error(Shared::stringFormat("Failed to free remote memory\nSystem error code: %u\n", lastError));
		}

		if (waitResult != WAIT_OBJECT_0)
		{
			CloseHandle(remoteThreadHandle);
			CloseHandle(this->remoteProcessHandle);
			FreeLibrary(this->moduleHandle);

			throw std::runtime_error(Shared::stringFormat("Remote LoadLibrary wait error\nWaitForSingleObject return code: %u\n", waitResult));
		}

		DWORD remoteThreadExitCode;
		if (GetExitCodeThread(remoteThreadHandle, &remoteThreadExitCode) == FALSE)
		{
			DWORD lastError = GetLastError();

			CloseHandle(remoteThreadHandle);
			CloseHandle(this->remoteProcessHandle);
			FreeLibrary(this->moduleHandle);

			throw std::runtime_error(Shared::stringFormat("Failed to get LoadLibrary's return code\nSystem error code: %u\n", lastError));
		}

		if (remoteThreadExitCode == NULL)
		{
			CloseHandle(remoteThreadHandle);
			CloseHandle(this->remoteProcessHandle);
			FreeLibrary(this->moduleHandle);

			throw std::runtime_error("Failed to load remote library\n");
		}

		if (CloseHandle(remoteThreadHandle) == FALSE)
		{
			DWORD lastError = GetLastError();

			CloseHandle(this->remoteProcessHandle);
			FreeLibrary(this->moduleHandle);

			throw std::runtime_error(Shared::stringFormat("Failed to close remote thread handle\nSystem error code: %u\n", lastError));
		}

		MODULEENTRY32W currentModuleEntry;
		currentModuleEntry.dwSize = sizeof(MODULEENTRY32W);

		HANDLE snapshotHandle = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, this->processId);
		if (snapshotHandle == INVALID_HANDLE_VALUE)
		{
			DWORD lastError = GetLastError();

			CloseHandle(this->remoteProcessHandle);
			FreeLibrary(this->moduleHandle);

			throw std::runtime_error(Shared::stringFormat("Failed to create tool help snapshot\nSystem error code: %u\n", lastError));
		}

		if (Module32FirstW(snapshotHandle, &currentModuleEntry) == FALSE)
		{
			CloseHandle(snapshotHandle);
			CloseHandle(this->remoteProcessHandle);
			FreeLibrary(this->moduleHandle);

			throw std::runtime_error("Failed to load remote library\n");
		}

		do
		{
			if (this->fileName.compare(currentModuleEntry.szModule) == 0)
			{
				if (CloseHandle(snapshotHandle) == FALSE)
				{
					DWORD lastError = GetLastError();

					CloseHandle(this->remoteProcessHandle);
					FreeLibrary(this->moduleHandle);

					throw std::runtime_error(Shared::stringFormat("Failed to close tool help snapshot\nSystem error code: %u\n", lastError));
				}

				memcpy(&this->remoteModuleEntry, &currentModuleEntry, sizeof(MODULEENTRY32W));

				this->injected = true;

				return;
			}
		} while (Module32NextW(snapshotHandle, &currentModuleEntry) == TRUE);
	}

	void RemoteLibrary::eject()
	{
		if (!this->injected)
		{
			return;
		}

		this->injected = false;

		HANDLE remoteThreadHandle = CreateRemoteThread(this->remoteProcessHandle, NULL, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(getFreeLibraryAddress()), this->remoteModuleEntry.hModule, 0, NULL);
		if (remoteThreadHandle == NULL)
		{
			DWORD lastError = GetLastError();

			CloseHandle(this->remoteProcessHandle);
			ZeroMemory(&this->remoteModuleEntry, sizeof(MODULEENTRY32W));
			FreeLibrary(this->moduleHandle);

			throw std::runtime_error(Shared::stringFormat("Failed to create remote thread\nSystem error code: %u\n", lastError));
		}

		ZeroMemory(&this->remoteModuleEntry, sizeof(MODULEENTRY32W));

		DWORD waitResult = WaitForSingleObject(remoteThreadHandle, INFINITE);

		if (waitResult != WAIT_OBJECT_0)
		{
			CloseHandle(remoteThreadHandle);
			CloseHandle(this->remoteProcessHandle);
			FreeLibrary(this->moduleHandle);

			throw std::runtime_error(Shared::stringFormat("Remote LoadLibrary wait error\nWaitForSingleObject return code: %u\n", waitResult));
		}

		DWORD remoteThreadExitCode;
		if (GetExitCodeThread(remoteThreadHandle, &remoteThreadExitCode) == FALSE)
		{
			DWORD lastError = GetLastError();

			CloseHandle(remoteThreadHandle);
			CloseHandle(this->remoteProcessHandle);
			FreeLibrary(this->moduleHandle);

			throw std::runtime_error(Shared::stringFormat("Failed to get LoadLibrary's return code\nSystem error code: %u\n", lastError));
		}

		if (remoteThreadExitCode == FALSE)
		{
			CloseHandle(remoteThreadHandle);
			CloseHandle(this->remoteProcessHandle);
			FreeLibrary(this->moduleHandle);

			throw std::runtime_error("Failed to free remote library\n");
		}

		if (CloseHandle(remoteThreadHandle) == FALSE)
		{
			DWORD lastError = GetLastError();

			CloseHandle(this->remoteProcessHandle);
			FreeLibrary(this->moduleHandle);

			throw std::runtime_error(Shared::stringFormat("Failed to close remote thread handle\nSystem error code: %u\n", lastError));
		}

		if (CloseHandle(this->remoteProcessHandle) == FALSE)
		{
			DWORD lastError = GetLastError();

			FreeLibrary(this->moduleHandle);

			throw std::runtime_error(Shared::stringFormat("Failed to close remote process handle\nSystem error code: %u\n", lastError));
		}

		if (FreeLibrary(this->moduleHandle) == FALSE)
		{
			throw std::runtime_error(Shared::stringFormat("Failed to free library handle\nSystem error code: %u\n", GetLastError()));
		}
	}

	DWORD RemoteLibrary::runRemoteFunction(const std::string &functionName, LPVOID argument, std::size_t argumentSize) const
	{
		if (!this->injected)
		{
			throw std::runtime_error("Not injected\n");
		}

		FARPROC localFunctionAddress = GetProcAddress(this->moduleHandle, functionName.c_str());
		if (localFunctionAddress == NULL)
		{
			DWORD lastError = GetLastError();

			throw std::runtime_error(Shared::stringFormat("Failed to get function address\nSystem error code: %u\n", GetLastError()));
		}

		DWORD_PTR remoteFunctionOffset = reinterpret_cast<DWORD_PTR>(this->getRemoteModuleEntry().modBaseAddr) + (reinterpret_cast<DWORD_PTR>(localFunctionAddress) - reinterpret_cast<DWORD_PTR>(this->moduleHandle));

		LPVOID remoteMemoryAddress;
		if (argument == NULL)
		{
			remoteMemoryAddress = NULL;
		}
		else
		{
			remoteMemoryAddress = VirtualAllocEx(this->remoteProcessHandle, NULL, argumentSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
			if (remoteMemoryAddress == NULL)
			{
				throw std::runtime_error(Shared::stringFormat("Failed to allocate remote memory\nSystem error code: %u\n", GetLastError()));
			}

			if (WriteProcessMemory(this->remoteProcessHandle, remoteMemoryAddress, argument, argumentSize, NULL) == FALSE)
			{
				DWORD lastError = GetLastError();

				VirtualFreeEx(this->remoteProcessHandle, remoteMemoryAddress, 0, MEM_RELEASE);

				throw std::runtime_error(Shared::stringFormat("Failed to write to remote memory\nSystem error code: %u\n", lastError));
			}
		}

		HANDLE remoteThreadHandle = CreateRemoteThread(this->remoteProcessHandle, NULL, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(remoteFunctionOffset), remoteMemoryAddress, NULL, 0);
		if (remoteThreadHandle == NULL)
		{
			DWORD lastError = GetLastError();

			if (remoteMemoryAddress != NULL)
			{
				VirtualFreeEx(this->remoteProcessHandle, remoteMemoryAddress, 0, MEM_RELEASE);
			}

			throw std::runtime_error(Shared::stringFormat("Failed to create remote thread\nSystem error code: %u\n", lastError));
		}

		DWORD waitResult = WaitForSingleObject(remoteThreadHandle, INFINITE);

		if (remoteMemoryAddress != NULL)
		{
			if (VirtualFreeEx(this->remoteProcessHandle, remoteMemoryAddress, 0, MEM_RELEASE) == FALSE)
			{
				DWORD lastError = GetLastError();

				CloseHandle(remoteThreadHandle);

				throw std::runtime_error(Shared::stringFormat("Failed to free remote memory\nSystem error code: %u\n", lastError));
			}
		}

		if (waitResult != WAIT_OBJECT_0)
		{
			CloseHandle(remoteThreadHandle);

			throw std::runtime_error(Shared::stringFormat("Remote LoadLibrary wait error\nWaitForSingleObject return code: %u\n", waitResult));
		}

		DWORD remoteThreadExitCode;
		if (GetExitCodeThread(remoteThreadHandle, &remoteThreadExitCode) == FALSE)
		{
			DWORD lastError = GetLastError();

			CloseHandle(remoteThreadHandle);

			throw std::runtime_error(Shared::stringFormat("Failed to get LoadLibrary's return code\nSystem error code: %u\n", lastError));
		}

		if (CloseHandle(remoteThreadHandle) == FALSE)
		{
			throw std::runtime_error(Shared::stringFormat("Failed to close remote thread handle\nSystem error code: %u\n", GetLastError()));
		}

		return remoteThreadExitCode;
	}

	HMODULE RemoteLibrary::getKernelModuleHandle()
	{
		static HMODULE KERNEL_MODULE_HANDLE = GetModuleHandleW(L"Kernel32");
		if (KERNEL_MODULE_HANDLE == NULL)
		{
			static DWORD error = GetLastError();

			throw std::runtime_error(Shared::stringFormat("Failed to retrieve Kernel32 module handle\nSystem error code: %u\n", error));
		}

		return KERNEL_MODULE_HANDLE;
	}

	FARPROC RemoteLibrary::getLoadLibraryAddress()
	{
		static FARPROC LOAD_LIBRARY_ADDRESS = GetProcAddress(getKernelModuleHandle(), "LoadLibraryW");
		if (LOAD_LIBRARY_ADDRESS == NULL)
		{
			static DWORD error = GetLastError();

			throw std::runtime_error(Shared::stringFormat("Failed to retrieve LoadLibrary address\nSystem error code: %u\n", error));
		}

		return LOAD_LIBRARY_ADDRESS;
	}

	FARPROC RemoteLibrary::getFreeLibraryAddress()
	{
		FARPROC FREE_LIBRARY_ADDRESS = GetProcAddress(getKernelModuleHandle(), "FreeLibrary");
		if (FREE_LIBRARY_ADDRESS == NULL)
		{
			static DWORD error = GetLastError();

			throw std::runtime_error(Shared::stringFormat("Failed to retrieve FreeLibrary address\nSystem error code: %u\n", error));
		}

		return FREE_LIBRARY_ADDRESS;
	}

	BOOL RemoteLibrary::isCurrentProcessWow64Process()
	{
		static BOOL *IS_WOW_64_PROCESS = nullptr;
		if (IS_WOW_64_PROCESS == nullptr)
		{
			static DWORD *error = nullptr;
			if (error != nullptr)
			{
				throw std::runtime_error(Shared::stringFormat("Failed to check if the current process is a WoW64 process\nSystem error code: %u\n", *error));
			}

			IS_WOW_64_PROCESS = new BOOL();
			if (IsWow64Process(GetCurrentProcess(), IS_WOW_64_PROCESS) == FALSE)
			{
				error = new DWORD(GetLastError());
				delete IS_WOW_64_PROCESS;

				throw std::runtime_error(Shared::stringFormat("Failed to check if the current process is a WoW64 process\nSystem error code: %u\n", *error));
			}
		}

		return *IS_WOW_64_PROCESS;
	}
}