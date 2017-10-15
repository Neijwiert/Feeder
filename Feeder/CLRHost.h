#pragma once

#include <string>

struct ICLRMetaHost;
struct ICLRRuntimeInfo;
struct ICLRRuntimeHost;

namespace Feeder
{
	class CLRHost
	{
		public:
			CLRHost(const std::wstring &version);
			CLRHost(const std::wstring &&version);
			CLRHost(CLRHost &&other);
			~CLRHost();

			HRESULT start();
			void release();
			HRESULT executeInDefaultAppDomain(_In_ LPCWSTR pwzAssemblyPath, _In_ LPCWSTR pwzTypeName, _In_ LPCWSTR pwzMethodName, _In_ LPCWSTR pwzArgument, _Out_ DWORD *pReturnValue) const;

			inline bool isStarted() const
			{
				return this->started;
			}

			inline const std::wstring &getVersion() const
			{
				return this->version;
			}

		private:
			CLRHost(const CLRHost &other) = delete;
			CLRHost &operator=(const CLRHost &other) = delete;
			CLRHost &operator=(const CLRHost &&other) = delete;

			ICLRMetaHost *metaHost;
			ICLRRuntimeInfo *runtimeInfo;
			ICLRRuntimeHost *runtimeHost;
			bool started;
			std::wstring version;
	};
}