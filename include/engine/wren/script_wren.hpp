#pragma once

#include <engine/script.hpp>
#include <engine/hash_map.hpp>
#include <engine/hash.hpp>

extern "C" {
#include <wren.h>
#include <wren_vm.h>
#include <wren_opt_meta.h>
#include <wren_opt_random.h>
}

struct ScriptVMImpl
{
	bool initialized{ false };
	bool error{ false };

	WrenVM* vm{ nullptr };

	// Wren storage for module code sources
	HashMap<CString<64>, String> wrenModulesSource;

	// Reflection mappings utility
	HashMap<SizeType, WrenForeignMethodFn> foreignMethods;
	HashMap<SizeType, WrenForeignMethodFn> foreignConstructors;
	HashMap<SizeType, WrenFinalizerFn> foreignDestructors;

	HashMap<TypeId, ScriptClassInfo> foreignClassRegistry;
	HashMap<u32, TypeId> wrenTypeIdMap;

	// Extra mappings for specialized template classes
	//HashMap<TypeId, ResourceClassWrapper> m_resourceClassWrappers;
	//HashMap<TypeId, ComponentClassWrapper> m_componentClassWrappers;
};

class BX_API ScriptWren final
	: public Script
{
	BX_MODULE(ScriptWren, Script)

public:
	bool Initialize() override;
	void Shutdown() override;

	ScriptHandle CreateVm(const ScriptVmInfo& info) override;
	void DestroyVm(ScriptHandle vm) override;

	bool HasError(ScriptHandle vm) override;
	void ClearError(ScriptHandle vm) override;

	void BindVm(ScriptHandle vm) override;

	void CompileString(StringView moduleName, StringView string) override;
	void CompileFile(StringView moduleName, StringView filepath) override;

	void BeginModule(StringView moduleName) override;
	void EndModule() override;

	void BeginClass(StringView className) override;
	void EndClass() override;

	void BindFunction(bool isStatic, StringView signature, ScriptMethodFn func) override;

	void EnsureSlots(ScriptHandle vm, i32 numSlots) override;

	bool GetSlotBool(ScriptHandle vm, i32 slot) override;
	u8 GetSlotU8(ScriptHandle vm, i32 slot) override;
	u16 GetSlotU16(ScriptHandle vm, i32 slot) override;
	u32 GetSlotU32(ScriptHandle vm, i32 slot) override;
	u64 GetSlotU64(ScriptHandle vm, i32 slot) override;
	i8 GetSlotI8(ScriptHandle vm, i32 slot) override;
	i16 GetSlotI16(ScriptHandle vm, i32 slot) override;
	i32 GetSlotI32(ScriptHandle vm, i32 slot) override;
	i64 GetSlotI64(ScriptHandle vm, i32 slot) override;
	f32 GetSlotF32(ScriptHandle vm, i32 slot) override;
	f64 GetSlotF64(ScriptHandle vm, i32 slot) override;
	StringView GetSlotString(ScriptHandle vm, i32 slot) override;
	void* GetSlotObject(ScriptHandle vm, i32 slot) override;

	void SetSlotBool(ScriptHandle vm, i32 slot, bool value) override;
	void SetSlotU8(ScriptHandle vm, i32 slot, u8 value) override;
	void SetSlotU16(ScriptHandle vm, i32 slot, u16 value) override;
	void SetSlotU32(ScriptHandle vm, i32 slot, u32 value) override;
	void SetSlotU64(ScriptHandle vm, i32 slot, u64 value) override;
	void SetSlotI8(ScriptHandle vm, i32 slot, i8 value) override;
	void SetSlotI16(ScriptHandle vm, i32 slot, i16 value) override;
	void SetSlotI32(ScriptHandle vm, i32 slot, i32 value) override;
	void SetSlotI64(ScriptHandle vm, i32 slot, i64 value) override;
	void SetSlotF32(ScriptHandle vm, i32 slot, f32 value) override;
	void SetSlotF64(ScriptHandle vm, i32 slot, f64 value) override;
	void SetSlotString(ScriptHandle vm, i32 slot, StringView text) override;
	void* SetSlotNewObject(ScriptHandle vm, i32 slot, i32 classSlot, SizeType size) override;

private:
	void RegisterClass(TypeId typeId) override;
	const ScriptClassInfo& GetClassInfo(TypeId typeId) override;

	void SetClass(ScriptHandle vm, i32 slot, TypeId typeId) override;

	void RegisterConstructor(SizeType arity, StringView signature, ScriptMethodFn func) override;
	void RegisterDestructor(ScriptFinalizerFn func) override;

public:
	ScriptVMImpl* GetVmImpl(WrenVM* vm);

private:
	// Wren binding context state
	ScriptVMImpl* m_boundVm{};
	StringView m_moduleName = nullptr;
	StringView m_className = nullptr;
};