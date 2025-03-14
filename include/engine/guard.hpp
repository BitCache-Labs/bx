#pragma once

#include <engine/log.hpp>
#include <cstdlib>

LOG_CHANNEL(Guard)

#ifdef DEBUG_BUILD

#define BX_ASSERT(Condition, Message)                               \
    do {                                                            \
        if (!(Condition))                                           \
        {                                                           \
            BX_LOGE(Guard, "Assertion failed: {}", Message);        \
            std::abort();                                           \
        }                                                           \
    } while (false)

#define BX_ENSURE(Condition) BX_ASSERT(Condition, #Condition)
#define BX_FAIL(Message) BX_ASSERT(false, Message)

#else

#define BX_ASSERT(Condition, Message) Condition

#define BX_ENSURE(Condition) BX_ASSERT(Condition, #Condition)
#define BX_FAIL(Message) BX_ASSERT(false, Message)

#endif

#define BX_TRYCATCH(Expr)                                                           \
    try { Expr; }                                                                   \
    catch (const std::exception& e) { BX_LOGE(Guard, e.what()); }

#define BX_TRYCATCH_FAIL(Expr, Msg)                                                 \
    try { Expr; }                                                                   \
    catch (const std::exception& e) { BX_LOGE(Guard, e.what()); BX_FAIL(Msg); }