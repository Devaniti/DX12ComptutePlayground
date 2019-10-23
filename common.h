#pragma once

#define CACHELINE_SIZE 64

#if defined(_DEBUG)
#define DEBUG_PRINT(message) PRINT(message)
#define DEBUG_PRINTW(message) PRINTW(message)
#define MyAssert(res) {if(!(res)) {DebugBreak();}}
#define CHECKED_CALL(call) MyAssert(call)
#define WIN_CALL(hr) MyAssert(SUCCEEDED(hr))
#define PRINT(message) (std::cout << message << std::endl)
#define PRINTW(message) (std::wcout << message << std::endl)
#else
#define DEBUG_PRINT(message)
#define DEBUG_PRINTW(message)
#define MyAssert(res)
#define CHECKED_CALL(call) (call)
#define WIN_CALL(hr) (hr)
#define PRINT(message)
#define PRINTW(message)
#endif