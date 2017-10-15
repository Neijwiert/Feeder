#include "stdafx.h"
#include "CLRHost.h"

#include <metahost.h>

namespace Feeder
{
	CLRHost::CLRHost(const std::wstring &version) :
		metaHost(NULL),
		runtimeInfo(NULL),
		runtimeHost(NULL),
		started(false),
		version(version)
	{

	}

	CLRHost::CLRHost(const std::wstring &&version) :
		metaHost(NULL),
		runtimeInfo(NULL),
		runtimeHost(NULL),
		started(false),
		version(std::forward<const std::wstring>(version))
	{

	}

	CLRHost::CLRHost(CLRHost &&other) :
		metaHost(other.metaHost),
		runtimeInfo(other.runtimeInfo),
		runtimeHost(other.runtimeHost),
		started(other.started)
	{
		other.metaHost = NULL;
		other.runtimeInfo = NULL;
		other.runtimeHost = NULL;
	}

	CLRHost::~CLRHost()
	{
		release();
	}

	HRESULT CLRHost::start()
	{
		if (this->started)
		{
			throw std::runtime_error("Runtime already started\n");
		}

		HRESULT result = CLRCreateInstance(CLSID_CLRMetaHost, IID_PPV_ARGS(&this->metaHost));
		if (FAILED(result))
		{
			return result;
		}

		result = this->metaHost->GetRuntime(this->version.c_str(), IID_PPV_ARGS(&this->runtimeInfo));
		if (FAILED(result))
		{
			this->metaHost->Release();
			this->metaHost = NULL;

			return result;
		}

		result = this->runtimeInfo->GetInterface(CLSID_CLRRuntimeHost, IID_PPV_ARGS(&this->runtimeHost));
		if (FAILED(result))
		{
			this->runtimeInfo->Release();
			this->runtimeInfo = NULL;

			this->metaHost->Release();
			this->metaHost = NULL;

			return result;
		}

		result = this->runtimeHost->Start();
		if (FAILED(result))
		{
			this->runtimeHost->Release();
			this->runtimeHost = NULL;

			this->runtimeInfo->Release();
			this->runtimeInfo = NULL;

			this->metaHost->Release();
			this->metaHost = NULL;

			return result;
		}

		this->started = true;

		return result;
	}

	void CLRHost::release()
	{
		if (!this->started)
		{
			return;
		}

		this->started = false;

		this->metaHost->Release();
		this->metaHost = NULL;

		this->runtimeInfo->Release();
		this->runtimeInfo = NULL;

		this->runtimeHost->Release();
		this->runtimeHost = NULL;
	}

	HRESULT CLRHost::executeInDefaultAppDomain(_In_ LPCWSTR pwzAssemblyPath, _In_ LPCWSTR pwzTypeName, _In_ LPCWSTR pwzMethodName, _In_ LPCWSTR pwzArgument, _Out_ DWORD *pReturnValue) const
	{
		if (!this->started)
		{
			throw std::runtime_error("Runtime not started\n");
		}

		return this->runtimeHost->ExecuteInDefaultAppDomain(pwzAssemblyPath, pwzTypeName, pwzMethodName, pwzArgument, pReturnValue);
	}
}