#include "stdafx.h"
#include "NamedMutex.h"

#include <Util.h>

namespace Inject
{
	NamedMutex::NamedMutex(const std::wstring &name) :
		mutexHandle(NULL),
		alreadyTaken()
	{
		mutexHandle = CreateMutexW(NULL, TRUE, name.c_str());
		if (mutexHandle == NULL)
		{
			throw std::runtime_error(Shared::stringFormat("Failed to create mutex\nSystem error code %u\n", GetLastError()));
		}

		alreadyTaken = (GetLastError() == ERROR_ALREADY_EXISTS);
	}

	NamedMutex::NamedMutex(NamedMutex &&other) :
		mutexHandle(other.mutexHandle),
		alreadyTaken(other.alreadyTaken)
	{
		other.mutexHandle = NULL;
	}

	NamedMutex &NamedMutex::operator=(NamedMutex &&other)
	{
		release();

		this->mutexHandle = other.mutexHandle;
		this->alreadyTaken = other.alreadyTaken;
		other.mutexHandle = NULL;

		return (*this);
	}

	NamedMutex::~NamedMutex()
	{
		try
		{
			release();
		}
		catch (std::runtime_error)
		{

		}
	}

	void NamedMutex::release()
	{
		if (this->mutexHandle != NULL)
		{
			BOOL result = ReleaseMutex(this->mutexHandle);
			this->mutexHandle = NULL;

			if (result == FALSE)
			{
				throw std::runtime_error(Shared::stringFormat("Failed to release mutex\nSystem error code %u\n", GetLastError()));
			}
		}
	}
}