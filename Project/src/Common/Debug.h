#pragma once

#if defined(_WIN32) && defined(_MSC_VER)
    #include <crtdbg.h>
    #ifndef DBG_NEW
        #if defined(_DEBUG)
            #define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
        #else
            #define DBG_NEW new
        #endif
    #endif // DBG_NEW
#else
    #define DBG_NEW new
#endif
