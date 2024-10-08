#pragma once

#include <bx/containers/list.hpp>
#include <bx/meta/any.hpp>
#include <bx/meta/meta.hpp>

#include <utility>
#include <type_traits>
#include <iostream>

//#define OLD_METAFN
#ifndef OLD_METAFN

struct TTag { constexpr TTag(u32 v) : value(v) {} u32 value = 0; };
struct TName { constexpr TName(const char* v) : value(v) {} const char* value = nullptr; };
struct TDescription { constexpr TDescription(const char* v) : value(v) {} const char* value = nullptr; };
struct TCategory { constexpr TCategory(const char* v) : value(v) {} const char* value = nullptr; };

namespace Tag
{
	static constexpr TTag None{ 0 };
	static constexpr TTag ReadOnly{ 1 };
	static constexpr TTag Scriptable{ 2 };
	static constexpr TTag All{ ReadOnly.value | Scriptable.value };
}

#define FUNCTION(FuncT, ...) static const FuncMeta sReflect##FuncT = FuncMeta::Create(__VA_ARGS__);

/*struct FuncMeta
{
	template<typename... Args>
	constexpr FuncMeta(Args&&... args)
	{
		AddAll(std::forward<Args>(args)...);
	}

	inline void AddArg(const TTag& tag)
	{
		mTag = tag;
	}
	
	//inline void AddArg(const TName& name)
	//{
	//	mName = name;
	//}
	//
	//inline void AddArg(const TDescription& desc)
	//{
	//	mDescription = desc;
	//}
	//
	//inline void AddArg(const TCategory& cat)
	//{
	//	mCategory = cat;
	//}
	
	template<typename... Args>
	void AddAll(Args&&... args)
	{
		(AddArg(args), ...);
	}

	TTag mTag{ 0 };
	//TName mName{ "" };
	//TDescription mDescription{ "" };
	//TCategory mCategory{ "" };
};*/

template <typename TFunc, TFunc Func>
struct Function;

template<typename R, typename C, typename... Args, R(C::* F)(Args...)>
struct Function<R(C::*)(Args...), F>
{
	constexpr Function()
	{
		//C::GetStorage().emplace_back(F);
	}
};

struct Foo
{
	//FUNCTION(Func, Tag::ReadOnly);
	void Func();
	static constexpr Function<decltype(&Func), &Func> sReflectFunc{};
	//void Func();
};

/*class MetaFunc
{
public:
	using DynamicArgs = List<Any>;
	using DynamicInvoke = std::function<Any(DynamicArgs)>;

	using Param = TypeId;
	using Params = std::vector<TypeId>;
	using Return = TypeId;

	MetaFunc(const DynamicInvoke& invoke,
		//NameOrTypeInit nameOrType, // If an operator or named method
		const Return& returnType,
		const Params& parameters);

private:
	MetaFunc(DynamicInvoke&& invoke,
		//NameOrTypeInit nameOrType,
		Params&& paramsAndReturnAtBack,
		u32 funcId);

	template<typename Ret, typename... ParamsT, typename... ParamAndRetNames>
	MetaFunc(std::function<Ret(ParamsT...)>&& func,
		//NameOrTypeInit typeOrName,
		ParamAndRetNames&&... paramAndRetNames);

	// Mutable member function
	template<typename Ret, typename Obj, typename... ParamsT, typename... ParamAndRetNames>
	MetaFunc(Ret(Obj::* func)(ParamsT...),
		//const NameOrTypeInit typeOrName,
		ParamAndRetNames&&... paramAndRetNames);

	// Const member function
	template<typename Ret, typename Obj, typename... ParamsT, typename... ParamAndRetNames>
	MetaFunc(Ret(Obj::* func)(ParamsT...) const,
		//const NameOrTypeInit typeOrName,
		ParamAndRetNames&&... paramAndRetNames);

	// Static function
	template<typename Ret, typename... ParamsT, typename... ParamAndRetNames>
	MetaFunc(Ret(*func)(ParamsT...),
		//const NameOrTypeInit typeOrName,
		ParamAndRetNames&&... paramAndRetNames);

	// Functors without arguments
	//template<typename T, typename... ParamAndRetNames, std::enable_if_t<std::is_invocable_v<T>, bool> = true>
	//MetaFunc(const T& functor,
	//	const NameOrTypeInit typeOrName,\
	//  ParamAndRetNames&&... paramAndRetNames);

	template<typename... T>
	struct ExplicitParams
	{
	};

	// Functors with arguments.		
	template<typename T, typename... ParamsT, typename... ParamAndRetNames>
	MetaFunc(const T& functor,
		const NameOrTypeInit typeOrName, const ExplicitParams<ParamsT...>,
		ParamAndRetNames&&... paramAndRetNames);

	MetaFunc(MetaFunc&& other) noexcept;
	MetaFunc(const MetaFunc&) = delete;

	~MetaFunc();

public:
	template<typename... Arg>
	Any operator()(Arg&&... args) const;
};*/

#else
#include <bx/containers/list.hpp>
#include <bx/meta/any.hpp>
#include <bx/meta/meta.hpp>

#include <utility>
#include <type_traits>
#include <iostream>

#define declfunc(F) decltype(F), F

template<typename Signature, Signature>
struct Function;

// Specialization for free functions
template<typename R, typename... Args, R(*F)(Args...)>
struct Function<R(*)(Args...), F>
{
	using ClassType = void; // Free functions have no class type
	using ReturnType = R;

	static constexpr SizeType ArgCount = sizeof...(Args);

	static List<TypeId> GetArgTypeIds()
	{
		return { Type<Args>::Id()... };
	}

	// Function to be called with strongly-typed arguments
	static std::conditional_t<std::is_void_v<R>, void, R> Call(Args... args)
	{
		if (!std::is_void<R>::value) {}
		return F(std::forward<Args>(args)...);
	}

	// Function to be called with List<void*> (dynamically casting arguments)
	static std::conditional_t<std::is_void_v<R>, void, R> Invoke(List<void*>& args)
	{
		return InvokeWithArgs(args, std::index_sequence_for<Args...>{});
	}

	// Enable this function when R is not void
	//template <typename ReturnType = R, typename = std::enable_if_t<!std::is_void_v<ReturnType>>>
	static meta::any DynamicInvoke(void* /* unused */, List<void*>& args)
	{
		return Invoke(args);
	}

	// Enable this function when R is void
	//template <typename ReturnType = R, typename = std::enable_if_t<std::is_void_v<ReturnType>>>
	//static meta::any Invoke(void* /* unused */, List<void*>& args)
	//{
	//	Invoke(args);
	//	return {};  // Return an empty return for void functions
	//}

private:
	// Helper function to unpack arguments from vector, cast them, and call the function
	template <std::size_t... I>
	static std::conditional_t<std::is_void_v<R>, void, R> InvokeWithArgs(List<void*>& args, std::index_sequence<I...>)
	{
		if (args.size() != ArgCount)
			throw std::runtime_error("Argument count mismatch");

		return Call(*reinterpret_cast<Args*>(args[I])...);
	}
};

// Specialization for non-const member functions
template<typename R, typename C, typename... Args, R(C::*F)(Args...)>
struct Function<R(C::*)(Args...), F>
{
public:
	using ClassType = C;
	using ReturnType = R;
	static constexpr SizeType ArgCount = sizeof...(Args);

	static List<TypeId> GetArgTypeIds()
	{
		return { Type<Args>::Id()... };
	}

	// Function to be called with strongly-typed arguments (requires instance)
	static std::conditional_t<std::is_void_v<R>, void, R> Call(C* obj, Args... args)
	{
		return (obj->*F)(std::forward<Args>(args)...);
	}

	// Function to be called with List<void*> (dynamically casting arguments)
	static std::conditional_t<std::is_void_v<R>, void, R> Invoke(C* obj, List<void*>& args)
	{
		return InvokeWithArgs(obj, args, std::index_sequence_for<Args...>{});
	}

	static meta::any DynamicInvoke(void* obj, List<void*>& args)
	{
		// Use the correct member function specialization to invoke the function
		return Invoke(reinterpret_cast<C*>(obj), args);
	}

private:
	// Helper function to unpack arguments from vector, cast them, and call the function
	template <std::size_t... I>
	static std::conditional_t<std::is_void_v<R>, void, R> InvokeWithArgs(C* obj, List<void*>& args, std::index_sequence<I...>)
	{
		if (args.size() != ArgCount)
			throw std::runtime_error("Argument count mismatch");
		
		return Call(obj, *reinterpret_cast<Args*>(args[I])...);
	}
};

// Specialization for const member functions
template<typename R, typename C, typename... Args, R(C::*F)(Args...) const>
struct Function<R(C::*)(Args...) const, F>
{
public:
	using ClassType = const C;
	using ReturnType = R;
	static constexpr SizeType ArgCount = sizeof...(Args);

	static List<TypeId> GetArgTypeIds()
	{
		return { Type<Args>::Id()... };
	}

	// Function to be called with strongly-typed arguments (requires const instance)
	static std::conditional_t<std::is_void_v<R>, void, R> Call(const C* obj, Args... args)
	{
		return (obj->*F)(std::forward<Args>(args)...);
	}

	// Function to be called with List<void*> (dynamically casting arguments)
	static std::conditional_t<std::is_void_v<R>, void, R> Invoke(const C* obj, List<void*>& args)
	{
		return InvokeWithArgs(obj, args, std::index_sequence_for<Args...>{});
	}

	static meta::any DynamicInvoke(void* obj, List<void*>& args)
	{
		// Use the correct const member function specialization to invoke the function
		return Invoke(reinterpret_cast<const C*>(obj), args);
	}

private:
	// Helper function to unpack arguments from vector, cast them, and call the function
	template <std::size_t... I>
	static std::conditional_t<std::is_void_v<R>, void, R> InvokeWithArgs(const C* obj, List<void*>& args, std::index_sequence<I...>)
	{
		if (args.size() != ArgCount)
			throw std::runtime_error("Argument count mismatch");

		return Call(obj, *reinterpret_cast<Args*>(args[I])...);
	}
};

class MetaFunc
{
public:
	template <typename TFunc, TFunc Func>
	static MetaFunc Create(const String& name)
	{
		using FunctionType = Function<TFunc, Func>;

		MetaFunc metaFunc;

		// Set the class type ID (if applicable)
		if (!std::is_void<typename FunctionType::ClassType>::value)
		{
			metaFunc.classTypeId = Type<typename FunctionType::ClassType>::Id();
		}

		// Set return type ID
		metaFunc.returnTypeId = Type<typename FunctionType::ReturnType>::Id();

		// Set argument count and argument type IDs
		metaFunc.argCount = FunctionType::ArgCount;
		metaFunc.argTypeIds = FunctionType::GetArgTypeIds();

		auto func = &FunctionType::Call;
		metaFunc.strongInvoke = &FunctionType::Call;
		metaFunc.dynamicInvoke = FunctionType::DynamicInvoke;

		return metaFunc;
	}

	template <typename R, typename... Args>
	inline R StrongInvoke(Args... args)
	{
		auto func = meta::any_cast<R(*)(Args...)>(strongInvoke);
		return func(std::forward<Args>(args)...);
	}

	template <typename R, typename C, typename... Args>
	inline R StrongInvoke(C* obj, Args... args)
	{
		auto func = meta::any_cast<R(*)(C*, Args...)>(strongInvoke);
		return func(obj, std::forward<Args>(args)...);
	}

	template <typename R, typename C, typename... Args>
	inline R StrongInvoke(const C* obj, Args... args)
	{
		auto func = meta::any_cast<R(*)(const C*, Args...)>(strongInvoke);
		return func(obj, std::forward<Args>(args)...);
	}

	template <typename R>
	inline R DynamicInvoke(void* obj, List<void*>& args)
	{
		auto ret = dynamicInvoke(obj, args);
		return meta::any_cast<R>(ret);
	}

public:
	TypeId classTypeId{ 0 };
	TypeId returnTypeId{ 0 };
	SizeType argCount{ 0 };
	List<TypeId> argTypeIds;

	meta::any strongInvoke;
	std::function<meta::any(void*, List<void*>&)> dynamicInvoke;
};
#endif