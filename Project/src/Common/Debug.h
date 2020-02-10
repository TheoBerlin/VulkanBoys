#pragma once

#include <crtdbg.h>
#ifdef _DEBUG
#ifndef DBG_NEW
#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
#define new DBG_NEW
#endif
#endif  // _DEBUG

// #include <crtdbg.h>

// //Memleak debugging
// #if defined(_DEBUG)
//     #define _CRTDBG_MAP_ALLOC
// 	#define DBG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__);
//     #define new DBG_NEW
// #endif
