#pragma once

#include <engine/log.hpp>
#include <cstdlib>

LOG_CHANNEL(Guard)

#define BX_ASSERT(Condition, Message) \
    do { \
        if (!(Condition)) \
        { \
			BX_LOGE(Guard, "Assertion failed: {}", Message); \
            std::abort(); \
        } \
    } while (false)

#define BX_ENSURE(Condition) BX_ASSERT(Condition, #Condition)
#define BX_FAIL(Message) BX_ASSERT(false, Message)