#pragma once

#include "bx/engine/core/meta.hpp"
#include "bx/engine/core/guard.hpp"
#include "bx/engine/core/type.hpp"
#include "bx/engine/core/ecs.hpp"
#include "bx/engine/core/resource.hpp"
#include "bx/engine/containers/string.hpp"
#include "bx/engine/containers/array.hpp"
#include "bx/engine/containers/list.hpp"

using BindApiFn = void(*)();

typedef struct WrenVM WrenVM;
typedef struct WrenHandle WrenHandle;
typedef void (*WrenForeignMethodFn)(WrenVM* vm);
typedef void (*WrenFinalizerFn)(void* data);

struct ScriptClassInfo
{
	const char* moduleName = nullptr;
	const char* className = nullptr;
	TypeId typeId = INVALID_TYPEID;
	WrenHandle* handle = nullptr;
};

//struct ResourceClassWrapper
//{
//	using CreateFn = void(*)(WrenVM*);
//	using GetDataFn = void(*)(WrenVM*, MetaResource);
//
//	CreateFn createFn = nullptr;
//	GetDataFn getDataFn = nullptr;
//};

struct ComponentClassWrapper
{
	using HasComponentFn = void(*)(WrenVM*, Entity);
	using AddComponentFn = void(*)(WrenVM*, Entity);
	using GetComponentFn = void(*)(WrenVM*, Entity);
	using RemoveComponentFn = void(*)(WrenVM*, Entity);

	HasComponentFn hasComponentFn = nullptr;
	AddComponentFn addComponentFn = nullptr;
	GetComponentFn getComponentFn = nullptr;
	RemoveComponentFn removeComponentFn = nullptr;
};

class Script
{
public:
	static void RegisterApi(BindApiFn bindApi);

	static bool HasError();
	static void ClearError();

	static void BeginModule(const char* moduleName);
	static void EndModule();

	template <typename T, typename... Args>
	static void BeginClass(const char* className);

	template <typename T>
	static void BeginClass(const char* className, WrenForeignMethodFn allocate, WrenFinalizerFn finalize);

	static void BeginClass(const char* className);

	template <typename TData>
	static void BeginResourceClass(const char* className);

	template <typename TCmp>
	static void BeginComponentClass(const char* className);

	static void EndClass();

	template<typename F, F f>
	static void BindFunction(bool isStatic, const char* signature);

	static void BindCFunction(bool isStatic, const char* signature, WrenForeignMethodFn fn);

	template<typename T, typename U, U T::* Field>
	static void BindGetter(const char* signature);

	template<typename T, typename U, U T::* Field>
	static void BindSetter(const char* signature);

	template <typename T, typename U, const U Val>
	static void BindEnumVal(const char* signature);

private:
	friend class Runtime;
	friend class Module;

	static bool Initialize();
	static void Reload();
	static void Shutdown();

	static void Update();
	static void Render();

	static void CollectGarbage();

	template<typename T>
	friend struct ScriptArg;

	friend class ScriptInvokeImpl;

	static void RegisterClass(TypeId typeId);
	//static void RegisterResourceClass(TypeId typeId, const ResourceClassWrapper& wrapper);
	static void RegisterComponentClass(TypeId typeId, const ComponentClassWrapper& wrapper);
	static const ScriptClassInfo& GetClassInfo(TypeId typeId);

	static void SetClass(WrenVM* vm, i32 slot, TypeId typeId);
	
	template<typename T>
	static void SetClass(WrenVM* vm, i32 slot)
	{
		SetClass(vm, slot, Type<T>::Id());
	}

	static void RegisterConstructor(std::size_t arity, const char* signature, WrenForeignMethodFn func);
	static void RegisterDestructor(WrenFinalizerFn func);
	static void RegisterFunction(bool isStatic, const char* signature, WrenForeignMethodFn func);

	template<typename T, typename... Args, std::size_t... index>
	static void Construct(WrenVM* vm, void* memory, meta::index_sequence<index...>);

	template<typename T, typename... Args>
	static void Allocate(WrenVM* vm);

	template<typename T>
	static void Finalize(void* data);

	static void EnsureSlots(WrenVM* vm, i32 numSlots);
};

// Script API inspired by: https://github.com/Nelarius/wrenpp

template<typename T>
struct ScriptArg;

template<>
struct ScriptArg<bool>
{
	static bool Get(WrenVM* vm, i32 slot);
	static void Set(WrenVM* vm, i32 slot, bool val);
};

template<> struct ScriptArg<bool&> { static void Set(WrenVM* vm, i32 slot, bool val) { ScriptArg<bool>::Set(vm, slot, val); } };
template<> struct ScriptArg<const bool&> { static void Set(WrenVM* vm, i32 slot, bool val) { ScriptArg<bool>::Set(vm, slot, val); } };

template<>
struct ScriptArg<i32>
{
	static i32 Get(WrenVM* vm, i32 slot);
	static void Set(WrenVM* vm, i32 slot, i32 val);
};

template<> struct ScriptArg<i32&> { static void Set(WrenVM* vm, i32 slot, i32 val) { ScriptArg<i32>::Set(vm, slot, val); } };
template<> struct ScriptArg<const i32&> { static void Set(WrenVM* vm, i32 slot, i32 val) { ScriptArg<i32>::Set(vm, slot, val); } };

template<>
struct ScriptArg<u32>
{
	static u32 Get(WrenVM* vm, i32 slot);
	static void Set(WrenVM* vm, i32 slot, u32 val);
};

template<> struct ScriptArg<u32&> { static void Set(WrenVM* vm, i32 slot, u32 val) { ScriptArg<u32>::Set(vm, slot, val); } };
template<> struct ScriptArg<const u32&> { static void Set(WrenVM* vm, i32 slot, u32 val) { ScriptArg<u32>::Set(vm, slot, val); } };

template<>
struct ScriptArg<u64>
{
	static u64 Get(WrenVM* vm, i32 slot);
	static void Set(WrenVM* vm, i32 slot, u64 val);
};

template<> struct ScriptArg<u64&> { static void Set(WrenVM* vm, i32 slot, u64 val) { ScriptArg<u64>::Set(vm, slot, val); } };
template<> struct ScriptArg<const u64&> { static void Set(WrenVM* vm, i32 slot, u64 val) { ScriptArg<u64>::Set(vm, slot, val); } };

template<>
struct ScriptArg<f32>
{
	static f32 Get(WrenVM* vm, i32 slot);
	static void Set(WrenVM* vm, i32 slot, f32 val);
};

template<> struct ScriptArg<f32&> { static void Set(WrenVM* vm, i32 slot, f32 val) { ScriptArg<f32>::Set(vm, slot, val); } };
template<> struct ScriptArg<const f32&> { static void Set(WrenVM* vm, i32 slot, f32 val) { ScriptArg<f32>::Set(vm, slot, val); } };

template<>
struct ScriptArg<f64>
{
	static f64 Get(WrenVM* vm, i32 slot);
	static void Set(WrenVM* vm, i32 slot, f64 val);
};

template<> struct ScriptArg<f64&> { static void Set(WrenVM* vm, i32 slot, f64 val) { ScriptArg<f64>::Set(vm, slot, val); } };
template<> struct ScriptArg<const f64&> { static void Set(WrenVM* vm, i32 slot, f64 val) { ScriptArg<f64>::Set(vm, slot, val); } };

template<>
struct ScriptArg<const char*>
{
	static const char* Get(WrenVM* vm, i32 slot);
	static void Set(WrenVM* vm, i32 slot, const char* val);
};

template<>
struct ScriptArg<String>
{
	static String Get(WrenVM* vm, i32 slot);
	static void Set(WrenVM* vm, i32 slot, const String& val);
};

template<>
struct ScriptArg<String&>
{
	static String Get(WrenVM* vm, i32 slot);
	static void Set(WrenVM* vm, i32 slot, const String& val);
};

template<>
struct ScriptArg<const String&>
{
	static String Get(WrenVM* vm, i32 slot);
	static void Set(WrenVM* vm, i32 slot, const String& val);
};

template<>
struct ScriptArg<void*>
{
	static void* Get(WrenVM* vm, i32 slot);
	static void* Set(WrenVM* vm, i32 slot, i32 classSlot, std::size_t size);

	static void SetList(WrenVM* vm, i32 slot);

	static i32 GetListCount(WrenVM* vm, i32 slot);
	static void GetListElement(WrenVM* vm, i32 listSlot, i32 index, i32 elementSlot);
	static void SetListElement(WrenVM* vm, i32 listSlot, i32 index, i32 elementSlot);

	static void InsertInList(WrenVM* vm, i32 listSlot, i32 index, i32 elementSlot);
};

struct ScriptObj
{
	virtual ~ScriptObj() = default;
	virtual void* Ptr() = 0;
};

// Lives within wren VM
template<typename T>
struct ScriptObjVal : public ScriptObj
{
	explicit ScriptObjVal() : obj() {}
	virtual ~ScriptObjVal() { static_cast<T*>(Ptr())->~T(); }
	void* Ptr() override { return &obj; }
	typename std::aligned_storage<sizeof(T), alignof(T)>::type obj;
};

// Lives within host
template<typename T>
struct ScriptObjPtr : public ScriptObj
{
	explicit ScriptObjPtr(T* obj) : obj{ obj } {}
	virtual ~ScriptObjPtr() = default;
	void* Ptr() override { return obj; }
	T* obj;
};

template<typename T>
struct ScriptArg
{
	static T Get(WrenVM* vm, i32 slot)
	{
		ScriptObj* obj = static_cast<ScriptObj*>(ScriptArg<void*>::Get(vm, slot));
		return *static_cast<T*>(obj->Ptr());
	}

	static void Set(WrenVM* vm, i32 slot, T val)
	{
		Script::SetClass<T>(vm, slot);
		ScriptObj* obj = new (ScriptArg<void*>::Set(vm, slot, slot, sizeof(ScriptObjVal<T>))) ScriptObjVal<T>();
		new (obj->Ptr()) T(val);
	}
};

template<typename T>
struct ScriptArg<T&>
{
	static T& Get(WrenVM* vm, i32 slot)
	{
		ScriptObj* obj = static_cast<ScriptObj*>(ScriptArg<void*>::Get(vm, slot));
		return *static_cast<T*>(obj->Ptr());
	}

	static void Set(WrenVM* vm, i32 slot, T& val)
	{
		Script::SetClass<T>(vm, slot);
		new (ScriptArg<void*>::Set(vm, slot, slot, sizeof(ScriptObjPtr<T>))) ScriptObjPtr<T>(&val);
	}
};

template<typename T>
struct ScriptArg<const T&>
{
	static const T& Get(WrenVM* vm, i32 slot)
	{
		ScriptObj* obj = static_cast<ScriptObj*>(ScriptArg<void*>::Get(vm, slot));
		return *static_cast<T*>(obj->Ptr());
	}

	static void Set(WrenVM* vm, i32 slot, const T& val)
	{
		Script::SetClass<T>(vm, slot);
		new (ScriptArg<void*>::Set(vm, slot, slot, sizeof(ScriptObjPtr<T>))) ScriptObjPtr<T>(const_cast<T*>(&val));
	}
};

template<typename T>
struct ScriptArg<T*>
{
	static T* Get(WrenVM* vm, i32 slot)
	{
		ScriptObj* obj = static_cast<ScriptObj*>(ScriptArg<void*>::Get(vm, slot));
		return static_cast<T*>(obj->Ptr());
	}

	static void Set(WrenVM* vm, i32 slot, T* val)
	{
		Script::SetClass<T>(vm, slot);
		new (ScriptArg<void*>::Set(vm, slot, slot, sizeof(ScriptObjPtr<T>))) ScriptObjPtr<T>(val);
	}
};

template<typename T>
struct ScriptArg<const T*>
{
	static const T* Get(WrenVM* vm, i32 slot)
	{
		ScriptObj* obj = static_cast<ScriptObj*>(ScriptArg<void*>::Get(vm, slot));
		return static_cast<T*>(obj->Ptr());
	}

	static void Set(WrenVM* vm, i32 slot, const T* val)
	{
		Script::SetClass<T>(vm, slot);
		new (ScriptArg<void*>::Set(vm, slot, slot, sizeof(ScriptObjPtr<T>))) ScriptObjPtr<T>(const_cast<T*>(val));
	}
};

template<class T, std::size_t Size>
struct ScriptArgArray
{
	static Array<T, Size> Get(WrenVM* vm, i32 slot)
	{
		Array<T, Size> val;
		i32 count = ScriptArg<void*>::GetListCount(vm, slot);
		BX_ASSERT(Size == count, "List does not match size of array!");
		for (i32 i = 0; i < count; i++)
		{
			ScriptArg<void*>::GetListElement(vm, slot, i, 0);
			val[i] = ScriptArg<T>::Get(vm, 0);
		}
		return val;
	}

	//static void Set(WrenVM* vm, i32 slot, const Array<f32, Size>& val)
	//{
	//	new (ScriptArg<void*>::Set(vm, slot, slot, sizeof(ScriptObjPtr<T>))) ScriptObjPtr<T>(val);
	//}
};

template<class T, std::size_t Size>
struct ScriptArg<Array<T, Size>>
{
	static Array<T, Size> Get(WrenVM* vm, i32 slot) { return ScriptArgArray<T, Size>::Get(vm, slot); }
};

template<class T, std::size_t Size>
struct ScriptArg<Array<T, Size>&>
{
	static Array<T, Size> Get(WrenVM* vm, i32 slot) { return ScriptArgArray<T, Size>::Get(vm, slot); }
};

template <class T, std::size_t Size>
struct ScriptArg<const Array<T, Size>&>
{
	static Array<T, Size> Get(WrenVM* vm, i32 slot) { return ScriptArgArray<T, Size>::Get(vm, slot); }
};

template<class T>
struct ScriptArgList
{
	static List<T> Get(WrenVM* vm, i32 slot)
	{
		List<T> val;
		i32 count = ScriptArg<void*>::GetListCount(vm, slot);
		val.resize(count);
		for (i32 i = 0; i < count; i++)
		{
			ScriptArg<void*>::GetListElement(vm, slot, i, 0);
			val[i] = ScriptArg<T>::Get(vm, 0);
		}
		return val;
	}

	static void Set(WrenVM* vm, i32 slot, const List<T>& val)
	{
		ScriptArg<void*>::SetList(vm, slot);
		for (const auto& v : val)
		{
			ScriptArg<T>::Set(vm, 1, v);
			ScriptArg<void*>::InsertInList(vm, 0, -1, 1);
		}
	}
};

template<class T>
struct ScriptArg<List<T>>
{
	static List<T> Get(WrenVM* vm, i32 slot) { return ScriptArgList<T>::Get(vm, slot); }
	static void Set(WrenVM* vm, i32 slot, const List<T>& val) { return ScriptArgList<T>::Set(vm, slot, val); }
};

template<class T>
struct ScriptArg<List<T>&>
{
	static List<T> Get(WrenVM* vm, i32 slot) { return ScriptArgList<T>::Get(vm, slot); }
	static void Set(WrenVM* vm, i32 slot, const List<T>& val) { return ScriptArgList<T>::Set(vm, slot, val); }
};

template <class T>
struct ScriptArg<const List<T>&>
{
	static List<T> Get(WrenVM* vm, i32 slot) { return ScriptArgList<T>::Get(vm, slot); }
	static void Set(WrenVM* vm, i32 slot, const List<T>& val) { return ScriptArgList<T>::Set(vm, slot, val); }
};

//template <typename T>
//struct ScriptArg<Resource<T>>
//{
//	static Resource<T> Get(WrenVM* vm, i32 slot)
//	{
//		ScriptObj* obj = static_cast<ScriptObj*>(ScriptArg<void*>::Get(vm, slot));
//		return *static_cast<Resource<T>*>(obj->Ptr());
//	}
//
//	template <typename U>
//	static void Set(WrenVM* vm, i32 slot, U val)
//	{
//		Script::SetClass<MetaResource>(vm, slot);
//		ScriptObj* obj = new (ScriptArg<void*>::Set(vm, slot, slot, sizeof(ScriptObjVal<Resource<T>>))) ScriptObjVal<Resource<T>>();
//		new (obj->Ptr()) Resource<T>(val);
//	}
//};
//
//template <typename T>
//struct ScriptArg<const Resource<T>>
//{
//	static Resource<T> Get(WrenVM* vm, i32 slot)
//	{
//		ScriptObj* obj = static_cast<ScriptObj*>(ScriptArg<void*>::Get(vm, slot));
//		return *static_cast<Resource<T>*>(obj->Ptr());
//	}
//
//	template <typename U>
//	static void Set(WrenVM* vm, i32 slot, U val)
//	{
//		Script::SetClass<MetaResource>(vm, slot);
//		ScriptObj* obj = new (ScriptArg<void*>::Set(vm, slot, slot, sizeof(ScriptObjVal<Resource<T>>))) ScriptObjVal<Resource<T>>();
//		new (obj->Ptr()) Resource<T>(val);
//	}
//};

class ScriptInvokeImpl
{
private:
	template<bool RetVoid>
	friend class ScriptInvoke;

	// function pointer
	template<typename R, typename... Args>
	static R CallWithArguments(WrenVM* vm, R(*f)(Args...))
	{
		constexpr std::size_t arity = meta::function_traits<decltype(f)>::arity;
		Script::EnsureSlots(vm, arity);
		return CallImpl(vm, f, meta::make_index_sequence<arity>{});
	}

	// member function pointer
	template<typename R, typename C, typename... Args>
	static R CallWithArguments(WrenVM* vm, R(C::* f)(Args...))
	{
		constexpr std::size_t arity = meta::function_traits<decltype(f)>::arity;
		Script::EnsureSlots(vm, arity);
		return CallImpl(vm, f, meta::make_index_sequence<arity>{});
	}

	// const member function pointer
	template<typename R, typename C, typename... Args>
	static R CallWithArguments(WrenVM* vm, R(C::* f)(Args...) const)
	{
		constexpr std::size_t arity = meta::function_traits<decltype(f)>::arity;
		Script::EnsureSlots(vm, arity);
		return CallImpl(vm, f, meta::make_index_sequence<arity>{});
	}

	// function pointer
	template<typename R, typename... Args, std::size_t... index>
	static R CallImpl(WrenVM* vm, R(*f)(Args...), meta::index_sequence<index...>)
	{
		using Traits = meta::function_traits<meta::remove_reference_t<decltype(f)>>;
		return f(ScriptArg<typename Traits::template argument_t<index>>::Get(vm, index + 1)...);
	}

	// member function pointer
	template<typename R, typename C, typename... Args, std::size_t... index>
	static R CallImpl(WrenVM* vm, R(C::* f)(Args...), meta::index_sequence<index...>)
	{
		using Traits = meta::function_traits<decltype(f)>;
		ScriptObj* obj = static_cast<ScriptObj*>(ScriptArg<void*>::Get(vm, 0));
		C* ptr = static_cast<C*>(obj->Ptr());
		return (ptr->*f)(ScriptArg<typename Traits::template argument_t<index>>::Get(vm, index + 1)...);
	}

	// const member function pointer
	template<typename R, typename C, typename... Args, std::size_t... index>
	static R CallImpl(WrenVM* vm, R(C::* f)(Args...) const, meta::index_sequence<index...>)
	{
		using Traits = meta::function_traits<decltype(f)>;
		ScriptObj* obj = static_cast<ScriptObj*>(ScriptArg<void*>::Get(vm, 0));
		C* ptr = static_cast<C*>(obj->Ptr());
		return (ptr->*f)(ScriptArg<typename Traits::template argument_t<index>>::Get(vm, index + 1)...);
	}
};

template<bool RetVoid>
class ScriptInvoke;

template<>
class ScriptInvoke<true>
{
private:
	template<typename Signature, Signature>
	friend class ScriptInvokeWrapper;

	// function pointer
	template<typename R, typename... Args>
	static void Call(WrenVM* vm, R(*f)(Args...))
	{
		ScriptInvokeImpl::CallWithArguments(vm, std::forward<R(*)(Args...)>(f));
	}

	// member function pointer
	template<typename R, typename C, typename... Args>
	static void Call(WrenVM* vm, R(C::* f)(Args...))
	{
		ScriptInvokeImpl::CallWithArguments(vm, std::forward<R(C::*)(Args...)>(f));
	}

	// const member function pointer
	template<typename R, typename C, typename... Args>
	static void Call(WrenVM* vm, R(C::* f)(Args...) const)
	{
		ScriptInvokeImpl::CallWithArguments(vm, std::forward<R(C::*)(Args...) const>(f));
	}
};

template<>
class ScriptInvoke<false>
{
private:
	template<typename Signature, Signature>
	friend class ScriptInvokeWrapper;

	// function pointer
	template<typename R, typename... Args>
	static void Call(WrenVM* vm, R(*f)(Args...))
	{
		using ReturnType = typename meta::function_traits<meta::remove_reference_t<decltype(f)>>::return_type;
		ScriptArg<ReturnType>::Set(vm, 0, ScriptInvokeImpl::CallWithArguments(vm, std::forward<R(*)(Args...)>(f)));
	}

	// member function pointer
	template<typename R, typename C, typename... Args>
	static void Call(WrenVM* vm, R(C::* f)(Args...))
	{
		using ReturnType = typename meta::function_traits<meta::remove_reference_t<decltype(f)>>::return_type;
		ScriptArg<ReturnType>::Set(vm, 0, ScriptInvokeImpl::CallWithArguments(vm, std::forward<R(C::*)(Args...)>(f)));
	}

	// const member function pointer
	template<typename R, typename C, typename... Args>
	static void Call(WrenVM* vm, R(C::* f)(Args...) const)
	{
		using ReturnType = typename meta::function_traits<meta::remove_reference_t<decltype(f)>>::return_type;
		ScriptArg<ReturnType>::Set(vm, 0, ScriptInvokeImpl::CallWithArguments(vm, std::forward<R(C::*)(Args...) const>(f)));
	}
};

template<typename Signature, Signature>
class ScriptInvokeWrapper;

template<typename R, typename... Args, R(*f)(Args...)>
class ScriptInvokeWrapper<R(*)(Args...), f>
{
private:
	friend class Script;

	static void Call(WrenVM* vm)
	{
		ScriptInvoke<std::is_void<R>::value>::Call(vm, f);
	}
};

template<typename R, typename C, typename... Args, R(C::* f)(Args...)>
class ScriptInvokeWrapper<R(C::*)(Args...), f>
{
private:
	friend class Script;

	static void Call(WrenVM* vm)
	{
		ScriptInvoke<std::is_void<R>::value>::Call(vm, f);
	}
};

template<typename R, typename C, typename... Args, R(C::* f)(Args...) const>
class ScriptInvokeWrapper<R(C::*)(Args...) const, f>
{
private:
	friend class Script;

	static void Call(WrenVM* vm)
	{
		ScriptInvoke<std::is_void<R>::value>::Call(vm, f);
	}
};

template<typename T, typename U, U T::* Field>
class ScriptProperty
{
private:
	friend class Script;

	static void Get(WrenVM* vm)
	{
		T* obj = ScriptArg<T*>::Get(vm, 0);
		ScriptArg<U>::Set(vm, 0, obj->*Field);
	}

	static void Set(WrenVM* vm)
	{
		T* obj = ScriptArg<T*>::Get(vm, 0);
		obj->*Field = ScriptArg<U>::Get(vm, 1);
	}
};

template<typename T, typename U, const U Val>
class ScriptEnumVal
{
private:
	friend class Script;

	static void Get(WrenVM* vm)
	{
		ScriptArg<T>::Set(vm, 0, T(Val));
	}
};

template<typename T, typename... Args, std::size_t... index>
void Script::Construct(WrenVM* vm, void* memory, meta::index_sequence<index...>)
{
	using Traits = meta::parameter_pack_traits<Args...>;
	EnsureSlots(vm, Traits::count);
	ScriptObjVal<T>* obj = new (memory) ScriptObjVal<T>{};
	new (obj->Ptr()) T{ ScriptArg<typename Traits::template parameter_t<index>>::Get(vm, index + 1)... };
}

template<typename T, typename... Args>
void Script::Allocate(WrenVM* vm)
{
	void* memory = ScriptArg<void*>::Set(vm, 0, 0, sizeof(ScriptObjVal<T>));
	Construct<T, Args...>(vm, memory, meta::make_index_sequence<meta::parameter_pack_traits<Args...>::count>{});
}

template<typename T>
void Script::Finalize(void* data)
{
	ScriptObj* obj = static_cast<ScriptObj*>(data);
	obj->~ScriptObj();
}

template<typename T, typename... Args>
void Script::BeginClass(const char* className)
{
	BeginClass(className);
	RegisterClass(Type<T>::Id());

	WrenForeignMethodFn allocate = &Allocate<T, Args...>;
	RegisterConstructor(0, "", allocate);

	WrenFinalizerFn finalize = &Finalize<T>;
	RegisterDestructor(finalize);
}

template<typename T>
void Script::BeginClass(const char* className, WrenForeignMethodFn allocate, WrenFinalizerFn finalize)
{
	BeginClass(className);
	RegisterClass(Type<T>::Id());
	RegisterConstructor(0, "", allocate);
	RegisterDestructor(finalize);
}

//template <typename TData>
//void Script::BeginResourceClass(const char* className)
//{
//	Script::BeginClass<TData>(className);
//
//	ResourceClassWrapper wrapper;
//	wrapper.createFn = [](WrenVM* vm)
//	{
//		const auto& filename = ScriptArg<const std::string&>::Get(vm, 2);
//		auto res = MetaResource::Create<TData>(filename);
//		ScriptArg<MetaResource>::Set(vm, 0, res);
//	};
//
//	wrapper.getDataFn = [](WrenVM* vm, MetaResource res)
//	{
//		auto& data = res.GetData<TData>();
//		ScriptArg<TData&>::Set(vm, 0, data);
//	};
//
//	RegisterResourceClass(Type<TData>::Id(), wrapper);
//}

template <typename TCmp>
void Script::BeginComponentClass(const char* className)
{
	Script::BeginClass<TCmp>(className);

	ComponentClassWrapper wrapper;
	wrapper.hasComponentFn = [](WrenVM* vm, Entity e)
	{
		ScriptArg<bool>::Set(vm, 0, e.HasComponent<TCmp>());
	};
	wrapper.addComponentFn = [](WrenVM* vm, Entity e)
	{
		auto& cmp = e.AddComponent<TCmp>();
		ScriptArg<TCmp&>::Set(vm, 0, cmp);
	};
	wrapper.getComponentFn = [](WrenVM* vm, Entity e)
	{
		auto& cmp = e.GetComponent<TCmp>();
		ScriptArg<TCmp&>::Set(vm, 0, cmp);
	};
	wrapper.removeComponentFn = [](WrenVM* vm, Entity e)
	{
		e.RemoveComponent<TCmp>();
	};

	RegisterComponentClass(Type<TCmp>::Id(), wrapper);
}

template<typename F, F f>
void Script::BindFunction(bool isStatic, const char* signature)
{
	RegisterFunction(isStatic, signature, ScriptInvokeWrapper<decltype(f), f>::Call);
}

inline void Script::BindCFunction(bool isStatic, const char* signature, WrenForeignMethodFn fn)
{
	RegisterFunction(isStatic, signature, fn);
}

template<typename T, typename U, U T::* Field>
void Script::BindGetter(const char* signature)
{
	RegisterFunction(false, signature, ScriptProperty<T, U, Field>::Get);
}

template<typename T, typename U, U T::* Field>
void Script::BindSetter(const char* signature)
{
	RegisterFunction(false, signature, ScriptProperty<T, U, Field>::Set);
}

template <typename T, typename U, const U Val>
void Script::BindEnumVal(const char* signature)
{
	RegisterFunction(true, signature, ScriptEnumVal<T, U, Val>::Get);
}