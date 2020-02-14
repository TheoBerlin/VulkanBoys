#include "Log.h"

#include <stdio.h>
#include <stdarg.h>
#include <mutex>

//GLOBAL VAR -> Good? -> Is probaly ok here since it is local to this module (see static)
static std::mutex g_LogMutex;

void logPrintf(const char* pFormat, ...)
{
	va_list args;
	va_start(args, pFormat);

	{
		std::scoped_lock<std::mutex> lock(g_LogMutex);
		vprintf(pFormat, args);
	}
	
	va_end(args);
}