/***********************************************************************
* FUNCTION:     ÄÚ´æÐ¹Â¶¼ì²é
* AUTHOR:       Ìï¿¡
* DATE£º        2015-04-13
* NOTES:        
* MODIFICATION:
**********************************************************************/
#ifndef __MEMORY_LEAK_CHECK_VALUE__
#define __MEMORY_LEAK_CHECK_VALUE__

#ifdef _WIN32

#ifdef MEM_LEAK_CHECK
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

inline void BeginMemLeakCheck()
{
#ifdef MEM_LEAK_CHECK
	_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
#endif
}

inline void EndMemLeakCheck()
{
}
#else

#ifdef MEM_LEAK_CHECK
#include <stdlib.h>
#include <mcheck.h>
#endif

inline void BeginMemLeakCheck()
{
#ifdef MEM_LEAK_CHECK
	setenv("MALLOC_TRACE", "MemLeakCheck.log", 1);
	mtrace();
#endif
}

inline void EndMemLeakCheck()
{
#ifdef MEM_LEAK_CHECK
	muntrace();
#endif
}
#endif

#endif   //  __MEMORY_LEAK_CHECK_VALUE__