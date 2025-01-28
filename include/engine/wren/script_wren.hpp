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

	ScriptVM CreateVm(const VmInfo& info) override;
	void DestroyVm(ScriptVM vm) override;

	void ConfigureApi(ScriptVM vm) override;

	void CollectGarbage(ScriptVM vm) override;

	bool HasError(ScriptVM vm) override;
	void ClearError(ScriptVM vm) override;

	void BindVm(ScriptVM vm) override;

	void BeginModule(StringView moduleName) override;
	void EndModule() override;

	void BeginClass(StringView className) override;
	void EndClass() override;

	void BindFunction(bool isStatic, StringView signature, ScriptMethodFn func) override;

	void EnsureSlots(ScriptVM vm, i32 numSlots) override;

	bool GetSlotBool(ScriptVM vm, i32 slot) override;
	double GetSlotDouble(ScriptVM vm, i32 slot) override;
	StringView GetSlotString(ScriptVM vm, i32 slot) override;
	void* GetSlotObject(ScriptVM vm, i32 slot) override;

	void SetSlotBool(ScriptVM vm, i32 slot, bool value) override;
	void SetSlotDouble(ScriptVM vm, i32 slot, f64 value) override;
	void SetSlotString(ScriptVM vm, i32 slot, StringView text) override;
	void* SetSlotNewObject(ScriptVM vm, i32 slot, i32 classSlot, SizeType size) override;

private:
	void RegisterClass(TypeId typeId) override;
	const ScriptClassInfo& GetClassInfo(TypeId typeId) override;

	void SetClass(ScriptVM vm, i32 slot, TypeId typeId) override;

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