#include <engine/wren/script_wren.hpp>

#include <engine/hash.hpp>
#include <engine/macros.hpp>
#include <engine/time.hpp>
#include <engine/profiler.hpp>
#include <engine/file.hpp>
#include <engine/math.hpp>
#include <engine/string.hpp>
#include <engine/hash_map.hpp>

#include <cstdio>
#include <cstring>
#include <functional>

BX_MODULE_DEFINE(ScriptWren)
BX_MODULE_DEFINE_INTERFACE(Script, ScriptWren)

// Helper functions and Wren callbacks
static SizeType GetClassHash(StringView moduleName, StringView className)
{
	static Hash<StringView> m_strHash;
	return m_strHash(moduleName)
		^ m_strHash(className);
}

static SizeType GetMethodHash(StringView moduleName, StringView className, bool isStatic, StringView signature)
{
	static Hash<StringView> m_strHash;
	return m_strHash(moduleName)
		^ m_strHash(className)
		^ m_strHash(isStatic ? "s" : "")
		^ m_strHash(signature);
}

ScriptWrenUserData* ScriptWren::GetUserData(WrenVM* vm)
{
	auto userData = wrenGetUserData(vm);
	BX_ENSURE(userData != nullptr);
	return static_cast<ScriptWrenUserData*>(userData);
}

WrenForeignMethodFn ScriptWren::WrenBindForeignMethod(WrenVM* vm, const char* moduleName, const char* className, bool isStatic, const char* signature)
{
	auto userData = ScriptWren::Get().GetUserData(vm);

	const StringView moduleNameStr = moduleName;
	const StringView classNameStr = className;
	const StringView signatureStr = signature;

	if (moduleNameStr.compare("random") == 0)
		return nullptr;
	if (moduleNameStr.compare("meta") == 0)
		return nullptr;

	const SizeType hash = GetMethodHash(moduleNameStr, classNameStr, isStatic, signatureStr);
	const auto it = userData->ctx->m_foreignMethods.find(hash);
	if (it == userData->ctx->m_foreignMethods.end())
		return nullptr;
	return it->second;
}

WrenForeignMethodFn ScriptWren::WrenAllocate(WrenVM* vm, const SizeType classHash)
{
	auto userData = ScriptWren::Get().GetUserData(vm);

	const auto it = userData->ctx->m_foreignConstructors.find(classHash);
	if (it != userData->ctx->m_foreignConstructors.end())
		return it->second;
	return nullptr;
}

WrenFinalizerFn ScriptWren::WrenFinalizer(WrenVM* vm, const SizeType classHash)
{
	auto userData = ScriptWren::Get().GetUserData(vm);

	const auto it = userData->ctx->m_foreignDestructors.find(classHash);
	if (it != userData->ctx->m_foreignDestructors.end())
		return it->second;
	return nullptr;
}

WrenForeignClassMethods ScriptWren::WrenBindForeignClass(WrenVM* vm, const char* moduleName, const char* className)
{
	const StringView moduleNameStr = moduleName;
	const StringView classNameStr = className;

	WrenForeignClassMethods methods{};
	methods.allocate = nullptr;
	methods.finalize = nullptr;

	if (moduleNameStr.compare("random") == 0) return methods;
	if (moduleNameStr.compare("meta") == 0) return methods;

	const SizeType classHash = GetClassHash(moduleNameStr, classNameStr);
	methods.allocate = WrenAllocate(vm, classHash);
	methods.finalize = WrenFinalizer(vm, classHash);

	return methods;
}

WrenLoadModuleResult ScriptWren::WrenLoadModule(WrenVM* vm, const char* moduleName)
{
	WrenLoadModuleResult res{};

	auto userData = ScriptWren::Get().GetUserData(vm);

	const CString<64> moduleNameStr = moduleName;

	if (moduleNameStr.compare("random") == 0)
		return res;
	if (moduleNameStr.compare("meta") == 0)
		return res;

	for (const auto& src : userData->ctx->m_apiModuleSources)
	{
		if (!moduleNameStr.compare(src.moduleName))
			continue;

		res.source = src.moduleSource.data();
		return res;
	}

	CString<64> moduleNameFmt{};
	moduleNameFmt.format("{}.wren", moduleNameStr);

	if (userData->ctx->m_wrenModulesSource.find(moduleNameStr) == userData->ctx->m_wrenModulesSource.end())
	{
		FilePath filepath{};
		if (!File::Get().Find("[assets]", moduleNameFmt, filepath))
		{
			BX_LOGE(Script, "Module '{}' can not be found!", moduleNameFmt);
			return res;
		}

		String moduleSrc = File::Get().ReadText(filepath);
		userData->ctx->m_wrenModulesSource.insert(std::make_pair(moduleNameStr, moduleSrc));
	}

	BX_ENSURE(userData->ctx->m_wrenModulesSource.find(moduleNameStr) != userData->ctx->m_wrenModulesSource.end());
	res.source = userData->ctx->m_wrenModulesSource[moduleNameStr].c_str();

	return res;
}

void ScriptWren::WrenWrite(WrenVM* vm, const char* text)
{
	const StringView textStr = text;
	if (textStr.compare("\n") == 0)
		return;

	BX_LOGI(Script, textStr.data());
}

void ScriptWren::WrenError(WrenVM* vm, WrenErrorType errorType, const char* module, const i32 line, const char* msg)
{
	auto userData = ScriptWren::Get().GetUserData(vm);

	switch (errorType)
	{
	case WREN_ERROR_COMPILE:
		BX_LOGE(Script, "[Compile: {} line {}] {}", module, line, msg); break;
	case WREN_ERROR_STACK_TRACE:
		BX_LOGE(Script, "[StackTrace: {} line {}] in {}", module, line, msg); break;
	case WREN_ERROR_RUNTIME:
		BX_LOGE(Script, "[Runtime] {}", msg); break;
	}

	userData->error = true;
}

bool ScriptWren::Initialize()
{
	// Get the list of all classes derived from ScriptApiRegister
	auto derivedClasses = rttr::type::get<ScriptApiRegister>().get_derived_classes();
	for (const auto& derivedClass : derivedClasses)
	{
		const auto& derivedClassName = derivedClass.get_name();
		BX_LOGD(Script, "Registering script api: {}", derivedClassName.data());

		// Retrieve the "Register" method from the derived class
		auto registerMethod = derivedClass.get_method("Register");
		if (!registerMethod.is_valid())
		{
			BX_LOGE(Script, "Register method not found for class: {}", derivedClassName.data());
			continue;
		}

		rttr::instance instance;  // Create a default instance, we are calling a static function
		auto ret = registerMethod.invoke(instance);
		m_apiModuleSources.emplace_back(ret.get_value<ScriptModuleSource>());
	}

	return true;
}

void ScriptWren::Shutdown()
{
	m_wrenModulesSource.clear();
	
	m_foreignMethods.clear();
	m_foreignConstructors.clear();
	m_foreignDestructors.clear();

	m_foreignClassRegistry.clear();
	m_wrenTypeIdMap.clear();

	//m_resourceClassWrappers.clear();
	//m_componentClassWrappers.clear();
}

ScriptHandle ScriptWren::CreateVm(const ScriptVmInfo& info)
{
	auto userData = new ScriptWrenUserData();

	// Create vm
	WrenConfiguration config;
	wrenInitConfiguration(&config);
	config.reallocateFn = 0;
	config.resolveModuleFn = 0;
	config.loadModuleFn = ScriptWren::WrenLoadModule;
	config.bindForeignMethodFn = ScriptWren::WrenBindForeignMethod;
	config.bindForeignClassFn = ScriptWren::WrenBindForeignClass;
	config.writeFn = ScriptWren::WrenWrite;
	config.errorFn = ScriptWren::WrenError;
	config.initialHeapSize = info.initialHeapSize;
	config.minHeapSize = info.minHeapSize;
	config.heapGrowthPercent = info.heapGrowthPercent;
	config.userData = userData;
	auto vm = wrenNewVM(&config);

	// Setup user data
	userData->initialized = false;
	userData->error = false;
	userData->vm = vm;
	userData->ctx = this;
	
	// Compile API into vm
	auto handle = (ScriptHandle)vm;
	for (const auto& src : m_apiModuleSources)
	{
		CompileString(handle, src.moduleName, src.moduleSource);
	}

	//File::FindEach("[assets]", ".wren",
	//	[](const auto& path, const auto& name)
	//	{
	//		WrenCompile(m_vm, name.c_str(), File::ReadTextFile(path).c_str());
	//	});
	//
	//for (auto& it : m_foreignClassRegistry)
	//{
	//	auto& info = it.second;
	//
	//	wrenEnsureSlots(m_vm, 1);
	//	wrenGetVariable(m_vm, info.moduleName.data(), info.className.data(), 0);
	//	info.handle = wrenGetSlotHandle(m_vm, 0);
	//	auto objClass = wrenGetClass(m_vm, info.handle->value);
	//
	//	m_wrenTypeIdMap.insert(std::make_pair(objClass->name->hash, info.typeId));
	//}

	return handle;
}

void ScriptWren::DestroyVm(ScriptHandle vm)
{
	auto wrenVm = (WrenVM*)vm;

	auto userData = GetUserData(wrenVm);
	userData->initialized = false;
	userData->error = false;
	userData->ctx = nullptr;
	userData->vm = nullptr;

	if (wrenVm)
	{
		wrenFreeVM(wrenVm);
		wrenVm = nullptr;
	}
}

void ScriptWren::SetUserData(ScriptHandle vm, void* data)
{
	auto userData = GetUserData((WrenVM*)vm);
	userData->data = data;
}

void* ScriptWren::GetUserData(ScriptHandle vm)
{
	auto userData = GetUserData((WrenVM*)vm);
	return userData->data;
}

bool ScriptWren::CompileString(ScriptHandle vm, StringView moduleName, StringView string)
{
	BeginModule(moduleName);

	// TODO: Move m_wrenModulesSource to the vm context

	bool success = true;

	auto wrenVm = (WrenVM*)vm;
	auto userData = GetUserData(wrenVm);

	const CString<64> moduleNameStr = moduleName;
	//if (m_wrenModulesSource.find(moduleNameStr) == m_wrenModulesSource.end())
	{
		switch (wrenInterpret(wrenVm, moduleNameStr.c_str(), string.data()))
		{
		case WREN_RESULT_COMPILE_ERROR:
			success = false;
			userData->error = true;
			BX_LOGE(Script, "Wren compilation error on module: {}", moduleNameStr);
			break;

		case WREN_RESULT_RUNTIME_ERROR:
			success = false;
			userData->error = true;
			BX_LOGE(Script, "Wren runtime error on module: {}", moduleNameStr);
			break;

		case WREN_RESULT_SUCCESS:
			userData->initialized = true;
			BX_LOGI(Script, "Wren successfully compiled module: {}", moduleNameStr);
			break;
		}

		//if (success)
		//	m_wrenModulesSource.insert(std::make_pair(moduleNameStr, string.data()));

		if (m_wrenModulesSource.find(moduleNameStr) == m_wrenModulesSource.end())
			m_wrenModulesSource.insert(std::make_pair(moduleNameStr, string.data()));
	}

	EndModule();
	return success;
}

bool ScriptWren::CompileFile(ScriptHandle vm, StringView moduleName, StringView filepath)
{
	auto src = File::Get().ReadText(filepath);
	return CompileString(vm, moduleName, src);
}

bool ScriptWren::HasError(ScriptHandle vm)
{
	//return m_error;
	return false;
}

void ScriptWren::ClearError(ScriptHandle vm)
{
	//m_error = false;
}

ScriptHandle ScriptWren::MakeCallHandle(ScriptHandle vm, StringView signature)
{
	auto wrenVm = (WrenVM*)vm;
	auto handle = wrenMakeCallHandle(wrenVm, signature.data());
	return handle;
}

ScriptHandle ScriptWren::MakeClassHandle(ScriptHandle vm, StringView moduleName, StringView className)
{
	auto wrenVm = (WrenVM*)vm;
	wrenGetVariable(wrenVm, moduleName.data(), className.data(), 0);
	return wrenGetSlotHandle(wrenVm, 0);
}

void ScriptWren::ReleaseHandle(ScriptHandle vm, ScriptHandle function)
{
	wrenReleaseHandle((WrenVM*)vm, (WrenHandle*)function);
}

u64 ScriptWren::GetHandleValue(ScriptHandle handle)
{
	auto wrenHandle = (WrenHandle*)handle;
	return wrenHandle->value;
}

void ScriptWren::EnsureSlots(ScriptHandle vm, i32 numSlots)
{
	wrenEnsureSlots((WrenVM*)vm, numSlots);
}

void ScriptWren::CallFunction(ScriptHandle vm, ScriptHandle handle)
{
	auto wrenVm = (WrenVM*)vm;
	auto wrenHandle = (WrenHandle*)handle;
	wrenCall(wrenVm, wrenHandle);
	//wrenCallFunction();
}

void ScriptWren::BeginModule(StringView moduleName)
{
	m_moduleName = moduleName;
}

StringView ScriptWren::GetCurrentModule() const
{
	return m_moduleName;
}

void ScriptWren::EndModule()
{
	m_moduleName = nullptr;
}

void ScriptWren::BeginClass(StringView className, TypeId typeId)
{
	m_className = className;

	if (m_foreignClassRegistry.find(typeId) == m_foreignClassRegistry.end())
	{
		ScriptClassInfo info;
		info.moduleName = m_moduleName;
		info.className = m_className;
		info.typeId = typeId;

		m_foreignClassRegistry.insert(std::make_pair(typeId, info));
	}
}

StringView ScriptWren::GetCurrentClass() const
{
	return m_className;
}

void ScriptWren::EndClass()
{
	m_className = nullptr;
}

const ScriptClassInfo& ScriptWren::GetClassInfo(TypeId typeId)
{
	auto it = m_foreignClassRegistry.find(typeId);
	BX_ASSERT(it != m_foreignClassRegistry.end(), "Class is not registered!");
	return it->second;
}

void ScriptWren::SetClass(ScriptHandle vm, i32 slot, TypeId typeId)
{
	const auto& info = ScriptWren::GetClassInfo(typeId);

	EnsureSlots(vm, slot + 1);
	wrenGetVariable((WrenVM*)vm, info.moduleName.data(), info.className.data(), slot);

	// TODO: Test this
	//auto it = m_foreignClassRegistry.find(typeId);
	//BX_ASSERT(it != m_foreignClassRegistry.end(), "Class is not registered!");
	//const auto& info = it->second;
	//wrenSetSlotHandle(vm, slot, info.handle);
}

void ScriptWren::BindConstructor(StringView signature, ScriptMethodFn func)
{
	const SizeType hash = GetClassHash(m_moduleName, m_className);
	m_foreignConstructors.insert(std::make_pair(hash, (WrenForeignMethodFn)func));
}

void ScriptWren::BindDestructor(ScriptFinalizerFn func)
{
	const SizeType hash = GetClassHash(m_moduleName, m_className);
	m_foreignDestructors.insert(std::make_pair(hash, (WrenFinalizerFn)func));
}

void ScriptWren::BindFunction(bool isStatic, StringView signature, ScriptMethodFn func)
{
	const SizeType hash = GetMethodHash(m_moduleName, m_className, isStatic, signature);
	m_foreignMethods.insert(std::make_pair(hash, (WrenForeignMethodFn)func));
}

bool ScriptWren::GetSlotBool(ScriptHandle vm, i32 slot)
{
	return wrenGetSlotBool((WrenVM*)vm, slot);
}

#define DECLARE_GET_SLOT_NUM(Num, Name)									\
Num ScriptWren::GetSlot##Name(ScriptHandle vm, i32 slot)				\
{																		\
	return static_cast<Num>(wrenGetSlotDouble((WrenVM*)vm, slot));		\
}

DECLARE_GET_SLOT_NUM(u8, U8)
DECLARE_GET_SLOT_NUM(u16, U16)
DECLARE_GET_SLOT_NUM(u32, U32)
DECLARE_GET_SLOT_NUM(u64, U64)
DECLARE_GET_SLOT_NUM(i8, I8)
DECLARE_GET_SLOT_NUM(i16, I16)
DECLARE_GET_SLOT_NUM(i32, I32)
DECLARE_GET_SLOT_NUM(i64, I64)
DECLARE_GET_SLOT_NUM(f32, F32)
DECLARE_GET_SLOT_NUM(f64, F64)

StringView ScriptWren::GetSlotString(ScriptHandle vm, i32 slot)
{
	return wrenGetSlotString((WrenVM*)vm, slot);
}

void* ScriptWren::GetSlotObject(ScriptHandle vm, i32 slot)
{
	return wrenGetSlotForeign((WrenVM*)vm, slot);
}

ScriptHandle ScriptWren::GetSlotHandle(ScriptHandle vm, i32 slot)
{
	return wrenGetSlotHandle((WrenVM*)vm, slot);
}

i32 ScriptWren::GetListCount(ScriptHandle vm, i32 slot)
{
	return wrenGetListCount((WrenVM*)vm, slot);
}

void ScriptWren::GetListElement(ScriptHandle vm, i32 listSlot, i32 index, i32 elementSlot)
{
	return wrenGetListElement((WrenVM*)vm, listSlot, index, elementSlot);
}

void ScriptWren::SetSlotBool(ScriptHandle vm, i32 slot, bool value)
{
	wrenSetSlotBool((WrenVM*)vm, slot, value);
}

#define DECLARE_SET_SLOT_NUM(Num, Name)									\
void ScriptWren::SetSlot##Name(ScriptHandle vm, i32 slot, Num value)	\
{																		\
	wrenSetSlotDouble((WrenVM*)vm, slot, static_cast<Num>(value));		\
}

// TODO: Wren only supports doubles, int 64s don't pack very well but we have no alternative for now...
DECLARE_SET_SLOT_NUM(u8, U8)
DECLARE_SET_SLOT_NUM(u16, U16)
DECLARE_SET_SLOT_NUM(u32, U32)
DECLARE_SET_SLOT_NUM(u64, U64)
DECLARE_SET_SLOT_NUM(i8, I8)
DECLARE_SET_SLOT_NUM(i16, I16)
DECLARE_SET_SLOT_NUM(i32, I32)
DECLARE_SET_SLOT_NUM(i64, I64)
DECLARE_SET_SLOT_NUM(f32, F32)
DECLARE_SET_SLOT_NUM(f64, F64)

void ScriptWren::SetSlotString(ScriptHandle vm, i32 slot, StringView text)
{
	wrenSetSlotString((WrenVM*)vm, slot, text.data());
}

void* ScriptWren::SetSlotNewObject(ScriptHandle vm, i32 slot, i32 classSlot, SizeType size)
{
	return wrenSetSlotNewForeign((WrenVM*)vm, slot, classSlot, size);
}

void ScriptWren::SetSlotHandle(ScriptHandle vm, i32 slot, ScriptHandle handle)
{
	wrenSetSlotHandle((WrenVM*)vm, slot, (WrenHandle*)handle);
}

void ScriptWren::SetSlotNewList(ScriptHandle vm, i32 slot)
{
	wrenSetSlotNewList((WrenVM*)vm, slot);
}

void ScriptWren::SetListElement(ScriptHandle vm, i32 listSlot, i32 index, i32 elementSlot)
{
	wrenSetListElement((WrenVM*)vm, listSlot, index, elementSlot);
}

void ScriptWren::InsertInList(ScriptHandle vm, i32 listSlot, i32 index, i32 elementSlot)
{
	wrenInsertInList((WrenVM*)vm, listSlot, index, elementSlot);
}