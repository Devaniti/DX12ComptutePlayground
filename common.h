#pragma once

#define WIN_CALL(hr) assert(SUCCEEDED(hr))
#define PRINT(message) (std::cout << message << std::endl)
#define PRINTW(message) (std::wcout << message << std::endl)

#define LOGGED_CALL(call) (call, std::cout << #call << std::endl)

#if defined(_DEBUG)
#define DEBUG_PRINT(message) PRINT(message)
#define DEBUG_PRINTW(message) PRINTW(message)
#else
#define DEBUG_PRINT(message)
#define DEBUG_PRINTW(message)
#endif