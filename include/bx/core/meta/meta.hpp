#pragma once

// TODO: Re-organize this, it's meant to be a extension of any meta-programming missing from c++11
// Would be nice to not depend on the std library however.

#include <cstddef>
#include <type_traits>
#include <tuple>

namespace meta
{
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


	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////


	template<class T>
	using remove_reference_t = typename std::remove_reference<T>::type;

	template<std::size_t I, class T>
	using tuple_element_t = typename std::tuple_element<I, T>::type;

	template< class T >
	using decay_t = typename std::decay<T>::type;


	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////


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


	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////


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