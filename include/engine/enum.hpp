#pragma once

#include <engine/traits.hpp>

namespace Enum
{
    //-------------------------------------------------------------------------
    // Basic Conversion Utilities
    //-------------------------------------------------------------------------

    // Enable implicit conversion from EnumT to its underlying type.
    template <typename EnumT>
    constexpr typename meta::enable_if_t<meta::is_enum<EnumT>::value,
        meta::underlying_type_t<EnumT>>
        as_value(EnumT e) noexcept
    {
        return static_cast<meta::underlying_type_t<EnumT>>(e);
    }

    // Enable implicit conversion from underlying type back to EnumT.
    template <typename EnumT>
    constexpr typename meta::enable_if_t<meta::is_enum<EnumT>::value,
        EnumT>
        as_type(meta::underlying_type_t<EnumT> value) noexcept
    {
        return static_cast<EnumT>(value);
    }

    // Utility for getting the count of an enum.
    // The enum must define an item called ENUM_COUNT.
    template <typename EnumT>
    constexpr typename meta::enable_if_t<meta::is_enum<EnumT>::value,
        EnumT>
        count() noexcept
    {
        return EnumT::ENUM_COUNT;
    }

    //-------------------------------------------------------------------------
    // Basic Flag Checks
    //-------------------------------------------------------------------------

    // Returns true if any bit in 'flags' is set (nonzero).
    template <typename EnumT>
    constexpr bool any_set(EnumT flags) noexcept
    {
        return as_value(flags) != 0;
    }

    // Returns true if the specific 'flag' is present in 'flags'.
    template <typename EnumT>
    constexpr bool has_flag(EnumT flags, EnumT flag) noexcept
    {
        return any_set(flags & flag);
    }

    // Returns true if all bits in 'flags' are set in 'mask'.
    template <typename EnumT>
    constexpr bool has_all_flags(EnumT mask, EnumT flags) noexcept
    {
        return (mask & flags) == flags;
    }

    // Returns true if any bit in 'flags' is set in 'mask'.
    template <typename EnumT>
    constexpr bool has_any_flags(EnumT mask, EnumT flags) noexcept
    {
        return any_set(mask & flags);
    }

    //-------------------------------------------------------------------------
    // Flag Modification Utilities
    //-------------------------------------------------------------------------

    // Returns a new mask with 'flag' set.
    template <typename EnumT>
    constexpr EnumT set_flag(EnumT mask, EnumT flag) noexcept
    {
        return mask | flag;
    }

    // Returns a new mask with 'flag' cleared.
    template <typename EnumT>
    constexpr EnumT clear_flag(EnumT mask, EnumT flag) noexcept
    {
        return mask & ~flag;
    }

    // Returns a new mask with 'flag' toggled.
    template <typename EnumT>
    constexpr EnumT toggle_flag(EnumT mask, EnumT flag) noexcept
    {
        return mask ^ flag;
    }

    // Returns the “reset” mask (i.e. with no flags set).
    template <typename EnumT>
    constexpr EnumT reset_flags() noexcept
    {
        return as_type<EnumT>(0);
    }

    //-------------------------------------------------------------------------
    // String Conversion Utilities
    //-------------------------------------------------------------------------

    // Convert an enum value to a string.
    // Note: Specialize this function for each enum type.
    template <typename EnumT>
    constexpr const char* to_string(EnumT value)
    {
        static_assert(meta::is_enum<EnumT>::value, "to_string not implemented for this enum type");
        return "undefined";
    }

    // Convert a string to an enum value.
    // Note: Specialize this function for each enum type.
    template <typename EnumT>
    inline EnumT from_string(const char* str)
    {
        static_assert(meta::is_enum<EnumT>::value, "from_string not implemented for this enum type");
        return as_type<EnumT>(0);
    }

    //-------------------------------------------------------------------------
    // Enum Iteration Support
    //-------------------------------------------------------------------------

    // An iterator for a sequential enum.
    template <typename EnumT>
    struct enum_iterator
    {
        using underlying = meta::underlying_type_t<EnumT>;
        underlying current;

        constexpr explicit enum_iterator(EnumT value) noexcept : current(as_value(value)) { }

        constexpr EnumT operator*() const noexcept { return as_type<EnumT>(current); }

        constexpr enum_iterator& operator++() noexcept
        {
            ++current;
            return *this;
        }

        constexpr bool operator!=(const enum_iterator& other) const noexcept
        {
            return current != other.current;
        }
    };

    // A range for iterating over a sequential enum.
    template <typename EnumT>
    struct enum_range
    {
        enum_iterator<EnumT> begin;
        enum_iterator<EnumT> end;
    };

    // Create an iterator range for the enum from 0 to ENUM_COUNT.
    template <typename EnumT>
    constexpr enum_range<EnumT> make_enum_range() noexcept
    {
        return { enum_iterator<EnumT>(as_type<EnumT>(0)), enum_iterator<EnumT>(count<EnumT>()) };
    }
}

//-------------------------------------------------------------------------
// Detection Traits for Enum-to-Enum Operators
//-------------------------------------------------------------------------

namespace Enum
{
    // Bitwise Operators
    template <typename, typename T, typename = void> struct has_bitwise_and : meta::false_type {};
    template <typename T> struct has_bitwise_and<decltype(void(std::declval<T>()& std::declval<T>())), T, void> : meta::true_type {};

    template <typename, typename T, typename = void> struct has_bitwise_or : meta::false_type {};
    template <typename T> struct has_bitwise_or<decltype(void(std::declval<T>() | std::declval<T>())), T, void> : meta::true_type {};

    template <typename, typename T, typename = void> struct has_bitwise_xor : meta::false_type {};
    template <typename T> struct has_bitwise_xor<decltype(void(std::declval<T>() ^ std::declval<T>())), T, void> : meta::true_type {};

    template <typename, typename T, typename = void> struct has_bitwise_not : meta::false_type {};
    template <typename T> struct has_bitwise_not<decltype(void(~std::declval<T>())), T, void> : meta::true_type {};

    // Compound Bitwise Assignment Operators
    template <typename, typename T, typename = void> struct has_compound_and : meta::false_type {};
    template <typename T> struct has_compound_and<decltype(void(std::declval<T&>() &= std::declval<T>())), T, void> : meta::true_type {};

    template <typename, typename T, typename = void> struct has_compound_or : meta::false_type {};
    template <typename T> struct has_compound_or<decltype(void(std::declval<T&>() |= std::declval<T>())), T, void> : meta::true_type {};

    template <typename, typename T, typename = void> struct has_compound_xor : meta::false_type {};
    template <typename T> struct has_compound_xor<decltype(void(std::declval<T&>() ^= std::declval<T>())), T, void> : meta::true_type {};

    // Comparison Operators
    template <typename, typename T, typename = void> struct has_greater_equal : meta::false_type {};
    template <typename T> struct has_greater_equal<decltype(void(std::declval<T>() >= std::declval<T>())), T, void> : meta::true_type {};

    template <typename, typename T, typename = void> struct has_less_equal : meta::false_type {};
    template <typename T> struct has_less_equal<decltype(void(std::declval<T>() <= std::declval<T>())), T, void> : meta::true_type {};

    template <typename, typename T, typename = void> struct has_greater : meta::false_type {};
    template <typename T> struct has_greater<decltype(void(std::declval<T>() > std::declval<T>())), T, void> : meta::true_type{};

    template <typename, typename T, typename = void> struct has_less : meta::false_type {};
    template <typename T> struct has_less<decltype(void(std::declval<T>() < std::declval<T>())), T, void> : meta::true_type{};

    // Arithmetic Operators
    template <typename, typename T, typename = void> struct has_plus : meta::false_type {};
    template <typename T> struct has_plus<decltype(void(std::declval<T>() + std::declval<T>())), T, void> : meta::true_type {};

    template <typename, typename T, typename = void> struct has_minus : meta::false_type {};
    template <typename T> struct has_minus<decltype(void(std::declval<T>() - std::declval<T>())), T, void> : meta::true_type {};

    template <typename, typename T, typename = void> struct has_multiply : meta::false_type {};
    template <typename T> struct has_multiply<decltype(void(std::declval<T>()* std::declval<T>())), T, void> : meta::true_type {};

    template <typename, typename T, typename = void> struct has_divide : meta::false_type {};
    template <typename T> struct has_divide<decltype(void(std::declval<T>() / std::declval<T>())), T, void> : meta::true_type {};

    template <typename, typename T, typename = void> struct has_modulo : meta::false_type {};
    template <typename T> struct has_modulo<decltype(void(std::declval<T>() % std::declval<T>())), T, void> : meta::true_type {};

    // Compound Arithmetic Assignment Operators
    template <typename, typename T, typename = void> struct has_compound_plus : meta::false_type {};
    template <typename T> struct has_compound_plus<decltype(void(std::declval<T&>() += std::declval<T>())), T, void> : meta::true_type {};

    template <typename, typename T, typename = void> struct has_compound_minus : meta::false_type {};
    template <typename T> struct has_compound_minus<decltype(void(std::declval<T&>() -= std::declval<T>())), T, void> : meta::true_type {};

    template <typename, typename T, typename = void> struct has_compound_multiply : meta::false_type {};
    template <typename T> struct has_compound_multiply<decltype(void(std::declval<T&>() *= std::declval<T>())), T, void> : meta::true_type {};

    template <typename, typename T, typename = void> struct has_compound_divide : meta::false_type {};
    template <typename T> struct has_compound_divide<decltype(void(std::declval<T&>() /= std::declval<T>())), T, void> : meta::true_type {};

    template <typename, typename T, typename = void> struct has_compound_modulo : meta::false_type {};
    template <typename T> struct has_compound_modulo<decltype(void(std::declval<T&>() %= std::declval<T>())), T, void> : meta::true_type {};
}

//
// ---------------- Enum-to-Enum Operator Overloads ----------------
//

// --- Bitwise Operators ---

// Bitwise AND operator for EnumT, EnumT.
template <typename EnumT>
constexpr meta::enable_if_t<
    meta::is_enum<EnumT>::value && !Enum::has_bitwise_and<void, EnumT>::value,
    EnumT>
    operator&(EnumT lhs, EnumT rhs) noexcept
{
    return Enum::as_type<EnumT>(Enum::as_value(lhs) & Enum::as_value(rhs));
}

// Bitwise OR operator for EnumT, EnumT.
template <typename EnumT>
constexpr meta::enable_if_t<
    meta::is_enum<EnumT>::value && !Enum::has_bitwise_or<void, EnumT>::value,
    EnumT>
    operator|(EnumT lhs, EnumT rhs) noexcept
{
    return Enum::as_type<EnumT>(Enum::as_value(lhs) | Enum::as_value(rhs));
}

// Bitwise XOR operator for EnumT, EnumT.
template <typename EnumT>
constexpr meta::enable_if_t<
    meta::is_enum<EnumT>::value && !Enum::has_bitwise_xor<void, EnumT>::value,
    EnumT>
    operator^(EnumT lhs, EnumT rhs) noexcept
{
    return Enum::as_type<EnumT>(Enum::as_value(lhs) ^ Enum::as_value(rhs));
}

// Bitwise NOT operator for EnumT.
template <typename EnumT>
constexpr meta::enable_if_t<
    meta::is_enum<EnumT>::value && !Enum::has_bitwise_not<void, EnumT>::value,
    EnumT>
    operator~(EnumT e) noexcept
{
    return Enum::as_type<EnumT>(~Enum::as_value(e));
}

// --- Compound Bitwise Assignment Operators ---

// Compound assignment AND operator for EnumT, EnumT.
template <typename EnumT>
constexpr meta::enable_if_t<
    meta::is_enum<EnumT>::value && !Enum::has_compound_and<void, EnumT>::value,
    EnumT&>
    operator&=(EnumT& lhs, EnumT rhs) noexcept
{
    lhs = lhs & rhs;
    return lhs;
}

// Compound assignment OR operator for EnumT, EnumT.
template <typename EnumT>
constexpr meta::enable_if_t<
    meta::is_enum<EnumT>::value && !Enum::has_compound_or<void, EnumT>::value,
    EnumT&>
    operator|=(EnumT& lhs, EnumT rhs) noexcept
{
    lhs = lhs | rhs;
    return lhs;
}

// Compound assignment XOR operator for EnumT, EnumT.
template <typename EnumT>
constexpr meta::enable_if_t<
    meta::is_enum<EnumT>::value && !Enum::has_compound_xor<void, EnumT>::value,
    EnumT&>
    operator^=(EnumT& lhs, EnumT rhs) noexcept
{
    lhs = lhs ^ rhs;
    return lhs;
}

// --- Comparison Operators ---

// Greater than or equal comparison operator for EnumT, EnumT.
template <typename EnumT>
constexpr meta::enable_if_t<
    meta::is_enum<EnumT>::value && !Enum::has_greater_equal<void, EnumT>::value,
    bool>
    operator>=(EnumT lhs, EnumT rhs) noexcept
{
    return Enum::as_value(lhs) >= Enum::as_value(rhs);
}

// Less than or equal comparison operator for EnumT, EnumT.
template <typename EnumT>
constexpr meta::enable_if_t<
    meta::is_enum<EnumT>::value && !Enum::has_less_equal<void, EnumT>::value,
    bool>
    operator<=(EnumT lhs, EnumT rhs) noexcept
{
    return Enum::as_value(lhs) <= Enum::as_value(rhs);
}

// Greater than comparison operator for EnumT, EnumT.
template <typename EnumT>
constexpr meta::enable_if_t<
    meta::is_enum<EnumT>::value && !Enum::has_greater<void, EnumT>::value,
    bool>
    operator>(EnumT lhs, EnumT rhs) noexcept
{
    return Enum::as_value(lhs) > Enum::as_value(rhs);
}

// Less than comparison operator for EnumT, EnumT.
template <typename EnumT>
constexpr meta::enable_if_t<
    meta::is_enum<EnumT>::value && !Enum::has_less<void, EnumT>::value,
    bool>
    operator<(EnumT lhs, EnumT rhs) noexcept
{
    return Enum::as_value(lhs) < Enum::as_value(rhs);
}

// --- Arithmetic Operators ---

// Arithmetic addition operator for EnumT, EnumT.
template <typename EnumT>
constexpr meta::enable_if_t<
    meta::is_enum<EnumT>::value && !Enum::has_plus<void, EnumT>::value,
    EnumT>
    operator+(EnumT lhs, EnumT rhs) noexcept
{
    return Enum::as_type<EnumT>(Enum::as_value(lhs) + Enum::as_value(rhs));
}

// Arithmetic subtraction operator for EnumT, EnumT.
template <typename EnumT>
constexpr meta::enable_if_t<
    meta::is_enum<EnumT>::value && !Enum::has_minus<void, EnumT>::value,
    EnumT>
    operator-(EnumT lhs, EnumT rhs) noexcept
{
    return Enum::as_type<EnumT>(Enum::as_value(lhs) - Enum::as_value(rhs));
}

// Arithmetic multiplication operator for EnumT, EnumT.
template <typename EnumT>
constexpr meta::enable_if_t<
    meta::is_enum<EnumT>::value && !Enum::has_multiply<void, EnumT>::value,
    EnumT>
    operator*(EnumT lhs, EnumT rhs) noexcept
{
    return Enum::as_type<EnumT>(Enum::as_value(lhs) * Enum::as_value(rhs));
}

// Arithmetic division operator for EnumT, EnumT.
template <typename EnumT>
constexpr meta::enable_if_t<
    meta::is_enum<EnumT>::value && !Enum::has_divide<void, EnumT>::value,
    EnumT>
    operator/(EnumT lhs, EnumT rhs) noexcept
{
    return Enum::as_type<EnumT>(Enum::as_value(lhs) / Enum::as_value(rhs));
}

// Arithmetic modulo operator for EnumT, EnumT.
template <typename EnumT>
constexpr meta::enable_if_t<
    meta::is_enum<EnumT>::value && !Enum::has_modulo<void, EnumT>::value,
    EnumT>
    operator%(EnumT lhs, EnumT rhs) noexcept
{
    return Enum::as_type<EnumT>(Enum::as_value(lhs) % Enum::as_value(rhs));
}

// --- Compound Arithmetic Assignment Operators ---

// Compound assignment addition operator for EnumT, EnumT.
template <typename EnumT>
constexpr meta::enable_if_t<
    meta::is_enum<EnumT>::value && !Enum::has_compound_plus<void, EnumT>::value,
    EnumT&>
    operator+=(EnumT& lhs, EnumT rhs) noexcept
{
    lhs = lhs + rhs;
    return lhs;
}

// Compound assignment subtraction operator for EnumT, EnumT.
template <typename EnumT>
constexpr meta::enable_if_t<
    meta::is_enum<EnumT>::value && !Enum::has_compound_minus<void, EnumT>::value,
    EnumT&>
    operator-=(EnumT& lhs, EnumT rhs) noexcept
{
    lhs = lhs - rhs;
    return lhs;
}

// Compound assignment multiplication operator for EnumT, EnumT.
template <typename EnumT>
constexpr meta::enable_if_t<
    meta::is_enum<EnumT>::value && !Enum::has_compound_multiply<void, EnumT>::value,
    EnumT&>
    operator*=(EnumT& lhs, EnumT rhs) noexcept
{
    lhs = lhs * rhs;
    return lhs;
}

// Compound assignment division operator for EnumT, EnumT.
template <typename EnumT>
constexpr meta::enable_if_t<
    meta::is_enum<EnumT>::value && !Enum::has_compound_divide<void, EnumT>::value,
    EnumT&>
    operator/=(EnumT& lhs, EnumT rhs) noexcept
{
    lhs = lhs / rhs;
    return lhs;
}

// Compound assignment modulo operator for EnumT, EnumT.
template <typename EnumT>
constexpr meta::enable_if_t<
    meta::is_enum<EnumT>::value && !Enum::has_compound_modulo<void, EnumT>::value,
    EnumT&>
    operator%=(EnumT& lhs, EnumT rhs) noexcept
{
    lhs = lhs % rhs;
    return lhs;
}

//-------------------------------------------------------------------------
// Detection Traits for Operators: EnumT, UnderlyingType
//-------------------------------------------------------------------------

namespace Enum
{
    // Bitwise Operators
    template <typename, typename T, typename = void> struct has_bitwise_and_enum_underlying : meta::false_type {};
    template <typename T> struct has_bitwise_and_enum_underlying<decltype(void(std::declval<T>()& std::declval<meta::underlying_type_t<T>>())), T, void> : meta::true_type {};

    template <typename, typename T, typename = void> struct has_bitwise_or_enum_underlying : meta::false_type {};
    template <typename T> struct has_bitwise_or_enum_underlying<decltype(void(std::declval<T>() | std::declval<meta::underlying_type_t<T>>())), T, void> : meta::true_type {};

    template <typename, typename T, typename = void> struct has_bitwise_xor_enum_underlying : meta::false_type {};
    template <typename T> struct has_bitwise_xor_enum_underlying<decltype(void(std::declval<T>() ^ std::declval<meta::underlying_type_t<T>>())), T, void> : meta::true_type {};

    // Compound Bitwise Assignment Operators
    template <typename, typename T, typename = void> struct has_compound_and_enum_underlying : meta::false_type {};
    template <typename T> struct has_compound_and_enum_underlying<decltype(void(std::declval<T&>() &= std::declval<meta::underlying_type_t<T>>())), T, void> : meta::true_type {};

    template <typename, typename T, typename = void> struct has_compound_or_enum_underlying : meta::false_type {};
    template <typename T> struct has_compound_or_enum_underlying<decltype(void(std::declval<T&>() |= std::declval<meta::underlying_type_t<T>>())), T, void> : meta::true_type {};

    template <typename, typename T, typename = void> struct has_compound_xor_enum_underlying : meta::false_type {};
    template <typename T> struct has_compound_xor_enum_underlying<decltype(void(std::declval<T&>() ^= std::declval<meta::underlying_type_t<T>>())), T, void> : meta::true_type {};

    // Comparison Operators
    template <typename, typename T, typename = void> struct has_greater_equal_enum_underlying : meta::false_type {};
    template <typename T> struct has_greater_equal_enum_underlying<decltype(void(std::declval<T>() >= std::declval<meta::underlying_type_t<T>>())), T, void> : meta::true_type {};

    template <typename, typename T, typename = void> struct has_less_equal_enum_underlying : meta::false_type {};
    template <typename T> struct has_less_equal_enum_underlying<decltype(void(std::declval<T>() <= std::declval<meta::underlying_type_t<T>>())), T, void> : meta::true_type {};

    template <typename, typename T, typename = void> struct has_greater_enum_underlying : meta::false_type {};
    template <typename T> struct has_greater_enum_underlying<decltype(void(std::declval<T>() > std::declval<meta::underlying_type_t<T>>())), T, void> : meta::true_type {};

    template <typename, typename T, typename = void> struct has_less_enum_underlying : meta::false_type {};
    template <typename T> struct has_less_enum_underlying<decltype(void(std::declval<T>() < std::declval<meta::underlying_type_t<T>>())), T, void> : meta::true_type{};

    // Arithmetic Operators
    template <typename, typename T, typename = void> struct has_plus_enum_underlying : meta::false_type {};
    template <typename T> struct has_plus_enum_underlying<decltype(void(std::declval<T>() + std::declval<meta::underlying_type_t<T>>())), T, void> : meta::true_type {};

    template <typename, typename T, typename = void> struct has_minus_enum_underlying : meta::false_type {};
    template <typename T> struct has_minus_enum_underlying<decltype(void(std::declval<T>() - std::declval<meta::underlying_type_t<T>>())), T, void> : meta::true_type {};

    template <typename, typename T, typename = void> struct has_multiply_enum_underlying : meta::false_type {};
    template <typename T> struct has_multiply_enum_underlying<decltype(void(std::declval<T>()* std::declval<meta::underlying_type_t<T>>())), T, void> : meta::true_type {};

    template <typename, typename T, typename = void> struct has_divide_enum_underlying : meta::false_type {};
    template <typename T> struct has_divide_enum_underlying<decltype(void(std::declval<T>() / std::declval<meta::underlying_type_t<T>>())), T, void> : meta::true_type {};

    template <typename, typename T, typename = void> struct has_modulo_enum_underlying : meta::false_type {};
    template <typename T> struct has_modulo_enum_underlying<decltype(void(std::declval<T>() % std::declval<meta::underlying_type_t<T>>())), T, void> : meta::true_type {};

    // Compound Arithmetic Assignment Operators
    template <typename, typename T, typename = void> struct has_compound_plus_enum_underlying : meta::false_type {};
    template <typename T> struct has_compound_plus_enum_underlying<decltype(void(std::declval<T&>() += std::declval<meta::underlying_type_t<T>>())), T, void> : meta::true_type {};

    template <typename, typename T, typename = void> struct has_compound_minus_enum_underlying : meta::false_type {};
    template <typename T> struct has_compound_minus_enum_underlying<decltype(void(std::declval<T&>() -= std::declval<meta::underlying_type_t<T>>())), T, void> : meta::true_type {};

    template <typename, typename T, typename = void> struct has_compound_multiply_enum_underlying : meta::false_type {};
    template <typename T> struct has_compound_multiply_enum_underlying<decltype(void(std::declval<T&>() *= std::declval<meta::underlying_type_t<T>>())), T, void> : meta::true_type {};

    template <typename, typename T, typename = void> struct has_compound_divide_enum_underlying : meta::false_type {};
    template <typename T> struct has_compound_divide_enum_underlying<decltype(void(std::declval<T&>() /= std::declval<meta::underlying_type_t<T>>())), T, void> : meta::true_type {};

    template <typename, typename T, typename = void> struct has_compound_modulo_enum_underlying : meta::false_type {};
    template <typename T> struct has_compound_modulo_enum_underlying<decltype(void(std::declval<T&>() %= std::declval<meta::underlying_type_t<T>>())), T, void> : meta::true_type {};
}

//
// ---------------- Enum-to-UnderlyingType Operator Overloads ----------------
//

// --- Bitwise Operators ---

// Bitwise AND operator for EnumT, UnderlyingType.
template <typename EnumT>
constexpr meta::enable_if_t<
    meta::is_enum<EnumT>::value && !Enum::has_bitwise_and_enum_underlying<void, EnumT>::value,
    EnumT>
    operator&(EnumT lhs, meta::underlying_type_t<EnumT> rhs) noexcept
{
    return Enum::as_type<EnumT>(Enum::as_value(lhs) & rhs);
}

// Bitwise OR operator for EnumT, UnderlyingType.
template <typename EnumT>
constexpr meta::enable_if_t<
    meta::is_enum<EnumT>::value && !Enum::has_bitwise_or_enum_underlying<void, EnumT>::value,
    EnumT>
    operator|(EnumT lhs, meta::underlying_type_t<EnumT> rhs) noexcept
{
    return Enum::as_type<EnumT>(Enum::as_value(lhs) | rhs);
}

// Bitwise XOR operator for EnumT, UnderlyingType.
template <typename EnumT>
constexpr meta::enable_if_t<
    meta::is_enum<EnumT>::value && !Enum::has_bitwise_xor_enum_underlying<void, EnumT>::value,
    EnumT>
    operator^(EnumT lhs, meta::underlying_type_t<EnumT> rhs) noexcept
{
    return Enum::as_type<EnumT>(Enum::as_value(lhs) ^ rhs);
}

// --- Compound Bitwise Assignment Operators ---

// Compound assignment AND operator for EnumT, UnderlyingType.
template <typename EnumT>
constexpr meta::enable_if_t<
    meta::is_enum<EnumT>::value && !Enum::has_compound_and_enum_underlying<void, EnumT>::value,
    EnumT&>
    operator&=(EnumT& lhs, meta::underlying_type_t<EnumT> rhs) noexcept
{
    lhs = lhs & rhs;
    return lhs;
}

// Compound assignment OR operator for EnumT, UnderlyingType.
template <typename EnumT>
constexpr meta::enable_if_t<
    meta::is_enum<EnumT>::value && !Enum::has_compound_or_enum_underlying<void, EnumT>::value,
    EnumT&>
    operator|=(EnumT& lhs, meta::underlying_type_t<EnumT> rhs) noexcept
{
    lhs = lhs | rhs;
    return lhs;
}

// Compound assignment XOR operator for EnumT, UnderlyingType.
template <typename EnumT>
constexpr meta::enable_if_t<
    meta::is_enum<EnumT>::value && !Enum::has_compound_xor_enum_underlying<void, EnumT>::value,
    EnumT&>
    operator^=(EnumT& lhs, meta::underlying_type_t<EnumT> rhs) noexcept
{
    lhs = lhs ^ rhs;
    return lhs;
}

// --- Comparison Operators ---

// Greater than or equal comparison operator for EnumT, UnderlyingType.
template <typename EnumT>
constexpr meta::enable_if_t<
    meta::is_enum<EnumT>::value && !Enum::has_greater_equal_enum_underlying<void, EnumT>::value,
    bool>
    operator>=(EnumT lhs, meta::underlying_type_t<EnumT> rhs) noexcept
{
    return Enum::as_value(lhs) >= rhs;
}

// Less than or equal comparison operator for EnumT, UnderlyingType.
template <typename EnumT>
constexpr meta::enable_if_t<
    meta::is_enum<EnumT>::value && !Enum::has_less_equal_enum_underlying<void, EnumT>::value,
    bool>
    operator<=(EnumT lhs, meta::underlying_type_t<EnumT> rhs) noexcept
{
    return Enum::as_value(lhs) <= rhs;
}

// Greater than comparison operator for EnumT, UnderlyingType.
template <typename EnumT>
constexpr meta::enable_if_t<
    meta::is_enum<EnumT>::value &&
    !Enum::has_greater_enum_underlying<void, EnumT>::value,
    bool>
    operator>(EnumT lhs, meta::underlying_type_t<EnumT> rhs) noexcept
{
    return Enum::as_value(lhs) > rhs;
}

// Less than comparison operator for EnumT, UnderlyingType.
template <typename EnumT>
constexpr meta::enable_if_t<
    meta::is_enum<EnumT>::value && !Enum::has_less_enum_underlying<void, EnumT>::value,
    bool>
    operator<(EnumT lhs, meta::underlying_type_t<EnumT> rhs) noexcept
{
    return Enum::as_value(lhs) < rhs;
}

// --- Arithmetic Operators ---

// Arithmetic addition operator for EnumT, UnderlyingType.
template <typename EnumT>
constexpr meta::enable_if_t<
    meta::is_enum<EnumT>::value && !Enum::has_plus_enum_underlying<void, EnumT>::value,
    EnumT>
    operator+(EnumT lhs, meta::underlying_type_t<EnumT> rhs) noexcept
{
    return Enum::as_type<EnumT>(Enum::as_value(lhs) + rhs);
}

// Arithmetic subtraction operator for EnumT, UnderlyingType.
template <typename EnumT>
constexpr meta::enable_if_t<
    meta::is_enum<EnumT>::value && !Enum::has_minus_enum_underlying<void, EnumT>::value,
    EnumT>
    operator-(EnumT lhs, meta::underlying_type_t<EnumT> rhs) noexcept
{
    return Enum::as_type<EnumT>(Enum::as_value(lhs) - rhs);
}

// Arithmetic multiplication operator for EnumT, UnderlyingType.
template <typename EnumT>
constexpr meta::enable_if_t<
    meta::is_enum<EnumT>::value && !Enum::has_multiply_enum_underlying<void, EnumT>::value,
    EnumT>
    operator*(EnumT lhs, meta::underlying_type_t<EnumT> rhs) noexcept
{
    return Enum::as_type<EnumT>(Enum::as_value(lhs) * rhs);
}

// Arithmetic division operator for EnumT, UnderlyingType.
template <typename EnumT>
constexpr meta::enable_if_t<
    meta::is_enum<EnumT>::value && !Enum::has_divide_enum_underlying<void, EnumT>::value,
    EnumT>
    operator/(EnumT lhs, meta::underlying_type_t<EnumT> rhs) noexcept
{
    return Enum::as_type<EnumT>(Enum::as_value(lhs) / rhs);
}

// Arithmetic modulo operator for EnumT, UnderlyingType.
template <typename EnumT>
constexpr meta::enable_if_t<
    meta::is_enum<EnumT>::value && !Enum::has_modulo_enum_underlying<void, EnumT>::value,
    EnumT>
    operator%(EnumT lhs, meta::underlying_type_t<EnumT> rhs) noexcept
{
    return Enum::as_type<EnumT>(Enum::as_value(lhs) % rhs);
}

// --- Compound Arithmetic Assignment Operators ---

// Compound assignment addition operator for EnumT, UnderlyingType.
template <typename EnumT>
constexpr meta::enable_if_t<
    meta::is_enum<EnumT>::value && !Enum::has_compound_plus_enum_underlying<void, EnumT>::value,
    EnumT&>
    operator+=(EnumT& lhs, meta::underlying_type_t<EnumT> rhs) noexcept
{
    lhs = lhs + rhs;
    return lhs;
}

// Compound assignment subtraction operator for EnumT, UnderlyingType.
template <typename EnumT>
constexpr meta::enable_if_t<
    meta::is_enum<EnumT>::value && !Enum::has_compound_minus_enum_underlying<void, EnumT>::value,
    EnumT&>
    operator-=(EnumT& lhs, meta::underlying_type_t<EnumT> rhs) noexcept
{
    lhs = lhs - rhs;
    return lhs;
}

// Compound assignment multiplication operator for EnumT, UnderlyingType.
template <typename EnumT>
constexpr meta::enable_if_t<
    meta::is_enum<EnumT>::value && !Enum::has_compound_multiply_enum_underlying<void, EnumT>::value,
    EnumT&>
    operator*=(EnumT& lhs, meta::underlying_type_t<EnumT> rhs) noexcept
{
    lhs = lhs * rhs;
    return lhs;
}

// Compound assignment division operator for EnumT, UnderlyingType.
template <typename EnumT>
constexpr meta::enable_if_t<
    meta::is_enum<EnumT>::value && !Enum::has_compound_divide_enum_underlying<void, EnumT>::value,
    EnumT&>
    operator/=(EnumT& lhs, meta::underlying_type_t<EnumT> rhs) noexcept
{
    lhs = lhs / rhs;
    return lhs;
}

// Compound assignment modulo operator for EnumT, UnderlyingType.
template <typename EnumT>
constexpr meta::enable_if_t<
    meta::is_enum<EnumT>::value && !Enum::has_compound_modulo_enum_underlying<void, EnumT>::value,
    EnumT&>
    operator%=(EnumT& lhs, meta::underlying_type_t<EnumT> rhs) noexcept
{
    lhs = lhs % rhs;
    return lhs;
}

//-------------------------------------------------------------------------
// Detection Traits for Operators: UnderlyingType, EnumT
//-------------------------------------------------------------------------

namespace Enum
{
    // Bitwise Operators
    template <typename, typename T, typename = void> struct has_bitwise_and_underlying_enum : meta::false_type {};
    template <typename T> struct has_bitwise_and_underlying_enum<decltype(void(std::declval<meta::underlying_type_t<T>>()& std::declval<T>())), T, void> : meta::true_type {};

    template <typename, typename T, typename = void> struct has_bitwise_or_underlying_enum : meta::false_type {};
    template <typename T> struct has_bitwise_or_underlying_enum<decltype(void(std::declval<meta::underlying_type_t<T>>() | std::declval<T>())), T, void> : meta::true_type {};

    template <typename, typename T, typename = void> struct has_bitwise_xor_underlying_enum : meta::false_type {};
    template <typename T> struct has_bitwise_xor_underlying_enum<decltype(void(std::declval<meta::underlying_type_t<T>>() ^ std::declval<T>())), T, void> : meta::true_type {};

    // Compound Bitwise Assignment Operators
    template <typename, typename T, typename = void> struct has_compound_and_underlying_enum : meta::false_type {};
    template <typename T> struct has_compound_and_underlying_enum<decltype(void(std::declval<meta::underlying_type_t<T>>() &= std::declval<T>())), T, void> : meta::true_type {};

    template <typename, typename T, typename = void> struct has_compound_or_underlying_enum : meta::false_type {};
    template <typename T> struct has_compound_or_underlying_enum<decltype(void(std::declval<meta::underlying_type_t<T>>() |= std::declval<T>())), T, void> : meta::true_type {};

    template <typename, typename T, typename = void> struct has_compound_xor_underlying_enum : meta::false_type {};
    template <typename T> struct has_compound_xor_underlying_enum<decltype(void(std::declval<meta::underlying_type_t<T>>() ^= std::declval<T>())), T, void> : meta::true_type {};

    // Comparison Operators
    template <typename, typename T, typename = void> struct has_greater_equal_underlying_enum : meta::false_type {};
    template <typename T> struct has_greater_equal_underlying_enum<decltype(void(std::declval<meta::underlying_type_t<T>>() >= std::declval<T>())), T, void> : meta::true_type {};

    template <typename, typename T, typename = void> struct has_less_equal_underlying_enum : meta::false_type {};
    template <typename T> struct has_less_equal_underlying_enum<decltype(void(std::declval<meta::underlying_type_t<T>>() <= std::declval<T>())), T, void> : meta::true_type {};

    template <typename, typename T, typename = void> struct has_greater_underlying_enum : meta::false_type {};
    template <typename T> struct has_greater_underlying_enum<decltype(void(std::declval<meta::underlying_type_t<T>>() > std::declval<T>())), T, void> : meta::true_type{};

    template <typename, typename T, typename = void> struct has_less_underlying_enum : meta::false_type {};
    template <typename T> struct has_less_underlying_enum<decltype(void(std::declval<meta::underlying_type_t<T>>() < std::declval<T>())), T, void> : meta::true_type{};

    // Arithmetic Operators
    template <typename, typename T, typename = void> struct has_plus_underlying_enum : meta::false_type {};
    template <typename T> struct has_plus_underlying_enum<decltype(void(std::declval<meta::underlying_type_t<T>>() + std::declval<T>())), T, void> : meta::true_type {};

    template <typename, typename T, typename = void> struct has_minus_underlying_enum : meta::false_type {};
    template <typename T> struct has_minus_underlying_enum<decltype(void(std::declval<meta::underlying_type_t<T>>() - std::declval<T>())), T, void> : meta::true_type {};

    template <typename, typename T, typename = void> struct has_multiply_underlying_enum : meta::false_type {};
    template <typename T> struct has_multiply_underlying_enum<decltype(void(std::declval<meta::underlying_type_t<T>>()* std::declval<T>())), T, void> : meta::true_type {};

    template <typename, typename T, typename = void> struct has_divide_underlying_enum : meta::false_type {};
    template <typename T> struct has_divide_underlying_enum<decltype(void(std::declval<meta::underlying_type_t<T>>() / std::declval<T>())), T, void> : meta::true_type {};

    template <typename, typename T, typename = void> struct has_modulo_underlying_enum : meta::false_type {};
    template <typename T> struct has_modulo_underlying_enum<decltype(void(std::declval<meta::underlying_type_t<T>>() % std::declval<T>())), T, void> : meta::true_type {};

    // Compound Arithmetic Assignment Operators
    template <typename, typename T, typename = void> struct has_compound_plus_underlying_enum : meta::false_type {};
    template <typename T> struct has_compound_plus_underlying_enum<decltype(void(std::declval<meta::underlying_type_t<T>>() += std::declval<T>())), T, void> : meta::true_type {};

    template <typename, typename T, typename = void> struct has_compound_minus_underlying_enum : meta::false_type {};
    template <typename T> struct has_compound_minus_underlying_enum<decltype(void(std::declval<meta::underlying_type_t<T>>() -= std::declval<T>())), T, void> : meta::true_type {};

    template <typename, typename T, typename = void> struct has_compound_multiply_underlying_enum : meta::false_type {};
    template <typename T> struct has_compound_multiply_underlying_enum<decltype(void(std::declval<meta::underlying_type_t<T>>() *= std::declval<T>())), T, void> : meta::true_type {};

    template <typename, typename T, typename = void> struct has_compound_divide_underlying_enum : meta::false_type {};
    template <typename T> struct has_compound_divide_underlying_enum<decltype(void(std::declval<meta::underlying_type_t<T>>() /= std::declval<T>())), T, void> : meta::true_type {};

    template <typename, typename T, typename = void> struct has_compound_modulo_underlying_enum : meta::false_type {};
    template <typename T> struct has_compound_modulo_underlying_enum<decltype(void(std::declval<meta::underlying_type_t<T>>() %= std::declval<T>())), T, void> : meta::true_type {};
}

//
// ---------------- UnderlyingType-to-Enum Operator Overloads ----------------
//

// --- Bitwise Operators ---

// Bitwise AND operator for UnderlyingType, EnumT.
template <typename EnumT>
constexpr meta::enable_if_t<
    meta::is_enum<EnumT>::value && !Enum::has_bitwise_and_underlying_enum<void, EnumT>::value,
    EnumT>
    operator&(meta::underlying_type_t<EnumT> lhs, EnumT rhs) noexcept
{
    return Enum::as_type<EnumT>(lhs & Enum::as_value(rhs));
}

// Bitwise OR operator for UnderlyingType, EnumT.
template <typename EnumT>
constexpr meta::enable_if_t<
    meta::is_enum<EnumT>::value && !Enum::has_bitwise_or_underlying_enum<void, EnumT>::value,
    EnumT>
    operator|(meta::underlying_type_t<EnumT> lhs, EnumT rhs) noexcept
{
    return Enum::as_type<EnumT>(lhs | Enum::as_value(rhs));
}

// Bitwise XOR operator for UnderlyingType, EnumT.
template <typename EnumT>
constexpr meta::enable_if_t<
    meta::is_enum<EnumT>::value && !Enum::has_bitwise_xor_underlying_enum<void, EnumT>::value,
    EnumT>
    operator^(meta::underlying_type_t<EnumT> lhs, EnumT rhs) noexcept
{
    return Enum::as_type<EnumT>(lhs ^ Enum::as_value(rhs));
}

// --- Compound Bitwise Assignment Operators ---

// Compound assignment AND operator for UnderlyingType, EnumT.
template <typename EnumT>
constexpr meta::enable_if_t<
    meta::is_enum<EnumT>::value && !Enum::has_compound_and_underlying_enum<void, EnumT>::value,
    meta::underlying_type_t<EnumT>&>
    operator&=(meta::underlying_type_t<EnumT>& lhs, EnumT rhs) noexcept
{
    lhs = lhs & Enum::as_value(rhs);
    return lhs;
}

// Compound assignment OR operator for UnderlyingType, EnumT.
template <typename EnumT>
constexpr meta::enable_if_t<
    meta::is_enum<EnumT>::value && !Enum::has_compound_or_underlying_enum<void, EnumT>::value,
    meta::underlying_type_t<EnumT>&>
    operator|=(meta::underlying_type_t<EnumT>& lhs, EnumT rhs) noexcept
{
    lhs = lhs | Enum::as_value(rhs);
    return lhs;
}

// Compound assignment XOR operator for UnderlyingType, EnumT.
template <typename EnumT>
constexpr meta::enable_if_t<
    meta::is_enum<EnumT>::value && !Enum::has_compound_xor_underlying_enum<void, EnumT>::value,
    meta::underlying_type_t<EnumT>&>
    operator^=(meta::underlying_type_t<EnumT>& lhs, EnumT rhs) noexcept
{
    lhs = lhs ^ Enum::as_value(rhs);
    return lhs;
}

// --- Comparison Operators ---

// Greater than or equal comparison operator for UnderlyingType, EnumT.
template <typename EnumT>
constexpr meta::enable_if_t<
    meta::is_enum<EnumT>::value && !Enum::has_greater_equal_underlying_enum<void, EnumT>::value,
    bool>
    operator>=(meta::underlying_type_t<EnumT> lhs, EnumT rhs) noexcept
{
    return lhs >= Enum::as_value(rhs);
}

// Less than or equal comparison operator for UnderlyingType, EnumT.
template <typename EnumT>
constexpr meta::enable_if_t<
    meta::is_enum<EnumT>::value && !Enum::has_less_equal_underlying_enum<void, EnumT>::value,
    bool>
    operator<=(meta::underlying_type_t<EnumT> lhs, EnumT rhs) noexcept
{
    return lhs <= Enum::as_value(rhs);
}

// Greater than comparison operator for UnderlyingType, EnumT.
template <typename EnumT>
constexpr meta::enable_if_t<
    meta::is_enum<EnumT>::value && !Enum::has_greater_underlying_enum<void, EnumT>::value,
    bool>
    operator>(meta::underlying_type_t<EnumT> lhs, EnumT rhs) noexcept
{
    return lhs > Enum::as_value(rhs);
}

// Less than comparison operator for UnderlyingType, EnumT.
template <typename EnumT>
constexpr meta::enable_if_t<
    meta::is_enum<EnumT>::value && !Enum::has_less_underlying_enum<void, EnumT>::value,
    bool>
    operator<(meta::underlying_type_t<EnumT> lhs, EnumT rhs) noexcept
{
    return lhs < Enum::as_value(rhs);
}

// --- Arithmetic Operators ---

// Arithmetic addition operator for UnderlyingType, EnumT.
template <typename EnumT>
constexpr meta::enable_if_t<
    meta::is_enum<EnumT>::value && !Enum::has_plus_underlying_enum<void, EnumT>::value,
    EnumT>
    operator+(meta::underlying_type_t<EnumT> lhs, EnumT rhs) noexcept
{
    return Enum::as_type<EnumT>(lhs + Enum::as_value(rhs));
}

// Arithmetic subtraction operator for UnderlyingType, EnumT.
template <typename EnumT>
constexpr meta::enable_if_t<
    meta::is_enum<EnumT>::value && !Enum::has_minus_underlying_enum<void, EnumT>::value,
    EnumT>
    operator-(meta::underlying_type_t<EnumT> lhs, EnumT rhs) noexcept
{
    return Enum::as_type<EnumT>(lhs - Enum::as_value(rhs));
}

// Arithmetic multiplication operator for UnderlyingType, EnumT.
template <typename EnumT>
constexpr meta::enable_if_t<
    meta::is_enum<EnumT>::value && !Enum::has_multiply_underlying_enum<void, EnumT>::value,
    EnumT>
    operator*(meta::underlying_type_t<EnumT> lhs, EnumT rhs) noexcept
{
    return Enum::as_type<EnumT>(lhs * Enum::as_value(rhs));
}

// Arithmetic division operator for UnderlyingType, EnumT.
template <typename EnumT>
constexpr meta::enable_if_t<
    meta::is_enum<EnumT>::value && !Enum::has_divide_underlying_enum<void, EnumT>::value,
    EnumT>
    operator/(meta::underlying_type_t<EnumT> lhs, EnumT rhs) noexcept
{
    return Enum::as_type<EnumT>(lhs / Enum::as_value(rhs));
}

// Arithmetic modulo operator for UnderlyingType, EnumT.
template <typename EnumT>
constexpr meta::enable_if_t<
    meta::is_enum<EnumT>::value && !Enum::has_modulo_underlying_enum<void, EnumT>::value,
    EnumT>
    operator%(meta::underlying_type_t<EnumT> lhs, EnumT rhs) noexcept
{
    return Enum::as_type<EnumT>(lhs % Enum::as_value(rhs));
}

// --- Compound Arithmetic Assignment Operators ---

// Compound assignment addition operator for UnderlyingType, EnumT.
template <typename EnumT>
constexpr meta::enable_if_t<
    meta::is_enum<EnumT>::value && !Enum::has_compound_plus_underlying_enum<void, EnumT>::value,
    meta::underlying_type_t<EnumT>&>
    operator+=(meta::underlying_type_t<EnumT>& lhs, EnumT rhs) noexcept
{
    lhs = lhs + Enum::as_value(rhs);
    return lhs;
}

// Compound assignment subtraction operator for UnderlyingType, EnumT.
template <typename EnumT>
constexpr meta::enable_if_t<
    meta::is_enum<EnumT>::value && !Enum::has_compound_minus_underlying_enum<void, EnumT>::value,
    meta::underlying_type_t<EnumT>&>
    operator-=(meta::underlying_type_t<EnumT>& lhs, EnumT rhs) noexcept
{
    lhs = lhs - Enum::as_value(rhs);
    return lhs;
}

// Compound assignment multiplication operator for UnderlyingType, EnumT.
template <typename EnumT>
constexpr meta::enable_if_t<
    meta::is_enum<EnumT>::value && !Enum::has_compound_multiply_underlying_enum<void, EnumT>::value,
    meta::underlying_type_t<EnumT>&>
    operator*=(meta::underlying_type_t<EnumT>& lhs, EnumT rhs) noexcept
{
    lhs = lhs * Enum::as_value(rhs);
    return lhs;
}

// Compound assignment division operator for UnderlyingType, EnumT.
template <typename EnumT>
constexpr meta::enable_if_t<
    meta::is_enum<EnumT>::value && !Enum::has_compound_divide_underlying_enum<void, EnumT>::value,
    meta::underlying_type_t<EnumT>&>
    operator/=(meta::underlying_type_t<EnumT>& lhs, EnumT rhs) noexcept
{
    lhs = lhs / Enum::as_value(rhs);
    return lhs;
}

// Compound assignment modulo operator for UnderlyingType, EnumT.
template <typename EnumT>
constexpr meta::enable_if_t<
    meta::is_enum<EnumT>::value && !Enum::has_compound_modulo_underlying_enum<void, EnumT>::value,
    meta::underlying_type_t<EnumT>&>
    operator%=(meta::underlying_type_t<EnumT>& lhs, EnumT rhs) noexcept
{
    lhs = lhs % Enum::as_value(rhs);
    return lhs;
}