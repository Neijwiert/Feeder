#pragma once

#include "Shared.h"

namespace Shared
{
	struct InjectionInitialization
	{
		wchar_t *assemblyPath;
		wchar_t *typeName;
		wchar_t *methodName;
		wchar_t *argument;
	};
}