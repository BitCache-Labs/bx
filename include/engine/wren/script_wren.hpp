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

class ScriptWren;

struct ScriptWrenUserData
{
	WrenVM* vm{ nullptr };
	ScriptWren* ctx{ nullptr };

	bool initialized{ false };
	bool error{ false };
	void* data{ nullptr };
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

	void SetUserData(ScriptHandle vm, void* data) override;
	void* GetUserData(ScriptHandle vm) override;

	bool CompileString(ScriptHandle vm, StringView moduleName, StringView string) override;
	bool CompileFile(ScriptHandle vm, StringView moduleName, StringView filepath) override;

	bool HasError(ScriptHandle vm) override;
	void ClearError(ScriptHandle vm) override;

	ScriptHandle MakeCallHandle(ScriptHandle vm, StringView signature) override;
	ScriptHandle MakeClassHandle(ScriptHandle vm, StringView moduleName, StringView className) override;
	void ReleaseHandle(ScriptHandle vm, ScriptHandle handle) override;
	u64 GetHandleValue(ScriptHandle handle) override;

	void EnsureSlots(ScriptHandle vm, i32 numSlots) override;
	void CallFunction(ScriptHandle vm, ScriptHandle handle) override;

	void BeginModule(StringView moduleName) override;
	StringView GetCurrentModule() const override;
	void EndModule() override;

	void BeginClass(StringView className, TypeId typeId) override;
	StringView GetCurrentClass() const override;
	void EndClass() override;

	const ScriptClassInfo& GetClassInfo(TypeId typeId) override;
	void SetClass(ScriptHandle vm, i32 slot, TypeId typeId) override;

	void BindConstructor(StringView signature, ScriptMethodFn func) override;
	void BindDestructor(ScriptFinalizerFn func) override;

	void BindFunction(bool isStatic, StringView signature, ScriptMethodFn func) override;

	// Getters
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
	ScriptHandle GetSlotHandle(ScriptHandle vm, i32 slot) override;

	i32 GetListCount(ScriptHandle vm, i32 slot) override;
	void GetListElement(ScriptHandle vm, i32 listSlot, i32 index, i32 elementSlot) override;

	// Setters
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
	void SetSlotHandle(ScriptHandle vm, i32 slot, ScriptHandle handle) override;

	void SetSlotNewList(ScriptHandle vm, i32 slot) override;
	void SetListElement(ScriptHandle vm, i32 listSlot, i32 index, i32 elementSlot) override;
	void InsertInList(ScriptHandle vm, i32 listSlot, i32 index, i32 elementSlot) override;

public:
	ScriptWrenUserData* GetUserData(WrenVM* vm);

private:
	static WrenForeignMethodFn WrenBindForeignMethod(WrenVM* vm, const char* moduleName, const char* className, bool isStatic, const char* signature);
	static WrenForeignMethodFn WrenAllocate(WrenVM* vm, const SizeType classHash);
	static WrenFinalizerFn WrenFinalizer(WrenVM* vm, const SizeType classHash);
	static WrenForeignClassMethods WrenBindForeignClass(WrenVM* vm, const char* moduleName, const char* className);
	static WrenLoadModuleResult WrenLoadModule(WrenVM* vm, const char* moduleName);
	static void WrenWrite(WrenVM* vm, const char* text);
	static void WrenError(WrenVM* vm, WrenErrorType errorType, const char* module, const i32 line, const char* msg);

private:
	// Wren binding context state
	StringView m_moduleName{ nullptr };
	StringView m_className{ nullptr };
	List<ScriptModuleSource> m_apiModuleSources{};

	// Wren storage for module code sources
	HashMap<CString<64>, String> m_wrenModulesSource{};

	// Reflection mappings utility
	HashMap<SizeType, WrenForeignMethodFn> m_foreignMethods{};
	HashMap<SizeType, WrenForeignMethodFn> m_foreignConstructors{};
	HashMap<SizeType, WrenFinalizerFn> m_foreignDestructors{};

	HashMap<TypeId, ScriptClassInfo> m_foreignClassRegistry{};
	HashMap<u32, TypeId> m_wrenTypeIdMap{};
};