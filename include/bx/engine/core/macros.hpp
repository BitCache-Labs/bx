
#pragma once

#include "bx/engine/core/byte_types.hpp"
#include "bx/engine/core/log.hpp"

#include <cstdlib>

#define BX_STR(x) #x
#define BX_XSTR(x) BX_STR(x)

#define BX_ARRAYSIZE(_ARR) ((u32)(sizeof(_ARR) / sizeof(*(_ARR))))

#define BX_BIT(x) (1 << (x))

#ifdef BX_DEBUG_BUILD
#define BX_LOGD(...) Log::Print(__FILE__, __LINE__, __func__, LogLevel::LOG_DEBUG, Log::Format(__VA_ARGS__))
#define BX_LOGI(...) Log::Print(__FILE__, __LINE__, __func__, LogLevel::LOG_INFO, Log::Format(__VA_ARGS__))
#define BX_LOGW(...) Log::Print(__FILE__, __LINE__, __func__, LogLevel::LOG_WARNING, Log::Format(__VA_ARGS__))
#define BX_LOGE(...) Log::Print(__FILE__, __LINE__, __func__, LogLevel::LOG_ERROR, Log::Format(__VA_ARGS__))

#define BX_ASSERT(condition, ...) \
    do { \
        if (!(condition)) \
        { \
			BX_LOGE("Assertion failed: {}", Log::Format(__VA_ARGS__)); \
            std::abort(); \
        } \
    } while (false)

#define BX_ENSURE(condition) BX_ASSERT(condition, #condition)
#define BX_FAIL(...) BX_ASSERT(false, __VA_ARGS__)

#else

#define BX_LOGD(...)
#define BX_LOGI(...) Log::Print(__FILE__, __LINE__, __func__, LogLevel::LOG_INFO, Log::Format(__VA_ARGS__))
#define BX_LOGW(...) Log::Print(__FILE__, __LINE__, __func__, LogLevel::LOG_WARNING, Log::Format(__VA_ARGS__))
#define BX_LOGE(...) Log::Print(__FILE__, __LINE__, __func__, LogLevel::LOG_ERROR, Log::Format(__VA_ARGS__))

#define BX_ASSERT(condition, ...)
#define BX_ENSURE(condition)
#define BX_FAIL(...)

#endif

#if defined(__GNUC__) || defined(__clang__)
#define BX_FUNCTION __PRETTY_FUNCTION__
#elif defined(_MSC_VER)
#define BX_FUNCTION __FUNCSIG__
#else
#define BX_FUNCTION __func__
#endif