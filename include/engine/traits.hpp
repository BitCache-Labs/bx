#pragma once

#include <engine/api.hpp>

#include <cstddef>
#include <type_traits>
#include <tuple>

// This namespace serves as a layer in-between std in order to implement missing >c++11 features
// Note that not everything is wrappable, so we'd still depend on std for those.
namespace meta
{
    using false_type = std::false_type;
    using true_type = std::true_type;

    // No wrapper possible?
    //template<typename T>
    //auto declval() noexcept -> decltype(std::declval<T>(0))
    //{
    //    return std::declval<T>(0);
    //}

    template <bool B, typename T = void>
    using enable_if = std::enable_if<B, T>;

    template <bool B, typename T = void>
    using enable_if_t = typename enable_if<B, T>::type;

    template <typename T>
    using is_enum = std::is_enum<T>;

    template <typename T, typename Enable = void>
    struct underlying_type;

    template <typename T>
    struct BX_API underlying_type<T, enable_if_t<is_enum<T>::value>>
    {
        using type = typename std::underlying_type<T>::type;
    };

    template <typename T>
    struct BX_API underlying_type<T, enable_if_t<!is_enum<T>::value>>
    {
        using type = void;
    };

    template <typename T>
    using underlying_type_t = typename underlying_type<T>::type;

	template<typename T, T... Ints>
	struct integer_sequence
	{
		typedef T value_type;
		static constexpr std::size_t size() { return sizeof...(Ints); }
	};

	template<std::size_t... Ints>
	using index_sequence = integer_sequence<std::size_t, Ints...>;

	template<typename T, std::size_t N, T... Is>
	struct make_integer_sequence : make_integer_sequence<T, N - 1, N - 1, Is...> {};

	template<typename T, T... Is>
	struct make_integer_sequence<T, 0, Is...> : integer_sequence<T, Is...> {};

	template<std::size_t N>
	using make_index_sequence = make_integer_sequence<std::size_t, N>;

	template<typename... T>
	using index_sequence_for = make_index_sequence<sizeof...(T)>;

	template<class T>
	using remove_reference_t = typename std::remove_reference<T>::type;

	template<std::size_t I, class T>
	using tuple_element_t = typename std::tuple_element<I, T>::type;

	template< class T >
	using decay_t = typename std::decay<T>::type;

	template<class F>
	struct function_traits;

	// function pointer
	template<class R, class... Args>
	struct function_traits<R(*)(Args...)> : public function_traits<R(Args...)> {};

	// member function pointer
	template<class C, class R, class... Args>
	struct function_traits<R(C::*)(Args...)> : public function_traits<R(Args...)> {};

	// const member function pointer
	template<class C, class R, class... Args>
	struct function_traits<R(C::*)(Args...) const> : public function_traits<R(Args...)> {};

	template<class R, class... Args>
	struct function_traits<R(Args...)>
	{
		using return_type = R;

		static constexpr std::size_t arity = sizeof...(Args);

		template <std::size_t N>
		struct argument
		{
			static_assert(N < arity, "Invalid parameter index.");
			using type = tuple_element_t<N, std::tuple<Args...>>;
		};

		template<std::size_t N>
		using argument_t = typename argument<N>::type;
	};

	template<typename... Args>
	struct parameter_pack_traits
	{
		constexpr static const std::size_t count = sizeof...(Args);

		template<std::size_t N>
		struct parameter
		{
			static_assert(N < count, "Invalid parameter index");
			using type = tuple_element_t<N, std::tuple<Args...>>;
		};

		template<std::size_t N>
		using parameter_t = typename parameter<N>::type;
	};

	template<typename T>
	struct identity { typedef T type; };

	//https://stackoverflow.com/questions/27501400/the-implementation-of-stdforward
	template<typename T>
	T&& forward(typename identity<T>::type&& param)
	{
		return static_cast<typename identity<T>::type&&>(param);
	}

	template<typename Dst>
	Dst implicit_cast(typename identity<Dst>::type t) { return t; }

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	//https://stackoverflow.com/questions/39816779/check-if-type-is-defined
	//template <typename T, typename D>
	//struct is_defined;
	//
	//template <class T>
	//struct is_defined<T, std::enable_if_t<std::is_object<T>::value && !std::is_pointer<T>::value && (sizeof(T) > 0)>> : std::true_type
	//{
	//public:
	//	using type = T;
	//};
}