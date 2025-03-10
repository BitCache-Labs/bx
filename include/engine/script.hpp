#pragma once

#include <engine/api.hpp>
#include <engine/guard.hpp>
#include <engine/type.hpp>
#include <engine/log.hpp>
#include <engine/traits.hpp>
#include <engine/enum.hpp>
#include <engine/string.hpp>
#include <engine/array.hpp>
#include <engine/list.hpp>

LOG_CHANNEL(Script)

// TODO: While the handle approach makes sense for other modules
// the one exception is the VM, atm the VM is treated as a handle but we should make
// an exception for this, eveything else but the VM can be a handle.
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
			class_<Class##ScriptApiRegister>(BX_STR(Class##_ScriptApiRegister))		\
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

class BX_API Script
{
	BX_MODULE_INTERFACE(Script)

public:
	virtual bool Initialize() = 0;
	virtual void Shutdown() = 0;

	virtual ScriptHandle CreateVm(const ScriptVmInfo& info) = 0;
	virtual void DestroyVm(ScriptHandle vm) = 0;

	virtual void SetUserData(ScriptHandle vm, void* data) = 0;
	virtual void* GetUserData(ScriptHandle vm) = 0;

	virtual bool CompileString(ScriptHandle vm, StringView moduleName, StringView string) = 0;
	virtual bool CompileFile(ScriptHandle vm, StringView moduleName, StringView filepath) = 0;

	virtual bool HasError(ScriptHandle vm) = 0;
	virtual void ClearError(ScriptHandle vm) = 0;

	virtual void BeginModule(StringView moduleName) = 0;
	virtual StringView GetCurrentModule() const = 0;
	virtual void EndModule() = 0;

	virtual void BeginClass(StringView className, TypeId typeId) = 0;
	virtual StringView GetCurrentClass() const = 0;
	virtual void EndClass() = 0;

	virtual const ScriptClassInfo& GetClassInfo(TypeId typeId) = 0;
	virtual void SetClass(ScriptHandle vm, i32 slot, TypeId typeId) = 0;

	template<typename T>
	void SetClass(ScriptHandle vm, i32 slot)
	{
		SetClass(vm, slot, Type<T>::Id());
	}

	template <typename T>
	void BeginClass(StringView className);

	template <typename T, typename U = std::underlying_type_t<T>>
	void BeginEnum(StringView className);

	virtual void BindConstructor(StringView signature, ScriptMethodFn func) = 0;
	virtual void BindDestructor(ScriptFinalizerFn func) = 0;

	template<typename T, typename... Args, SizeType... Index>
	void Construct(ScriptHandle vm, void* memory, meta::index_sequence<Index...>);

	template<typename T, typename... Args>
	void Allocate(ScriptHandle vm);

	template<typename T>
	void Finalize(void* data);

	template <typename T, typename... Args>
	void BindConstructor(StringView signature);

	template <typename T>
	void BindDestructor();

	virtual void BindFunction(bool isStatic, StringView signature, ScriptMethodFn func) = 0;

	template<typename Fn, Fn F>
	void BindFunction(bool isStatic, StringView signature);

	template<typename T, typename U, U T::* Field>
	void BindGetter(StringView signature);

	template<typename T, typename U, U T::* Field>
	void BindSetter(StringView signature);

	template <typename T, T Val>
	void BindEnumVal(StringView signature);

	// Getters
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
	virtual ScriptHandle GetSlotHandle(ScriptHandle vm, i32 slot) = 0;

	virtual i32 GetListCount(ScriptHandle vm, i32 slot) = 0;
	virtual void GetListElement(ScriptHandle vm, i32 listSlot, i32 index, i32 elementSlot) = 0;

	template <typename T>
	T GetSlotArg(ScriptHandle vm, i32 slot);

	// Setters
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
	virtual void SetSlotHandle(ScriptHandle vm, i32 slot, ScriptHandle handle) = 0;

	virtual void SetSlotNewList(ScriptHandle vm, i32 slot) = 0;
	virtual void SetListElement(ScriptHandle vm, i32 listSlot, i32 index, i32 elementSlot) = 0;
	virtual void InsertInList(ScriptHandle vm, i32 listSlot, i32 index, i32 elementSlot) = 0;

	template <typename T>
	void SetSlotArg(ScriptHandle vm, i32 slot, T&& val);

	// Script handles
	virtual ScriptHandle MakeCallHandle(ScriptHandle vm, StringView signature) = 0;
	virtual ScriptHandle MakeClassHandle(ScriptHandle vm, StringView moduleName, StringView className) = 0;
	virtual void ReleaseHandle(ScriptHandle vm, ScriptHandle handle) = 0;
	virtual u64 GetHandleValue(ScriptHandle handle) = 0;

	virtual void EnsureSlots(ScriptHandle vm, i32 numSlots) = 0;
	virtual void CallFunction(ScriptHandle vm, ScriptHandle handle) = 0;
};

// Script API inspired by: https://github.com/Nelarius/wrenpp

template<typename T>
struct ScriptArg;

#define DECLARE_SCRIPT_ARG(Type, GetFunc, SetFunc)																											\
template<> struct ScriptArg<Type>																												\
{																																				\
	static Type Get(ScriptHandle vm, i32 slot) { return Script::Get().##GetFunc(vm, slot); }												\
	static void Set(ScriptHandle vm, i32 slot, Type val) { Script::Get().##SetFunc(vm, slot, val); }										\
};																																				\
template<> struct ScriptArg<Type&> { static void Set(ScriptHandle vm, i32 slot, Type val) { ScriptArg<Type>::Set(vm, slot, val); } };			\
template<> struct ScriptArg<const Type&> { static void Set(ScriptHandle vm, i32 slot, Type val) { ScriptArg<Type>::Set(vm, slot, val); } };

// Basic types
DECLARE_SCRIPT_ARG(bool, GetSlotBool, SetSlotBool)
DECLARE_SCRIPT_ARG(u8, GetSlotU8, SetSlotU8)
DECLARE_SCRIPT_ARG(u16, GetSlotU16, SetSlotU16)
DECLARE_SCRIPT_ARG(u32, GetSlotU32, SetSlotU32)
DECLARE_SCRIPT_ARG(u64, GetSlotU64, SetSlotU64)
DECLARE_SCRIPT_ARG(i8, GetSlotI8, SetSlotI8)
DECLARE_SCRIPT_ARG(i16, GetSlotI16, SetSlotI16)
DECLARE_SCRIPT_ARG(i32, GetSlotI32, SetSlotI32)
DECLARE_SCRIPT_ARG(i64, GetSlotI64, SetSlotI64)
DECLARE_SCRIPT_ARG(f32, GetSlotF32, SetSlotF32)
DECLARE_SCRIPT_ARG(f64, GetSlotF64, SetSlotF64)

// String support
DECLARE_SCRIPT_ARG(StringView, GetSlotString, SetSlotString)

template<SizeType N>
struct ScriptArg<CString<N>>
{
	static CString<N> Get(ScriptHandle vm, i32 slot) { return Script::Get().GetSlotString(vm, slot); }
	static void Set(ScriptHandle vm, i32 slot, CString<N> val) { Script::Get().SetSlotString(vm, slot, val.c_str()); }
};
template<SizeType N>
struct ScriptArg<CString<N>&>
{
	static void Set(ScriptHandle vm, i32 slot, CString<N> val) { ScriptArg<StringView>::Set(vm, slot, val); }
};
template<SizeType N>
struct ScriptArg<const CString<N>&>
{
	static void Set(ScriptHandle vm, i32 slot, CString<N> val) { ScriptArg<StringView>::Set(vm, slot, val); }
};

// Containers support
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

template<class T>
struct ScriptArgList
{
	static List<T> Get(ScriptHandle vm, i32 slot)
	{
		Script::Get().EnsureSlots(vm, slot + 2);

		List<T> val;
		i32 count = Script::Get().GetListCount(vm, slot);
		val.resize(count);
		for (i32 i = 0; i < count; i++)
		{
			Script::Get().GetListElement(vm, slot, i, slot + 1);
			val[i] = ScriptArg<T>::Get(vm, slot + 1);
		}
		return val;
	}

	static void Set(ScriptHandle vm, i32 slot, const List<T>& val)
	{
		Script::Get().SetSlotNewList(vm, slot);
		for (const auto& v : val)
		{
			ScriptArg<T>::Set(vm, slot + 1, v);
			Script::Get().InsertInList(vm, slot, -1, slot + 1);
		}
	}
};

template<class T>
struct ScriptArg<List<T>>
{
	static List<T> Get(ScriptHandle vm, i32 slot) { return ScriptArgList<T>::Get(vm, slot); }
	static void Set(ScriptHandle vm, i32 slot, const List<T>& val) { return ScriptArgList<T>::Set(vm, slot, val); }
};

template<class T>
struct ScriptArg<List<T>&>
{
	static List<T> Get(ScriptHandle vm, i32 slot) { return ScriptArgList<T>::Get(vm, slot); }
	static void Set(ScriptHandle vm, i32 slot, const List<T>& val) { return ScriptArgList<T>::Set(vm, slot, val); }
};

template <class T>
struct ScriptArg<const List<T>&>
{
	static List<T> Get(ScriptHandle vm, i32 slot) { return ScriptArgList<T>::Get(vm, slot); }
	static void Set(ScriptHandle vm, i32 slot, const List<T>& val) { return ScriptArgList<T>::Set(vm, slot, val); }
};

// Base class for script objects
struct ScriptObj
{
	virtual ~ScriptObj() = default;
	virtual void* Ptr() = 0;
};

// Values live within wren VM
template<typename T>
struct ScriptObjVal : public ScriptObj
{
	explicit ScriptObjVal() : obj() {}
	virtual ~ScriptObjVal() { static_cast<T*>(Ptr())->~T(); }
	void* Ptr() override { return &obj; }
	typename std::aligned_storage<sizeof(T), alignof(T)>::type obj;
};

// Pointers live within host
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
		Script::Get().SetClass<T>(vm, slot);
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
		Script::Get().SetClass<T>(vm, slot);
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
		Script::Get().SetClass<T>(vm, slot);
		new (Script::Get().SetSlotNewObject(vm, slot, slot, sizeof(ScriptObjPtr<T>))) ScriptObjPtr<T>(const_cast<T*>(&val));
	}
};

template<typename T>
struct ScriptArg<T*>
{
	static T* Get(ScriptHandle vm, i32 slot)
	{
		ScriptObj* obj = static_cast<ScriptObj*>(Script::Get().GetSlotObject(vm, slot));
		return static_cast<T*>(obj->Ptr());
	}

	static void Set(ScriptHandle vm, i32 slot, T* val)
	{
		Script::Get().SetClass<T>(vm, slot);
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
		Script::Get().SetClass<T>(vm, slot);
		new (Script::Get().SetSlotNewObject(vm, slot, slot, sizeof(ScriptObjPtr<T>))) ScriptObjPtr<T>(const_cast<T*>(val));
	}
};

// Invoke meta programming
class ScriptInvokeImpl
{
private:
	template<bool RetVoid>
	friend class ScriptInvoke;

	// function pointer
	template<typename R, typename... Args>
	static R CallWithArguments(ScriptHandle vm, R(*F)(Args...))
	{
		constexpr SizeType arity = meta::function_traits<decltype(F)>::arity;
		Script::Get().EnsureSlots(vm, arity);
		return CallImpl(vm, F, meta::make_index_sequence<arity>{});
	}

	// member function pointer
	template<typename R, typename C, typename... Args>
	static R CallWithArguments(ScriptHandle vm, R(C::*F)(Args...))
	{
		constexpr SizeType arity = meta::function_traits<decltype(F)>::arity;
		Script::Get().EnsureSlots(vm, arity);
		return CallImpl(vm, F, meta::make_index_sequence<arity>{});
	}

	// const member function pointer
	template<typename R, typename C, typename... Args>
	static R CallWithArguments(ScriptHandle vm, R(C::*F)(Args...) const)
	{
		constexpr SizeType arity = meta::function_traits<decltype(F)>::arity;
		Script::Get().EnsureSlots(vm, arity);
		return CallImpl(vm, F, meta::make_index_sequence<arity>{});
	}

	// function pointer
	template<typename R, typename... Args, SizeType... index>
	static R CallImpl(ScriptHandle vm, R(*F)(Args...), meta::index_sequence<index...>)
	{
		using Traits = meta::function_traits<meta::remove_reference_t<decltype(F)>>;
		return F(ScriptArg<typename Traits::template argument_t<index>>::Get(vm, index + 1)...);
	}

	// member function pointer
	template<typename R, typename C, typename... Args, SizeType... index>
	static R CallImpl(ScriptHandle vm, R(C::*F)(Args...), meta::index_sequence<index...>)
	{
		using Traits = meta::function_traits<decltype(F)>;
		ScriptObj* obj = static_cast<ScriptObj*>(Script::Get().GetSlotObject(vm, 0));
		C* ptr = static_cast<C*>(obj->Ptr());
		return (ptr->*F)(ScriptArg<typename Traits::template argument_t<index>>::Get(vm, index + 1)...);
	}

	// const member function pointer
	template<typename R, typename C, typename... Args, SizeType... index>
	static R CallImpl(ScriptHandle vm, R(C::*F)(Args...) const, meta::index_sequence<index...>)
	{
		using Traits = meta::function_traits<decltype(F)>;
		ScriptObj* obj = static_cast<ScriptObj*>(Script::Get().GetSlotObject(vm, 0));
		C* ptr = static_cast<C*>(obj->Ptr());
		return (ptr->*F)(ScriptArg<typename Traits::template argument_t<index>>::Get(vm, index + 1)...);
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
	static void Call(ScriptHandle vm, R(*F)(Args...))
	{
		ScriptInvokeImpl::CallWithArguments(vm, std::forward<R(*)(Args...)>(F));
	}

	// member function pointer
	template<typename R, typename C, typename... Args>
	static void Call(ScriptHandle vm, R(C::*F)(Args...))
	{
		ScriptInvokeImpl::CallWithArguments(vm, std::forward<R(C::*)(Args...)>(F));
	}

	// const member function pointer
	template<typename R, typename C, typename... Args>
	static void Call(ScriptHandle vm, R(C::*F)(Args...) const)
	{
		ScriptInvokeImpl::CallWithArguments(vm, std::forward<R(C::*)(Args...) const>(F));
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
	static void Call(ScriptHandle vm, R(*F)(Args...))
	{
		using ReturnType = typename meta::function_traits<meta::remove_reference_t<decltype(F)>>::return_type;
		ScriptArg<ReturnType>::Set(vm, 0, ScriptInvokeImpl::CallWithArguments(vm, std::forward<R(*)(Args...)>(F)));
	}

	// member function pointer
	template<typename R, typename C, typename... Args>
	static void Call(ScriptHandle vm, R(C::*F)(Args...))
	{
		using ReturnType = typename meta::function_traits<meta::remove_reference_t<decltype(F)>>::return_type;
		ScriptArg<ReturnType>::Set(vm, 0, ScriptInvokeImpl::CallWithArguments(vm, std::forward<R(C::*)(Args...)>(F)));
	}

	// const member function pointer
	template<typename R, typename C, typename... Args>
	static void Call(ScriptHandle vm, R(C::*F)(Args...) const)
	{
		using ReturnType = typename meta::function_traits<meta::remove_reference_t<decltype(F)>>::return_type;
		ScriptArg<ReturnType>::Set(vm, 0, ScriptInvokeImpl::CallWithArguments(vm, std::forward<R(C::*)(Args...) const>(F)));
	}
};

template<typename Signature, Signature>
class ScriptInvokeWrapper;

template<typename R, typename... Args, R(*F)(Args...)>
class ScriptInvokeWrapper<R(*)(Args...), F>
{
private:
	friend class Script;

	static void Call(ScriptHandle vm)
	{
		ScriptInvoke<std::is_void<R>::value>::Call(vm, F);
	}
};

template<typename R, typename C, typename... Args, R(C::*F)(Args...)>
class ScriptInvokeWrapper<R(C::*)(Args...), F>
{
private:
	friend class Script;

	static void Call(ScriptHandle vm)
	{
		ScriptInvoke<std::is_void<R>::value>::Call(vm, F);
	}
};

template<typename R, typename C, typename... Args, R(C::*F)(Args...) const>
class ScriptInvokeWrapper<R(C::*)(Args...) const, F>
{
private:
	friend class Script;

	static void Call(ScriptHandle vm)
	{
		ScriptInvoke<std::is_void<R>::value>::Call(vm, F);
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

template<typename T, T Val>
class ScriptEnumVal
{
private:
	friend class Script;

	static void Get(ScriptHandle vm)
	{
		ScriptArg<T>::Set(vm, 0, Val);
	}
};

// Script implementation
template<typename T>
void Script::BeginClass(StringView className)
{
	BeginClass(className, Type<T>::Id());
}

template <typename T, typename U>
void Script::BeginEnum(StringView className)
{
	BeginClass(className, Type<T>::Id());

	BindConstructor("",
		[](ScriptHandle vm)
		{
			void* memory = Script::Get().SetSlotNewObject(vm, 0, 0, sizeof(ScriptObjVal<T>));
			Script::Get().EnsureSlots(vm, 1);
			ScriptObjVal<T>* obj = new (memory) ScriptObjVal<T>{};
			new (obj->Ptr()) T(Enum::as_type<T>(ScriptArg<U>::Get(vm, 1)));
		});

	BindDestructor(
		[](void* data)
		{
			ScriptObj* obj = static_cast<ScriptObj*>(data);
			obj->~ScriptObj();
		});
}

template<typename T, typename... Args, SizeType... Index>
void Script::Construct(ScriptHandle vm, void* memory, meta::index_sequence<Index...>)
{
	using Traits = meta::parameter_pack_traits<Args...>;
	Script::Get().EnsureSlots(vm, Traits::count);
	ScriptObjVal<T>* obj = new (memory) ScriptObjVal<T>{};
	new (obj->Ptr()) T{ ScriptArg<typename Traits::template parameter_t<Index>>::Get(vm, Index + 1)... };
}

template<typename T, typename... Args>
void Script::Allocate(ScriptHandle vm)
{
	void* memory = Script::Get().SetSlotNewObject(vm, 0, 0, sizeof(ScriptObjVal<T>));
	Construct<T, Args...>(vm, memory, meta::make_index_sequence<meta::parameter_pack_traits<Args...>::count>{});
}

template<typename T>
void Script::Finalize(void* data)
{
	ScriptObj* obj = static_cast<ScriptObj*>(data);
	obj->~ScriptObj();
}

template<typename T, typename... Args>
void Script::BindConstructor(StringView signature)
{
	ScriptMethodFn allocate = &Allocate<T, Args...>;
	BindConstructor(signature, allocate);
}

template<typename T>
void Script::BindDestructor()
{
	ScriptFinalizerFn finalize = &Finalize<T>;
	BindDestructor(finalize);
}

template<typename Fn, Fn F>
void Script::BindFunction(bool isStatic, StringView signature)
{
	BindFunction(isStatic, signature, ScriptInvokeWrapper<decltype(F), F>::Call);
}

template<typename T, typename U, U T::* Field>
void Script::BindGetter(StringView signature)
{
	BindFunction(false, signature, ScriptProperty<T, U, Field>::Get);
}

template<typename T, typename U, U T::* Field>
void Script::BindSetter(StringView signature)
{
	BindFunction(false, signature, ScriptProperty<T, U, Field>::Set);
}

template <typename T, T Val>
void Script::BindEnumVal(StringView signature)
{
	BindFunction(true, signature, ScriptEnumVal<T, Val>::Get);
}

template <typename T>
T Script::GetSlotArg(ScriptHandle vm, i32 slot)
{
	return ScriptArg<T>::Get(vm, slot);
}

template <typename T>
void Script::SetSlotArg(ScriptHandle vm, i32 slot, T&& val)
{
	ScriptArg<T>::Set(vm, slot, val);
}