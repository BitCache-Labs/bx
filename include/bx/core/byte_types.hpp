#pragma once

#include <cstdint>
#include <cstddef>

using c8 = char;
using b8 = bool;

static constexpr b8 bFalse = false;
static constexpr b8 bTrue = true;

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using f32 = float;
using f64 = double;

using SizeType = std::size_t;

using VoidPtr = void*;
using CVoidPtr = const void*;

using CharPtr = char*;
using CCharPtr = const char*;