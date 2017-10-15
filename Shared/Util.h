#pragma once

#include <memory>
#include <string>

#include "Shared.h"

namespace Shared
{
	template<typename ... Args>
	std::string stringFormat(const std::string &format, Args ...args)
	{
		size_t size = std::snprintf(nullptr, 0, format.c_str(), args...) + 1;
		std::unique_ptr<char[]> buf(new char[size]);
		std::snprintf(buf.get(), size, format.c_str(), args...);

		return std::string(buf.get(), buf.get() + size - 1);
	}

	template<typename ... Args>
	std::wstring stringFormat(const std::wstring &format, Args ...args)
	{
		size_t size = std::swprintf(nullptr, 0, format.c_str(), args...) + 1;
		std::unique_ptr<wchar_t[]> buf(new wchar_t[size]);
		std::swprintf(buf.get(), size, format.c_str(), args...);

		return std::wstring(buf.get(), buf.get() + size - 1);
	}

	void getExecutionPath(std::wstring &path);
}