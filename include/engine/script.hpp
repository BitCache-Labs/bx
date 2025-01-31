#pragma once

#include <engine/api.hpp>
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

using ScriptHandle = void*;
constexpr ScriptHandle SCRIPT_INVALID_HANDLE = nullptr;

using ScriptMethodFn = void(*)(ScriptHandle vm);
using ScriptFinalizerFn = void(*)(void* data);

struct BX_API ScriptModuleSource
{
	CString<64> moduleName{};
	StringView moduleSource{};
};

struct BX_API ScriptApiRegister
{
	BX_TYPE(ScriptApiRegister)
};

#define BX_SCRIPT_API_REGISTRATION(Class)											\
static ScriptModuleSource Class##ApiRegisterCallback();								\
namespace                                                                           \
{                                                                                   \
    struct BX_API Class##ScriptApiRegister final : public ScriptApiRegister			\
    {                                                                               \
	BX_TYPE(Class##ScriptApiRegister, ScriptApiRegister)							\
	public:																			\
		Class##ScriptApiRegister()													\
        {                                                                           \
			rttr::registration::													\
			class_<Class##ScriptApiRegister>(STR(Class##_ScriptApiRegister))		\
			.method("Register", Class##ScriptApiRegister::Register);				\
        }                                                                           \
        static ScriptModuleSource Register()										\
        {                                                                           \
			return Class##ApiRegisterCallback();									\
        }                                                                           \
    };                                                                              \
}                                                                                   \
static const Class##ScriptApiRegister g_##Class##ScriptApiRegister;					\
static ScriptModuleSource Class##ApiRegisterCallback()

struct BX_API ScriptVmInfo
{
	SizeType initialHeapSize{ 1024LL * 1024 * 32 };
	SizeType minHeapSize{ 1024LL * 1024 * 16 };
	i32 heapGrowthPercent{ 80 };
};

struct BX_API ScriptClassInfo
{
	CString<64> moduleName{};
	CString<64> className{};
	TypeId typeId{};
	ScriptHandle handle{ SCRIPT_INVALID_HANDLE };
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

	virtual ScriptHandle CreateVm(const ScriptVmInfo& info) = 0;
	virtual void DestroyVm(ScriptHandle vm) = 0;

	virtual bool HasError(ScriptHandle vm) = 0;
	virtual void ClearError(ScriptHandle vm) = 0;

	virtual void CompileString(ScriptHandle vm, StringView moduleName, StringView string) = 0;
	virtual void CompileFile(ScriptHandle vm, StringView moduleName, StringView filepath) = 0;

	virtual void BeginModule(StringView moduleName) = 0;
	virtual void EndModule() = 0;

	virtual void BeginClass(StringView className) = 0;
	virtual void EndClass() = 0;

	virtual void BindFunction(bool isStatic, StringView signature, ScriptMethodFn func) = 0;

	virtual bool GetSlotBool(ScriptHandle vm, i32 slot) = 0;
	virtual u8 GetSlotU8(ScriptHandle vm, i32 slot) = 0;
	virtual u16 GetSlotU16(ScriptHandle vm, i32 slot) = 0;
	virtual u32 GetSlotU32(ScriptHandle vm, i32 slot) = 0;
	virtual u64 GetSlotU64(ScriptHandle vm, i32 slot) = 0;
	virtual i8 GetSlotI8(ScriptHandle vm, i32 slot) = 0;
	virtual i16 GetSlotI16(ScriptHandle vm, i32 slot) = 0;
	virtual i32 GetSlotI32(ScriptHandle vm, i32 slot) = 0;
	virtual i64 GetSlotI64(ScriptHandle vm, i32 slot) = 0;
	virtual f32 GetSlotF32(ScriptHandle vm, i32 slot) = 0;
	virtual f64 GetSlotF64(ScriptHandle vm, i32 slot) = 0;
	virtual StringView GetSlotString(ScriptHandle vm, i32 slot) = 0;
	virtual void* GetSlotObject(ScriptHandle vm, i32 slot) = 0;

	virtual void SetSlotBool(ScriptHandle vm, i32 slot, bool value) = 0;
	virtual void SetSlotU8(ScriptHandle vm, i32 slot, u8 value) = 0;
	virtual void SetSlotU16(ScriptHandle vm, i32 slot, u16 value) = 0;
	virtual void SetSlotU32(ScriptHandle vm, i32 slot, u32 value) = 0;
	virtual void SetSlotU64(ScriptHandle vm, i32 slot, u64 value) = 0;
	virtual void SetSlotI8(ScriptHandle vm, i32 slot, i8 value) = 0;
	virtual void SetSlotI16(ScriptHandle vm, i32 slot, i16 value) = 0;
	virtual void SetSlotI32(ScriptHandle vm, i32 slot, i32 value) = 0;
	virtual void SetSlotI64(ScriptHandle vm, i32 slot, i64 value) = 0;
	virtual void SetSlotF32(ScriptHandle vm, i32 slot, f32 value) = 0;
	virtual void SetSlotF64(ScriptHandle vm, i32 slot, f64 value) = 0;
	virtual void SetSlotString(ScriptHandle vm, i32 slot, StringView text) = 0;
	virtual void* SetSlotNewObject(ScriptHandle vm, i32 slot, i32 classSlot, SizeType size) = 0;

	virtual void EnsureSlots(ScriptHandle vm, i32 numSlots) = 0;

public:
	template <typename T, typename... Args>
	void BeginClass(StringView className);

	template <typename T>
	void BeginClass(StringView className, ScriptMethodFn allocate, ScriptFinalizerFn finalize);

	//template <typename TData>
	//void BeginResourceClass(StringView className);
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
	virtual const ScriptClassInfo& GetClassInfo(TypeId typeId) = 0;

	virtual void SetClass(ScriptHandle vm, i32 slot, TypeId typeId) = 0;

	virtual void RegisterConstructor(SizeType arity, StringView signature, ScriptMethodFn func) = 0;
	virtual void RegisterDestructor(ScriptFinalizerFn func) = 0;

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

#define DECLARE_SCRIPT_ARG(Type, Func)																											\
template<> struct ScriptArg<Type>																												\
{																																				\
	static Type Get(ScriptHandle vm, i32 slot) { return Script::Get().GetSlot##Func(vm, slot); }												\
	static void Set(ScriptHandle vm, i32 slot, Type val) { Script::Get().SetSlot##Func(vm, slot, val); }										\
};																																				\
template<> struct ScriptArg<Type&> { static void Set(ScriptHandle vm, i32 slot, Type val) { ScriptArg<Type>::Set(vm, slot, val); } };			\
template<> struct ScriptArg<const Type&> { static void Set(ScriptHandle vm, i32 slot, Type val) { ScriptArg<Type>::Set(vm, slot, val); } };

DECLARE_SCRIPT_ARG(bool, Bool)
DECLARE_SCRIPT_ARG(u8, U8)
DECLARE_SCRIPT_ARG(u16, U16)
DECLARE_SCRIPT_ARG(u32, U32)
DECLARE_SCRIPT_ARG(u64, U64)
DECLARE_SCRIPT_ARG(i8, I8)
DECLARE_SCRIPT_ARG(i16, I16)
DECLARE_SCRIPT_ARG(i32, I32)
DECLARE_SCRIPT_ARG(i64, I64)
DECLARE_SCRIPT_ARG(f32, F32)
DECLARE_SCRIPT_ARG(f64, F64)
DECLARE_SCRIPT_ARG(StringView, String)

// TODO: FIX: Need a cast conversion from StringView to const char* -> (const char*)string_view;
//SCRIPT_ARG_VARIANT(StringView, const char*)
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
		ScriptObj* obj = static_cast<ScriptObj*>(Script::Get().GetSlotObject(vm, 0));
		C* ptr = static_cast<C*>(obj->Ptr());
		return (ptr->*f)(ScriptArg<typename Traits::template argument_t<index>>::Get(vm, index + 1)...);
	}

	// const member function pointer
	template<typename R, typename C, typename... Args, SizeType... index>
	static R CallImpl(ScriptHandle vm, R(C::* f)(Args...) const, meta::index_sequence<index...>)
	{
		using Traits = meta::function_traits<decltype(f)>;
		ScriptObj* obj = static_cast<ScriptObj*>(Script::Get().GetSlotObject(vm, 0));
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