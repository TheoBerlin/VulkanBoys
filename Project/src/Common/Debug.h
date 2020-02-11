#pragma once

#ifdef _WIN32
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


// #include <crtdbg.h>

// //Memleak debugging
// #if defined(_DEBUG)
//     #define _CRTDBG_MAP_ALLOC
// 	#define DBG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__);
//     #define new DBG_NEW
// #endif
