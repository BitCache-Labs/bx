#pragma once

#ifdef _WIN32
#ifdef BX_EXPORT
#define BX_API __declspec(dllexport)
#else
#define BX_API __declspec(dllimport)
#endif
#else
#define BX_API
#endif