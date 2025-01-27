#pragma once

#include <engine/log.hpp>
#include <cstdlib>

#define BX_ASSERT(Condition, Message) \
    do { \
        if (!(Condition)) \
        { \
			BX_LOGE(Engine, "Assertion failed: {}", Message); \
            std::abort(); \
        } \
    } while (false)

#define BX_ENSURE(Condition) BX_ASSERT(Condition, #Condition)
#define BX_FAIL(Message) BX_ASSERT(false, Message)