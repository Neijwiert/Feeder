#include "stdafx.h"
#include "Util.h"

namespace Shared
{
	void getExecutionPath(std::wstring &path)
	{
		SetLastError(ERROR_SUCCESS);

		DWORD bufferSize = MAX_PATH;
		wchar_t *buffer = new wchar_t[bufferSize];
		DWORD bufferLength;

		while ((bufferLength = GetModuleFileNameW(NULL, buffer, bufferSize)) == 0 && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
		{
			delete[] buffer;
			bufferSize *= 2;
			buffer = new wchar_t[bufferSize];
		}

		if (GetLastError() != ERROR_SUCCESS)
		{
			delete[] buffer;

			throw std::runtime_error(stringFormat("Failed to get module file name\nSystem error code: %u\n", GetLastError()));
		}

		for (wchar_t *bufferPtr = buffer + bufferLength; bufferPtr >= buffer; bufferPtr--)
		{
			if (bufferPtr[0] == L'\\' || bufferPtr[0] == L'/')
			{
				bufferPtr[0] = L'\0';

				break;
			}
		}

		path = buffer;
		delete[] buffer;
	}
}