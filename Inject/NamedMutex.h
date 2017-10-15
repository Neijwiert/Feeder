#pragma once

#include <string>
#include <Windows.h>

namespace Inject
{
	class NamedMutex
	{
		public:
			NamedMutex(const std::wstring &name);
			NamedMutex(NamedMutex &&other);
			NamedMutex &operator=(NamedMutex &&other);
			~NamedMutex();

			inline bool isAlreadyTaken() const
			{
				return this->alreadyTaken;
			}

		private:
			NamedMutex(const NamedMutex &other) = delete;
			NamedMutex &operator=(const NamedMutex &other) = delete;

			void release();

			HANDLE mutexHandle;
			bool alreadyTaken;
	};
}