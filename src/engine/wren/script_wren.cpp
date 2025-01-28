#include <engine/wren/script_wren.hpp>

#include <engine/hash.hpp>
#include <engine/macros.hpp>
#include <engine/time.hpp>
//#include <engine/input.hpp>
//#include <engine/data.hpp>
#include <engine/profiler.hpp>
#include <engine/file.hpp>
#include <engine/math.hpp>
//#include <engine/ecs.hpp>
#include <engine/string.hpp>
#include <engine/hash_map.hpp>

#include <engine/audio.hpp>
#include <engine/graphics.hpp>
//#include <engine/physics.hpp>
#include <engine/window.hpp>

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

ScriptVMImpl* ScriptWren::GetVmImpl(WrenVM* vm)
{
	auto userData = wrenGetUserData(vm);
	BX_ENSURE(userData != nullptr);
	return static_cast<ScriptVMImpl*>(userData);
}

static WrenForeignMethodFn WrenBindForeignMethod(WrenVM* vm, const char* moduleName, const char* className, bool isStatic, const char* signature)
{
	auto ctx = ScriptWren::Get().GetVmImpl(vm);

	const StringView moduleNameStr = moduleName;
	const StringView classNameStr = className;
	const StringView signatureStr = signature;

	if (moduleNameStr.compare("random") == 0)
		return nullptr;
	if (moduleNameStr.compare("meta") == 0)
		return nullptr;

	const SizeType hash = GetMethodHash(moduleNameStr, classNameStr, isStatic, signatureStr);
	const auto it = ctx->foreignMethods.find(hash);
	if (it == ctx->foreignMethods.end())
		return nullptr;
	return it->second;
}

static WrenForeignMethodFn WrenAllocate(WrenVM* vm, const SizeType classHash)
{
	auto ctx = ScriptWren::Get().GetVmImpl(vm);

	const auto it = ctx->foreignConstructors.find(classHash);
	if (it != ctx->foreignConstructors.end())
		return it->second;
	return nullptr;
}

static WrenFinalizerFn WrenFinalizer(WrenVM* vm, const SizeType classHash)
{
	auto ctx = ScriptWren::Get().GetVmImpl(vm);

	const auto it = ctx->foreignDestructors.find(classHash);
	if (it != ctx->foreignDestructors.end())
		return it->second;
	return nullptr;
}

static WrenForeignClassMethods WrenBindForeignClass(WrenVM* vm, const char* moduleName, const char* className)
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

static WrenLoadModuleResult WrenLoadModule(WrenVM* vm, const char* moduleName)
{
	auto ctx = ScriptWren::Get().GetVmImpl(vm);

	const CString<64> moduleNameStr = moduleName;

	WrenLoadModuleResult res{};

	if (moduleNameStr.compare("random") == 0)
		return res;
	if (moduleNameStr.compare("meta") == 0)
		return res;

	CString<64> moduleNameFmt{};
	moduleNameFmt.format("{}.wren", moduleNameStr);

	//if (ctx->wrenModulesSource.find(moduleNameStr) == ctx->wrenModulesSource.end())
	//{
	//	String filepath;
	//	if (!File::Find("[assets]", moduleNameFmt, filepath))
	//	{
	//		BX_LOGE(Script, "Module '{}' can not be found!", moduleNameFmt);
	//		return res;
	//	}
	//
	//	String moduleSrc = File::ReadTextFile(filepath);
	//	ctx->wrenModulesSource.insert(std::make_pair(moduleNameStr, moduleSrc));
	//}

	res.source = ctx->wrenModulesSource[moduleNameStr].c_str();
	return res;
}

static void WrenWrite(WrenVM* vm, const char* text)
{
	const StringView textStr = text;
	if (textStr.compare("\n") == 0)
		return;

	BX_LOGI(Script, textStr.data());
}

static void WrenError(WrenVM* vm, WrenErrorType errorType, const char* module, const i32 line, const char* msg)
{
	auto ctx = ScriptWren::Get().GetVmImpl(vm);

	switch (errorType)
	{
	case WREN_ERROR_COMPILE:
		BX_LOGE(Script, "[Compile: {} line {}] {}", module, line, msg); break;
	case WREN_ERROR_STACK_TRACE:
		BX_LOGE(Script, "[StackTrace: {} line {}] in {}", module, line, msg); break;
	case WREN_ERROR_RUNTIME:
		BX_LOGE(Script, "[Runtime] {}", msg); break;
	}

	ctx->error = true;
}

bool ScriptWren::Initialize()
{
	return true;
}

void ScriptWren::Shutdown()
{
}

ScriptVM ScriptWren::CreateVm(const VmInfo& info)
{
	WrenConfiguration config;
	wrenInitConfiguration(&config);
	config.writeFn = WrenWrite;
	config.errorFn = WrenError;
	config.bindForeignClassFn = WrenBindForeignClass;
	config.bindForeignMethodFn = WrenBindForeignMethod;
	config.bindForeignClassFn = WrenBindForeignClass;
	config.loadModuleFn = WrenLoadModule;
	config.initialHeapSize = 1024LL * 1024 * 32;
	config.minHeapSize = 1024LL * 1024 * 16;
	config.heapGrowthPercent = 80;
	auto vm = wrenNewVM(&config);

	auto userData = new ScriptVMImpl();
	userData->vm = vm;

	wrenSetUserData(vm, userData);
	return (ScriptVM)vm;
}

void ScriptWren::DestroyVm(ScriptVM vm)
{
	auto wrenVm = (WrenVM*)vm;
	if (wrenVm)
	{
		wrenFreeVM(wrenVm);
		wrenVm = nullptr;
	}

	auto userData = GetVmImpl(wrenVm);
	userData->wrenModulesSource.clear();
	
	userData->foreignMethods.clear();
	userData->foreignConstructors.clear();
	userData->foreignDestructors.clear();
	
	//impl.foreignClassRegistry.clear();
	userData->wrenTypeIdMap.clear();

	//m_resourceClassWrappers.clear();
	//m_componentClassWrappers.clear();
}

static void WrenCompile(WrenVM* vm, StringView moduleName, StringView moduleSrc)
{
	auto ctx = ScriptWren::Get().GetVmImpl(vm);

	const CString<64> moduleNameStr = moduleName;
	if (ctx->wrenModulesSource.find(moduleNameStr) == ctx->wrenModulesSource.end())
	{
		switch (wrenInterpret(vm, moduleNameStr.c_str(), moduleSrc.data()))
		{
		case WREN_RESULT_COMPILE_ERROR:
			ctx->error = true;
			BX_LOGE(Script, "Wren compilation error on module: {}", moduleNameStr);
			break;

		case WREN_RESULT_RUNTIME_ERROR:
			ctx->error = true;
			BX_LOGE(Script, "Wren runtime error on module: {}", moduleNameStr);
			break;

		case WREN_RESULT_SUCCESS:
			ctx->initialized = true;
			BX_LOGI(Script, "Wren successfully compiled module: {}", moduleNameStr);
			break;
		}

		ctx->wrenModulesSource.insert(std::make_pair(moduleNameStr, moduleSrc.data()));
	}
}

#if defined DEBUG_BUILD || defined EDITOR_BUILD
#define LOAD_ENGINE_MODULE(ModuleName) { String src = File::ReadTextFile(ENGINE_PATH"/wren/"#ModuleName".wren"); WrenCompile(m_vm, #ModuleName, src.c_str()); }

#else
extern "C" {
#include "core_wren.h"
#include "device_wren.h"
#include "math_wren.h"
#include "graphicm_wren.h"
#include "physicm_wren.h"
#include "audio_wren.h"
#include "ecm_wren.h"
#include "framework_wren.h"
}

#define LOAD_ENGINE_MODULE(ModuleName) { WrenCompile(m_vm, #ModuleName, ModuleName##_wren_data); }
#endif

void ScriptWren::ConfigureApi(ScriptVM vm)
{
	//LOAD_ENGINE_MODULE(core);
	//LOAD_ENGINE_MODULE(device);
	//LOAD_ENGINE_MODULE(math);
	//LOAD_ENGINE_MODULE(graphics);
	//LOAD_ENGINE_MODULE(physics);
	//LOAD_ENGINE_MODULE(audio);
	//LOAD_ENGINE_MODULE(ecs);
	//LOAD_ENGINE_MODULE(framework);
	//
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
	//
	//if (m_initialized && !m_error)
	//{
	//	wrenEnsureSlots(m_vm, 1);
	//	wrenGetVariable(m_vm, "game", "Game", 0);
	//	m_gameClass = wrenGetSlotHandle(m_vm, 0);
	//	wrenSetSlotHandle(m_vm, 0, m_gameClass);
	//
	//	m_configMethod = wrenMakeCallHandle(m_vm, "config()");
	//	m_initMethod = wrenMakeCallHandle(m_vm, "init()");
	//	m_updateMethod = wrenMakeCallHandle(m_vm, "update()");
	//	m_renderMethod = wrenMakeCallHandle(m_vm, "render()");
	//
	//	m_goNewMethod = wrenMakeCallHandle(m_vm, "new(_)");
	//	m_goEntityMethod = wrenMakeCallHandle(m_vm, "entity");
	//	m_goStartMethod = wrenMakeCallHandle(m_vm, "start()");
	//	m_goUpdateMethod = wrenMakeCallHandle(m_vm, "update()");
	//
	//	wrenCall(m_vm, m_configMethod);
	//}
}

void ScriptWren::CollectGarbage(ScriptVM vm)
{
	//wrenCollectGarbage(m_vm);
}

bool ScriptWren::HasError(ScriptVM vm)
{
	//return m_error;
	return false;
}

void ScriptWren::ClearError(ScriptVM vm)
{
	//m_error = false;
}

void ScriptWren::BindVm(ScriptVM vm)
{
	if (vm == nullptr)
		m_boundVm = nullptr;
	m_boundVm = GetVmImpl((WrenVM*)vm);
}

void ScriptWren::BeginModule(StringView moduleName)
{
	m_moduleName = moduleName;
}

void ScriptWren::EndModule()
{
	m_moduleName = nullptr;
}

void ScriptWren::BeginClass(StringView className)
{
	m_className = className;
}

void ScriptWren::EndClass()
{
	m_className = nullptr;
}

void ScriptWren::BindFunction(bool isStatic, StringView signature, ScriptMethodFn func)
{
	const SizeType hash = GetMethodHash(m_moduleName, m_className, isStatic, signature);
	m_boundVm->foreignMethods.insert(std::make_pair(hash, (WrenForeignMethodFn)func));
}

//void ScriptWren::RegisterResourceClass(TypeId typeId, const ResourceClassWrapper& wrapper)
//{
//	m_resourceClassWrappers.insert(std::make_pair(typeId, wrapper));
//}
//
//static void GetResourceCallInfo(WrenVM* vm, ResourceClassWrapper& wrapper)
//{
//	WrenHandle* handle = wrenGetSlotHandle(vm, 1);
//	ObjClass* objClass = wrenGetClass(vm, handle->value);
//
//	auto it = m_wrenTypeIdMap.find(objClass->name->hash);
//	BX_ASSERT(it != m_wrenTypeIdMap.end(), "Class is not registered!");
//	const auto& typeId = it->second;
//
//	auto it2 = m_resourceClassWrappers.find(typeId);
//	BX_ASSERT(it2 != m_resourceClassWrappers.end(), "Resource is not registered!");
//	wrapper = it2->second;
//}
//
//static void ResourceCreate(WrenVM* vm)
//{
//	ResourceClassWrapper wrapper;
//	GetResourceCallInfo(vm, wrapper);
//	wrapper.createFn(vm);
//}
//
//static void GetResourceCallInfo(WrenVM* vm, MetaResource& res, ResourceClassWrapper& wrapper)
//{
//	wrenEnsureSlots(vm, 2);
//	res = ScriptArg<MetaResource>::Get(vm, 0);
//
//	WrenHandle* handle = wrenGetSlotHandle(vm, 1);
//	ObjClass* objClass = wrenGetClass(vm, handle->value);
//
//	auto it = m_wrenTypeIdMap.find(objClass->name->hash);
//	BX_ASSERT(it != m_wrenTypeIdMap.end(), "Class is not registered!");
//	const auto& typeId = it->second;
//
//	auto it2 = m_resourceClassWrappers.find(typeId);
//	BX_ASSERT(it2 != m_resourceClassWrappers.end(), "Resource is not registered!");
//	wrapper = it2->second;
//}
//
//static void ResourceGetData(WrenVM* vm)
//{
//	MetaResource res;
//	ResourceClassWrapper wrapper;
//	GetResourceCallInfo(vm, res, wrapper);
//	wrapper.getDataFn(vm, res);
//}
//
//void ScriptWren::RegisterComponentClass(TypeId typeId, const ComponentClassWrapper& wrapper)
//{
//	m_componentClassWrappers.insert(std::make_pair(typeId, wrapper));
//}
//
//static void GetComponentCallInfo(WrenVM* vm, Entity& entity, ComponentClassWrapper& wrapper)
//{
//	wrenEnsureSlots(vm, 2);
//	entity = ScriptArg<Entity&>::Get(vm, 0);
//
//	WrenHandle* handle = wrenGetSlotHandle(vm, 1);
//	ObjClass* objClass = wrenGetClass(vm, handle->value);
//
//	auto it = m_wrenTypeIdMap.find(objClass->name->hash);
//	BX_ASSERT(it != m_wrenTypeIdMap.end(), "Class is not registered!");
//	const auto& typeId = it->second;
//
//	auto it2 = m_componentClassWrappers.find(typeId);
//	BX_ASSERT(it2 != m_componentClassWrappers.end(), "Component is not registered!");
//	wrapper = it2->second;
//}
//
//static void EntityHasComponent(WrenVM* vm)
//{
//	Entity entity;
//	ComponentClassWrapper wrapper;
//	GetComponentCallInfo(vm, entity, wrapper);
//	wrapper.hasComponentFn(vm, entity);
//}
//
//static void EntityAddComponent(WrenVM* vm)
//{
//	Entity entity;
//	ComponentClassWrapper wrapper;
//	GetComponentCallInfo(vm, entity, wrapper);
//	wrapper.addComponentFn(vm, entity);
//}
//
//static void EntityGetComponent(WrenVM* vm)
//{
//	Entity entity;
//	ComponentClassWrapper wrapper;
//	GetComponentCallInfo(vm, entity, wrapper);
//	wrapper.getComponentFn(vm, entity);
//}
//
//static void EntityRemoveComponent(WrenVM* vm)
//{
//	Entity entity;
//	ComponentClassWrapper wrapper;
//	GetComponentCallInfo(vm, entity, wrapper);
//	wrapper.removeComponentFn(vm, entity);
//}
//
//static void RegisterGameObject(WrenVM* vm)
//{
//	WrenHandle* classHandle = wrenGetSlotHandle(vm, 1);
//	ObjClass* classObj = wrenGetClass(vm, classHandle->value);
//
//	String className = classObj->name->value;
//	className = className.substr(0, className.find(" "));
//
//	GameObjectMetaData metaData
//	{
//		[vm, classHandle]()
//		{
//			wrenSetSlotHandle(vm, 0, classHandle);
//		},
//		[vm](const GameObjectData& data)
//		{
//			ScriptArg<const GameObjectData&>::Set(vm, 1, data);
//			auto result = wrenCall(vm, m_goNewMethod);
//			return result == WREN_RESULT_SUCCESS;
//		}
//	};
//
//	GameObject::Register(className, metaData);
//}
//
//static void GetTypeGameObjectData(WrenVM* vm)
//{
//	wrenEnsureSlots(vm, 0);
//	const auto& data = ScriptArg<const GameObjectData&>::Get(vm, 0);
//	const auto& metaData = GameObject::GetClassMetaData(data.className);
//	metaData.bindClassFn();
//}
//
//static void CreateGameObjectData(WrenVM* vm)
//{
//	wrenEnsureSlots(vm, 1);
//	WrenHandle* objHandle = wrenGetSlotHandle(vm, 1);
//	ObjClass* classObj = wrenGetClass(vm, objHandle->value);
//	String className = classObj->name->value;
//	className = className.substr(0, className.find(" "));
//
//	GameObjectData data{ className, className, Entity::Invalid() };
//	ScriptArg<GameObjectData>::Set(vm, 0, data);
//}
//
//static void ConstructGameObjectBase(WrenVM* vm)
//{
//	wrenEnsureSlots(vm, 2);
//
//	const auto& data = ScriptArg<const GameObjectData&>::Get(vm, 1);
//
//	WrenHandle* objHandle = wrenGetSlotHandle(vm, 2);
//	ObjClass* classObj = wrenGetClass(vm, objHandle->value);
//	String className = classObj->name->value;
//	className = className.substr(0, className.find(" "));
//
//	void* memory = ScriptArg<void*>::Set(vm, 0, 0, sizeof(ScriptObjVal<GameObjectBase>));
//	ScriptObjVal<GameObjectBase>* obj = new (memory) ScriptObjVal<GameObjectBase>{};
//
//	new (obj->Ptr()) GameObjectBase(
//		Scene::GetCurrent(), data,
//		[vm, objHandle]()
//		{
//			wrenSetSlotHandle(vm, 0, objHandle);
//		},
//		[vm, objHandle]()
//		{
//			//auto fiber = wrenNewFiber(vm, NULL);
//			//wrenSetSlotHandle(vm, 0, objHandle);
//			//wrenCallFunction(vm, fiber, AS_CLOSURE(m_goStartMethod->value), 0);
//
//			wrenSetSlotHandle(vm, 0, objHandle);
//			wrenCall(vm, m_goStartMethod);
//		},
//		[vm, objHandle]()
//		{
//			wrenSetSlotHandle(vm, 0, objHandle);
//			wrenCall(vm, m_goUpdateMethod);
//		});
//}
//
//static void FinalizeGameObjectBase(void* data)
//{
//	ScriptObj* obj = static_cast<ScriptObj*>(data);
//	obj->~ScriptObj();
//}

void ScriptWren::EnsureSlots(ScriptVM vm, i32 numSlots)
{
	wrenEnsureSlots((WrenVM*)vm, numSlots);
}

bool ScriptWren::GetSlotBool(ScriptVM vm, i32 slot)
{
	return wrenGetSlotBool((WrenVM*)vm, slot);
}

double ScriptWren::GetSlotDouble(ScriptVM vm, i32 slot)
{
	return wrenGetSlotDouble((WrenVM*)vm, slot);
}

StringView ScriptWren::GetSlotString(ScriptVM vm, i32 slot)
{
	return wrenGetSlotString((WrenVM*)vm, slot);
}

void* ScriptWren::GetSlotObject(ScriptVM vm, i32 slot)
{
	return wrenGetSlotForeign((WrenVM*)vm, slot);
}

void ScriptWren::SetSlotBool(ScriptVM vm, i32 slot, bool value)
{
	wrenSetSlotBool((WrenVM*)vm, slot, value);
}

void ScriptWren::SetSlotDouble(ScriptVM vm, i32 slot, f64 value)
{
	wrenSetSlotDouble((WrenVM*)vm, slot, value);
}

void ScriptWren::SetSlotString(ScriptVM vm, i32 slot, StringView text)
{
	wrenSetSlotString((WrenVM*)vm, slot, text.data());
}

void* ScriptWren::SetSlotNewObject(ScriptVM vm, i32 slot, i32 classSlot, SizeType size)
{
	wrenSetSlotNewForeign((WrenVM*)vm, slot, classSlot, size);
}

void ScriptWren::RegisterClass(TypeId typeId)
{
	if (m_boundVm->foreignClassRegistry.find(typeId) == m_boundVm->foreignClassRegistry.end())
	{
		ScriptClassInfo info;
		info.moduleName = m_moduleName;
		info.className = m_className;
		info.typeId = typeId;
	
		m_boundVm->foreignClassRegistry.insert(std::make_pair(typeId, info));
	}
}

const ScriptClassInfo& ScriptWren::GetClassInfo(TypeId typeId)
{
	auto it = m_boundVm->foreignClassRegistry.find(typeId);
	BX_ASSERT(it != m_boundVm->foreignClassRegistry.end(), "Class is not registered!");
	return it->second;
}

void ScriptWren::SetClass(ScriptVM vm, i32 slot, TypeId typeId)
{
	const auto& info = ScriptWren::GetClassInfo(typeId);

	EnsureSlots(vm, slot + 1);
	//wrenGetVariable(vm, info.moduleName.data(), info.className.data(), slot);

	// TODO: Test this
	//auto it = m_foreignClassRegistry.find(typeId);
	//BX_ASSERT(it != m_foreignClassRegistry.end(), "Class is not registered!");
	//const auto& info = it->second;
	//wrenSetSlotHandle(vm, slot, info.handle);
}

void ScriptWren::RegisterConstructor(SizeType arity, StringView signature, ScriptMethodFn func)
{
	const SizeType hash = GetClassHash(m_moduleName, m_className);
	m_boundVm->foreignConstructors.insert(std::make_pair(hash, (WrenForeignMethodFn)func));
}

void ScriptWren::RegisterDestructor(ScriptFinalizerFn func)
{
	const SizeType hash = GetClassHash(m_moduleName, m_className);
	m_boundVm->foreignDestructors.insert(std::make_pair(hash, (WrenFinalizerFn)func));
}