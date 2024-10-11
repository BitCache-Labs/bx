#pragma once

#ifdef _WIN32
#ifdef BUILD_SHARED
#define API __declspec(dllexport)
#else
#define API __declspec(dllimport)
#endif
#else
#define BX_API
#endif