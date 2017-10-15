#pragma once

#include <string>
#include <Windows.h>
#include <TlHelp32.h>

namespace Inject
{
	class RemoteLibrary
	{
		public:
			RemoteLibrary(DWORD processId, const std::wstring &filePath);
			RemoteLibrary(DWORD processId, const std::wstring &&filePath);
			RemoteLibrary(RemoteLibrary &&other);
			~RemoteLibrary();

			void inject();
			void eject();
			DWORD runRemoteFunction(const std::string &functionName, LPVOID argument, std::size_t argumentSize) const;

			inline DWORD runRemoteFunction(const std::string &functionName) const
			{
				return runRemoteFunction(std::forward<const std::string>(functionName), NULL, 0);
			}

			inline DWORD getProcessId() const
			{
				return this->processId;
			}

			inline void setProcessId(DWORD processId)
			{
				if (this->injected)
				{
					throw std::runtime_error("Cannot set process id while injected\n");
				}

				this->processId = processId;
			}

			inline const std::wstring getFilePath() const
			{
				return this->filePath;
			}

			inline bool isInjected() const
			{
				return this->injected;
			}

			inline const MODULEENTRY32W &getRemoteModuleEntry() const
			{
				return this->remoteModuleEntry;
			}
		private:
			RemoteLibrary(const RemoteLibrary &other) = delete;
			RemoteLibrary &operator=(const RemoteLibrary &other) = delete;
			RemoteLibrary &operator=(RemoteLibrary &&other) = delete;

			static HMODULE getKernelModuleHandle();
			static FARPROC getLoadLibraryAddress();
			static FARPROC getFreeLibraryAddress();
			static BOOL isCurrentProcessWow64Process();

			DWORD processId;
			std::wstring filePath;
			std::wstring fileName;
			bool injected;
			HANDLE remoteProcessHandle;
			MODULEENTRY32W remoteModuleEntry;
			HMODULE moduleHandle;
	};
}
