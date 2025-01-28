#pragma once

#include <engine/script.hpp>
#include <engine/hash_map.hpp>

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

	//HashMap<TypeId, ScriptClassInfo> m_foreignClassRegistry;
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

	ScriptHandle CreateVm(const VmInfo& info) override;
	void DestroyVm(ScriptHandle vm) override;

	void ConfigureApi(ScriptHandle vm) override;

	void CollectGarbage(ScriptHandle vm) override;

	bool HasError(ScriptHandle vm) override;
	void ClearError(ScriptHandle vm) override;

	void BindVm(ScriptHandle vm) override;

	void BeginModule(StringView moduleName) override;
	void EndModule() override;

	void BeginClass(StringView className) override;
	void EndClass() override;

	void BindCFunction(bool isStatic, StringView signature, ScriptMethodFn fn) override;

	bool GetSlotBool(ScriptHandle vm, i32 slot) override;
	double GetSlotDouble(ScriptHandle vm, i32 slot) override;
	StringView GetSlotString(ScriptHandle vm, i32 slot) override;
	void* GetSlotObject(ScriptHandle vm, i32 slot) override;

	void SetSlotBool(ScriptHandle vm, i32 slot, bool value) override;
	void SetSlotDouble(ScriptHandle vm, i32 slot, f64 value) override;
	void SetSlotString(ScriptHandle vm, i32 slot, StringView text) override;
	void* SetSlotNewObject(ScriptHandle vm, i32 slot, i32 classSlot, SizeType size) override;

private:
	void RegisterClass(TypeId typeId) override;
	void SetClass(ScriptHandle vm, i32 slot, TypeId typeId) override;

	void RegisterConstructor(SizeType arity, StringView signature, ScriptMethodFn func) override;
	void RegisterDestructor(ScriptFinalizerFn func) override;
	void RegisterFunction(bool isStatic, StringView signature, ScriptMethodFn func) override;

	void EnsureSlots(ScriptHandle vm, i32 numSlots) override;

public:
	ScriptVMImpl& GetVmImpl(WrenVM* vm);

private:
	HashMap<ScriptHandle, ScriptVMImpl> m_vmMap;

	// Wren binding context state
	ScriptVMImpl m_boundVm{};
	StringView m_moduleName = nullptr;
	StringView m_className = nullptr;
};