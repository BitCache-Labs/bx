#pragma once

//#include <engine/meta.hpp>
#include <engine/guard.hpp>
#include <engine/type.hpp>
#include <engine/log.hpp>
//#include <engine/ecs.hpp>
//#include <engine/resource.hpp>
#include <engine/traits.hpp>
#include <engine/string.hpp>
#include <engine/array.hpp>
#include <engine/list.hpp>

LOG_CHANNEL(Script)

using ScriptHandle = u64;
constexpr ScriptHandle SCRIPT_INVALID_HANDLE = -1;

typedef void (*ScriptMethodFn)(ScriptHandle vm);
typedef void (*ScriptFinalizerFn)(void* data);

struct VmInfo {};

struct ScriptClassInfo
{
	StringView moduleName{};
	StringView className{};
	TypeId typeId{};
	void* impl{ nullptr };
};

//struct ResourceClassWrapper
//{
//	using CreateFn = void(*)(ScriptHandle);
//	using GetDataFn = void(*)(ScriptHandle, MetaResource);
//
//	CreateFn createFn = nullptr;
//	GetDataFn getDataFn = nullptr;
//};

//struct ComponentClassWrapper
//{
//	using HasComponentFn = void(*)(ScriptHandle, Entity);
//	using AddComponentFn = void(*)(ScriptHandle, Entity);
//	using GetComponentFn = void(*)(ScriptHandle, Entity);
//	using RemoveComponentFn = void(*)(ScriptHandle, Entity);
//
//	HasComponentFn hasComponentFn = nullptr;
//	AddComponentFn addComponentFn = nullptr;
//	GetComponentFn getComponentFn = nullptr;
//	RemoveComponentFn removeComponentFn = nullptr;
//};

class BX_API Script
{
	BX_MODULE_INTERFACE(Script)

public:
	virtual bool Initialize() = 0;
	virtual void Shutdown() = 0;

	virtual ScriptHandle CreateVm(const VmInfo& info) = 0;
	virtual void DestroyVm(ScriptHandle vm) = 0;

	virtual void ConfigureApi(ScriptHandle vm) = 0;

	virtual void CollectGarbage(ScriptHandle vm) = 0;

	virtual bool HasError(ScriptHandle vm) = 0;
	virtual void ClearError(ScriptHandle vm) = 0;

	virtual void BindVm(ScriptHandle vm) = 0;

	virtual void BeginModule(StringView moduleName) = 0;
	virtual void EndModule() = 0;

	virtual void BeginClass(StringView className) = 0;
	virtual void EndClass() = 0;

	virtual void BindCFunction(bool isStatic, StringView signature, ScriptMethodFn fn) = 0;

	virtual bool GetSlotBool(ScriptHandle vm, i32 slot) = 0;
	virtual double GetSlotDouble(ScriptHandle vm, i32 slot) = 0;
	virtual StringView GetSlotString(ScriptHandle vm, i32 slot) = 0;
	virtual void* GetSlotObject(ScriptHandle vm, i32 slot) = 0;

	virtual void SetSlotBool(ScriptHandle vm, i32 slot, bool value) = 0;
	virtual void SetSlotDouble(ScriptHandle vm, i32 slot, f64 value) = 0;
	virtual void SetSlotString(ScriptHandle vm, i32 slot, StringView text) = 0;
	virtual void* SetSlotNewObject(ScriptHandle vm, i32 slot, i32 classSlot, SizeType size) = 0;

public:
	template <typename T, typename... Args>
	void BeginClass(StringView className);

	template <typename T>
	void BeginClass(StringView className, ScriptMethodFn allocate, ScriptFinalizerFn finalize);

	//template <typename TData>
	//void BeginResourceClass(StringView className);
	//
	//template <typename TCmp>
	//void BeginComponentClass(StringView className);

	template<typename F, F f>
	void BindFunction(bool isStatic, StringView signature);

	template<typename T, typename U, U T::* Field>
	void BindGetter(StringView signature);

	template<typename T, typename U, U T::* Field>
	void BindSetter(StringView signature);

	template <typename T, typename U, const U Val>
	void BindEnumVal(StringView signature);

private:
	template<typename T>
	friend struct ScriptArg;

	friend class ScriptInvokeImpl;

	virtual void RegisterClass(TypeId typeId) = 0;
	//void RegisterResourceClass(TypeId typeId, const ResourceClassWrapper& wrapper);
	//void RegisterComponentClass(TypeId typeId, const ComponentClassWrapper& wrapper);
	//const ScriptClassInfo& GetClassInfo(TypeId typeId);

	virtual void SetClass(ScriptHandle vm, i32 slot, TypeId typeId) = 0;

	virtual void RegisterConstructor(SizeType arity, StringView signature, ScriptMethodFn func) = 0;
	virtual void RegisterDestructor(ScriptFinalizerFn func) = 0;
	virtual void RegisterFunction(bool isStatic, StringView signature, ScriptMethodFn func) = 0;

	virtual void EnsureSlots(ScriptHandle vm, i32 numSlots) = 0;

private:
	template<typename T>
	void SetClass(ScriptHandle vm, i32 slot)
	{
		SetClass(vm, slot, Type<T>::Id());
	}

	template<typename T, typename... Args, SizeType... index>
	void Construct(ScriptHandle vm, void* memory, meta::index_sequence<index...>);

	template<typename T, typename... Args>
	void Allocate(ScriptHandle vm);

	template<typename T>
	void Finalize(void* data);
};

// Script API inspired by: https://github.com/Nelarius/wrenpp

template<typename T>
struct ScriptArg;

template<>
struct ScriptArg<bool>
{
	static bool Get(ScriptHandle vm, i32 slot) { return Script::Get().GetSlotBool(vm, slot); }
	static void Set(ScriptHandle vm, i32 slot, bool val) { Script::Get().SetSlotBool(vm, slot, val); }
};

template<> struct ScriptArg<bool&> { static void Set(ScriptHandle vm, i32 slot, bool val) { ScriptArg<bool>::Set(vm, slot, val); } };
template<> struct ScriptArg<const bool&> { static void Set(ScriptHandle vm, i32 slot, bool val) { ScriptArg<bool>::Set(vm, slot, val); } };

template<>
struct ScriptArg<f64>
{
	static f64 Get(ScriptHandle vm, i32 slot) { return Script::Get().GetSlotDouble(vm, slot); }
	static void Set(ScriptHandle vm, i32 slot, f64 val) { Script::Get().SetSlotDouble(vm, slot, val); }
};

template<> struct ScriptArg<f64&> { static void Set(ScriptHandle vm, i32 slot, f64 val) { ScriptArg<f64>::Set(vm, slot, val); } };
template<> struct ScriptArg<const f64&> { static void Set(ScriptHandle vm, i32 slot, f64 val) { ScriptArg<f64>::Set(vm, slot, val); } };

#define SCRIPT_ARG_VARIANT(Base, Type) \
template<> struct ScriptArg<Type> \
{ \
	static Type Get(ScriptHandle vm, i32 slot) { return static_cast<Type>(ScriptArg<Base>::Get(vm, slot)); }	\
	static void Set(ScriptHandle vm, i32 slot, Type val) { ScriptArg<Base>::Set(vm, slot, static_cast<Type>(val)); }	\
}; \
template<> struct ScriptArg<Type&> { static void Set(ScriptHandle vm, i32 slot, Type val) { ScriptArg<Type>::Set(vm, slot, val); } }; \
template<> struct ScriptArg<const Type&> { static void Set(ScriptHandle vm, i32 slot, Type val) { ScriptArg<Type>::Set(vm, slot, val); } };

SCRIPT_ARG_VARIANT(f64, f32)
SCRIPT_ARG_VARIANT(f64, i32)
SCRIPT_ARG_VARIANT(f64, i64)
SCRIPT_ARG_VARIANT(f64, u32)
SCRIPT_ARG_VARIANT(f64, u64)

template<>
struct ScriptArg<StringView>
{
	static StringView Get(ScriptHandle vm, i32 slot) { return Script::Get().GetSlotString(vm, slot); }
	static void Set(ScriptHandle vm, i32 slot, StringView val) { Script::Get().SetSlotString(vm, slot, val); }
};

template<> struct ScriptArg<StringView&> { static void Set(ScriptHandle vm, i32 slot, StringView val) { ScriptArg<StringView>::Set(vm, slot, val); } };
template<> struct ScriptArg<const StringView&> { static void Set(ScriptHandle vm, i32 slot, StringView val) { ScriptArg<StringView>::Set(vm, slot, val); } };

SCRIPT_ARG_VARIANT(StringView, StringView)
// TODO: FIX: Need a cast conversion from StringView to String -> (String)string_view;
//SCRIPT_ARG_VARIANT(StringView, String)

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
	static T Get(ScriptHandle vm, i32 slot)
	{
		ScriptObj* obj = static_cast<ScriptObj*>(Script::Get().GetSlotObject(vm, slot));
		return *static_cast<T*>(obj->Ptr());
	}

	static void Set(ScriptHandle vm, i32 slot, T val)
	{
		Script::SetClass<T>(vm, slot);
		ScriptObj* obj = new (Script::Get().SetSlotNewObject(vm, slot, slot, sizeof(ScriptObjVal<T>))) ScriptObjVal<T>();
		new (obj->Ptr()) T(val);
	}
};

template<typename T>
struct ScriptArg<T&>
{
	static T& Get(ScriptHandle vm, i32 slot)
	{
		ScriptObj* obj = static_cast<ScriptObj*>(Script::Get().GetSlotObject(vm, slot));
		return *static_cast<T*>(obj->Ptr());
	}

	static void Set(ScriptHandle vm, i32 slot, T& val)
	{
		Script::SetClass<T>(vm, slot);
		new (Script::Get().SetSlotNewObject(vm, slot, slot, sizeof(ScriptObjPtr<T>))) ScriptObjPtr<T>(&val);
	}
};

template<typename T>
struct ScriptArg<const T&>
{
	static const T& Get(ScriptHandle vm, i32 slot)
	{
		ScriptObj* obj = static_cast<ScriptObj*>(Script::Get().GetSlotObject(vm, slot));
		return *static_cast<T*>(obj->Ptr());
	}

	static void Set(ScriptHandle vm, i32 slot, const T& val)
	{
		Script::SetClass<T>(vm, slot);
		new (Script::Get().SetSlotNewObject(vm, slot, slot, sizeof(ScriptObjPtr<T>))) ScriptObjPtr<T>(const_cast<T*>(&val));
	}
};

template<typename T>
struct ScriptArg<T*>
{
	static T* Get(ScriptHandle vm, i32 slot)
	{
		ScriptObj* obj = static_cast<ScriptObj*>(Script::Get().GetSlotObject::Get(vm, slot));
		return static_cast<T*>(obj->Ptr());
	}

	static void Set(ScriptHandle vm, i32 slot, T* val)
	{
		Script::SetClass<T>(vm, slot);
		new (Script::Get().SetSlotNewObject(vm, slot, slot, sizeof(ScriptObjPtr<T>))) ScriptObjPtr<T>(val);
	}
};

template<typename T>
struct ScriptArg<const T*>
{
	static const T* Get(ScriptHandle vm, i32 slot)
	{
		ScriptObj* obj = static_cast<ScriptObj*>(Script::Get().GetSlotObject(vm, slot));
		return static_cast<T*>(obj->Ptr());
	}

	static void Set(ScriptHandle vm, i32 slot, const T* val)
	{
		Script::SetClass<T>(vm, slot);
		new (Script::Get().SetSlotNewObject(vm, slot, slot, sizeof(ScriptObjPtr<T>))) ScriptObjPtr<T>(const_cast<T*>(val));
	}
};

// TODO: STL support?
//template<class T, SizeType Size>
//struct ScriptArgArray
//{
//	static Array<T, Size> Get(ScriptHandle vm, i32 slot)
//	{
//		Array<T, Size> val;
//		i32 count = ScriptArg<void*>::GetListCount(vm, slot);
//		ENGINE_ASSERT(Size == count, "List does not match size of array!");
//		for (i32 i = 0; i < count; i++)
//		{
//			ScriptArg<void*>::GetListElement(vm, slot, i, 0);
//			val[i] = ScriptArg<T>::Get(vm, 0);
//		}
//		return val;
//	}
//
//	//static void Set(ScriptHandle vm, i32 slot, const Array<f32, Size>& val)
//	//{
//	//	new (ScriptArg<void*>::Set(vm, slot, slot, sizeof(ScriptObjPtr<T>))) ScriptObjPtr<T>(val);
//	//}
//};
//
//template<class T, SizeType Size>
//struct ScriptArg<Array<T, Size>>
//{
//	static Array<T, Size> Get(ScriptHandle vm, i32 slot) { return ScriptArgArray<T, Size>::Get(vm, slot); }
//};
//
//template<class T, SizeType Size>
//struct ScriptArg<Array<T, Size>&>
//{
//	static Array<T, Size> Get(ScriptHandle vm, i32 slot) { return ScriptArgArray<T, Size>::Get(vm, slot); }
//};
//
//template <class T, SizeType Size>
//struct ScriptArg<const Array<T, Size>&>
//{
//	static Array<T, Size> Get(ScriptHandle vm, i32 slot) { return ScriptArgArray<T, Size>::Get(vm, slot); }
//};
//
//template<class T>
//struct ScriptArgList
//{
//	static List<T> Get(ScriptHandle vm, i32 slot)
//	{
//		List<T> val;
//		i32 count = ScriptArg<void*>::GetListCount(vm, slot);
//		val.resize(count);
//		for (i32 i = 0; i < count; i++)
//		{
//			ScriptArg<void*>::GetListElement(vm, slot, i, 0);
//			val[i] = ScriptArg<T>::Get(vm, 0);
//		}
//		return val;
//	}
//
//	static void Set(ScriptHandle vm, i32 slot, const List<T>& val)
//	{
//		ScriptArg<void*>::SetList(vm, slot);
//		for (const auto& v : val)
//		{
//			ScriptArg<T>::Set(vm, 1, v);
//			ScriptArg<void*>::InsertInList(vm, 0, -1, 1);
//		}
//	}
//};
//
//template<class T>
//struct ScriptArg<List<T>>
//{
//	static List<T> Get(ScriptHandle vm, i32 slot) { return ScriptArgList<T>::Get(vm, slot); }
//	static void Set(ScriptHandle vm, i32 slot, const List<T>& val) { return ScriptArgList<T>::Set(vm, slot, val); }
//};
//
//template<class T>
//struct ScriptArg<List<T>&>
//{
//	static List<T> Get(ScriptHandle vm, i32 slot) { return ScriptArgList<T>::Get(vm, slot); }
//	static void Set(ScriptHandle vm, i32 slot, const List<T>& val) { return ScriptArgList<T>::Set(vm, slot, val); }
//};
//
//template <class T>
//struct ScriptArg<const List<T>&>
//{
//	static List<T> Get(ScriptHandle vm, i32 slot) { return ScriptArgList<T>::Get(vm, slot); }
//	static void Set(ScriptHandle vm, i32 slot, const List<T>& val) { return ScriptArgList<T>::Set(vm, slot, val); }
//};

//template <typename T>
//struct ScriptArg<Resource<T>>
//{
//	static Resource<T> Get(ScriptHandle vm, i32 slot)
//	{
//		ScriptObj* obj = static_cast<ScriptObj*>(ScriptArg<void*>::Get(vm, slot));
//		return *static_cast<Resource<T>*>(obj->Ptr());
//	}
//
//	template <typename U>
//	static void Set(ScriptHandle vm, i32 slot, U val)
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
//	static Resource<T> Get(ScriptHandle vm, i32 slot)
//	{
//		ScriptObj* obj = static_cast<ScriptObj*>(ScriptArg<void*>::Get(vm, slot));
//		return *static_cast<Resource<T>*>(obj->Ptr());
//	}
//
//	template <typename U>
//	static void Set(ScriptHandle vm, i32 slot, U val)
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
	static R CallWithArguments(ScriptHandle vm, R(*f)(Args...))
	{
		constexpr SizeType arity = meta::function_traits<decltype(f)>::arity;
		Script::EnsureSlots(vm, arity);
		return CallImpl(vm, f, meta::make_index_sequence<arity>{});
	}

	// member function pointer
	template<typename R, typename C, typename... Args>
	static R CallWithArguments(ScriptHandle vm, R(C::* f)(Args...))
	{
		constexpr SizeType arity = meta::function_traits<decltype(f)>::arity;
		Script::EnsureSlots(vm, arity);
		return CallImpl(vm, f, meta::make_index_sequence<arity>{});
	}

	// const member function pointer
	template<typename R, typename C, typename... Args>
	static R CallWithArguments(ScriptHandle vm, R(C::* f)(Args...) const)
	{
		constexpr SizeType arity = meta::function_traits<decltype(f)>::arity;
		Script::EnsureSlots(vm, arity);
		return CallImpl(vm, f, meta::make_index_sequence<arity>{});
	}

	// function pointer
	template<typename R, typename... Args, SizeType... index>
	static R CallImpl(ScriptHandle vm, R(*f)(Args...), meta::index_sequence<index...>)
	{
		using Traits = meta::function_traits<meta::remove_reference_t<decltype(f)>>;
		return f(ScriptArg<typename Traits::template argument_t<index>>::Get(vm, index + 1)...);
	}

	// member function pointer
	template<typename R, typename C, typename... Args, SizeType... index>
	static R CallImpl(ScriptHandle vm, R(C::* f)(Args...), meta::index_sequence<index...>)
	{
		using Traits = meta::function_traits<decltype(f)>;
		ScriptObj* obj = static_cast<ScriptObj*>(ScriptArg<void*>::Get(vm, 0));
		C* ptr = static_cast<C*>(obj->Ptr());
		return (ptr->*f)(ScriptArg<typename Traits::template argument_t<index>>::Get(vm, index + 1)...);
	}

	// const member function pointer
	template<typename R, typename C, typename... Args, SizeType... index>
	static R CallImpl(ScriptHandle vm, R(C::* f)(Args...) const, meta::index_sequence<index...>)
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
	static void Call(ScriptHandle vm, R(*f)(Args...))
	{
		ScriptInvokeImpl::CallWithArguments(vm, std::forward<R(*)(Args...)>(f));
	}

	// member function pointer
	template<typename R, typename C, typename... Args>
	static void Call(ScriptHandle vm, R(C::* f)(Args...))
	{
		ScriptInvokeImpl::CallWithArguments(vm, std::forward<R(C::*)(Args...)>(f));
	}

	// const member function pointer
	template<typename R, typename C, typename... Args>
	static void Call(ScriptHandle vm, R(C::* f)(Args...) const)
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
	static void Call(ScriptHandle vm, R(*f)(Args...))
	{
		using ReturnType = typename meta::function_traits<meta::remove_reference_t<decltype(f)>>::return_type;
		ScriptArg<ReturnType>::Set(vm, 0, ScriptInvokeImpl::CallWithArguments(vm, std::forward<R(*)(Args...)>(f)));
	}

	// member function pointer
	template<typename R, typename C, typename... Args>
	static void Call(ScriptHandle vm, R(C::* f)(Args...))
	{
		using ReturnType = typename meta::function_traits<meta::remove_reference_t<decltype(f)>>::return_type;
		ScriptArg<ReturnType>::Set(vm, 0, ScriptInvokeImpl::CallWithArguments(vm, std::forward<R(C::*)(Args...)>(f)));
	}

	// const member function pointer
	template<typename R, typename C, typename... Args>
	static void Call(ScriptHandle vm, R(C::* f)(Args...) const)
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

	static void Call(ScriptHandle vm)
	{
		ScriptInvoke<std::is_void<R>::value>::Call(vm, f);
	}
};

template<typename R, typename C, typename... Args, R(C::* f)(Args...)>
class ScriptInvokeWrapper<R(C::*)(Args...), f>
{
private:
	friend class Script;

	static void Call(ScriptHandle vm)
	{
		ScriptInvoke<std::is_void<R>::value>::Call(vm, f);
	}
};

template<typename R, typename C, typename... Args, R(C::* f)(Args...) const>
class ScriptInvokeWrapper<R(C::*)(Args...) const, f>
{
private:
	friend class Script;

	static void Call(ScriptHandle vm)
	{
		ScriptInvoke<std::is_void<R>::value>::Call(vm, f);
	}
};

template<typename T, typename U, U T::* Field>
class ScriptProperty
{
private:
	friend class Script;

	static void Get(ScriptHandle vm)
	{
		T* obj = ScriptArg<T*>::Get(vm, 0);
		ScriptArg<U>::Set(vm, 0, obj->*Field);
	}

	static void Set(ScriptHandle vm)
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

	static void Get(ScriptHandle vm)
	{
		ScriptArg<T>::Set(vm, 0, T(Val));
	}
};

template<typename T, typename... Args, SizeType... index>
void Script::Construct(ScriptHandle vm, void* memory, meta::index_sequence<index...>)
{
	using Traits = meta::parameter_pack_traits<Args...>;
	EnsureSlots(vm, Traits::count);
	ScriptObjVal<T>* obj = new (memory) ScriptObjVal<T>{};
	new (obj->Ptr()) T{ ScriptArg<typename Traits::template parameter_t<index>>::Get(vm, index + 1)... };
}

template<typename T, typename... Args>
void Script::Allocate(ScriptHandle vm)
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
void Script::BeginClass(StringView className)
{
	BeginClass(className);
	RegisterClass(Type<T>::Id());

	ScriptMethodFn allocate = &Allocate<T, Args...>;
	RegisterConstructor(0, "", allocate);

	ScriptFinalizerFn finalize = &Finalize<T>;
	RegisterDestructor(finalize);
}

template<typename T>
void Script::BeginClass(StringView className, ScriptMethodFn allocate, ScriptFinalizerFn finalize)
{
	BeginClass(className);
	RegisterClass(Type<T>::Id());
	RegisterConstructor(0, "", allocate);
	RegisterDestructor(finalize);
}

//template <typename TData>
//void Script::BeginResourceClass(StringView className)
//{
//	Script::BeginClass<TData>(className);
//
//	ResourceClassWrapper wrapper;
//	wrapper.createFn = [](ScriptHandle vm)
//	{
//		const auto& filename = ScriptArg<const std::string&>::Get(vm, 2);
//		auto res = MetaResource::Create<TData>(filename);
//		ScriptArg<MetaResource>::Set(vm, 0, res);
//	};
//
//	wrapper.getDataFn = [](ScriptHandle vm, MetaResource res)
//	{
//		auto& data = res.GetData<TData>();
//		ScriptArg<TData&>::Set(vm, 0, data);
//	};
//
//	RegisterResourceClass(Type<TData>::Id(), wrapper);
//}

//template <typename TCmp>
//void Script::BeginComponentClass(StringView className)
//{
//	Script::BeginClass<TCmp>(className);
//
//	ComponentClassWrapper wrapper;
//	wrapper.hasComponentFn = [](ScriptHandle vm, Entity e)
//		{
//			ScriptArg<bool>::Set(vm, 0, e.HasComponent<TCmp>());
//		};
//	wrapper.addComponentFn = [](ScriptHandle vm, Entity e)
//		{
//			auto& cmp = e.AddComponent<TCmp>();
//			ScriptArg<TCmp&>::Set(vm, 0, cmp);
//		};
//	wrapper.getComponentFn = [](ScriptHandle vm, Entity e)
//		{
//			auto& cmp = e.GetComponent<TCmp>();
//			ScriptArg<TCmp&>::Set(vm, 0, cmp);
//		};
//	wrapper.removeComponentFn = [](ScriptHandle vm, Entity e)
//		{
//			e.RemoveComponent<TCmp>();
//		};
//
//	RegisterComponentClass(Type<TCmp>::Id(), wrapper);
//}

template<typename F, F f>
void Script::BindFunction(bool isStatic, StringView signature)
{
	RegisterFunction(isStatic, signature, ScriptInvokeWrapper<decltype(f), f>::Call);
}

inline void Script::BindCFunction(bool isStatic, StringView signature, ScriptMethodFn fn)
{
	RegisterFunction(isStatic, signature, fn);
}

template<typename T, typename U, U T::* Field>
void Script::BindGetter(StringView signature)
{
	RegisterFunction(false, signature, ScriptProperty<T, U, Field>::Get);
}

template<typename T, typename U, U T::* Field>
void Script::BindSetter(StringView signature)
{
	RegisterFunction(false, signature, ScriptProperty<T, U, Field>::Set);
}

template <typename T, typename U, const U Val>
void Script::BindEnumVal(StringView signature)
{
	RegisterFunction(true, signature, ScriptEnumVal<T, U, Val>::Get);
}