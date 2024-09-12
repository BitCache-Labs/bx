#include "bx/engine/modules/script.hpp"

#include "bx/engine/core/macros.hpp"
#include "bx/engine/core/time.hpp"
#include "bx/engine/core/input.hpp"
#include "bx/engine/core/data.hpp"
#include "bx/engine/core/profiler.hpp"
#include "bx/engine/core/file.hpp"
#include "bx/engine/core/math.hpp"
#include "bx/engine/core/ecs.hpp"
#include "bx/engine/containers/string.hpp"
#include "bx/engine/containers/hash_map.hpp"

#include "bx/engine/modules/audio.hpp"
#include "bx/engine/modules/graphics.hpp"
#include "bx/engine/modules/physics.hpp"
#include "bx/engine/modules/window.hpp"

// TODO: Framework bindings should be handled outside of Engine
#include <bx/framework/components/animator.hpp>
#include <bx/framework/components/attributes.hpp>
#include <bx/framework/components/audio_listener.hpp>
#include <bx/framework/components/audio_source.hpp>
#include <bx/framework/components/camera.hpp>
#include <bx/framework/components/character_controller.hpp>
#include <bx/framework/components/collider.hpp>
#include <bx/framework/components/light.hpp>
#include <bx/framework/components/mesh_filter.hpp>
#include <bx/framework/components/mesh_renderer.hpp>
#include <bx/framework/components/rigidbody.hpp>
#include <bx/framework/components/spline.hpp>
#include <bx/framework/components/transform.hpp>
#include <bx/framework/systems/renderer.hpp>
#include <bx/framework/systems/dynamics.hpp>
#include <bx/framework/systems/acoustics.hpp>
#include <bx/framework/gameobject.hpp>

#include <cstdio>
#include <cstring>
#include <functional>

extern "C" {
#include <wren.h>
#include <wren_vm.h>
#include <wren_opt_meta.h>
#include <wren_opt_random.h>
}

// TODO: wrenEnsureSlots is not being used correctly, assumption was that the function would
// throw an error if num slots wasn't correct, should use ENGINE_ASSERT and wrenGetSlotCount.

// Wren VM state handles
static WrenVM* s_vm;
static WrenHandle* s_goNewMethod = nullptr;
static WrenHandle* s_goStartMethod = nullptr;
static WrenHandle* s_goUpdateMethod = nullptr;

static List<BindApiFn> g_bindApis;

static bool s_initialized = false;
static bool s_error = false;

// Wren binding context state
static const char* s_moduleName = nullptr;
static const char* s_className = nullptr;

// Wren storage for module code sources
static HashMap<String, String> s_wrenModulesSource;

// Reflection mappings utility
static HashMap<SizeType, WrenForeignMethodFn> s_foreignMethods;
static HashMap<SizeType, WrenForeignMethodFn> s_foreignConstructors;
static HashMap<SizeType, WrenFinalizerFn> s_foreignDestructors;

static HashMap<TypeId, ScriptClassInfo> s_foreignClassRegistry;
static HashMap<u32, TypeId> s_wrenTypeIdMap;

// Extra mappings for specialized template classes
//static HashMap<TypeId, ResourceClassWrapper> s_resourceClassWrappers;
static HashMap<TypeId, ComponentClassWrapper> s_componentClassWrappers;

// Helper functions and Wren callbacks
static SizeType GetClassHash(const char* moduleName, const char* className)
{
	static std::hash<String> s_strHash;
	return s_strHash(moduleName)
		^ s_strHash(className);
}

static SizeType GetMethodHash(const char* moduleName, const char* className, bool isStatic, const char* signature)
{
	static std::hash<String> s_strHash;
	return s_strHash(moduleName)
		^ s_strHash(className)
		^ s_strHash(isStatic ? "s" : "")
		^ s_strHash(signature);
}

static WrenForeignMethodFn WrenBindForeignMethod(WrenVM* vm, const char* moduleName, const char* className, bool isStatic, const char* signature)
{
	if (std::strcmp(moduleName, "random") == 0)
		return nullptr;
	if (std::strcmp(moduleName, "meta") == 0)
		return nullptr;

	const SizeType hash = GetMethodHash(moduleName, className, isStatic, signature);
	const auto it = s_foreignMethods.find(hash);
	if (it == s_foreignMethods.end())
		return nullptr;
	return it->second;
}

static WrenForeignMethodFn WrenAllocate(const SizeType classHash)
{
	const auto it = s_foreignConstructors.find(classHash);
	if (it != s_foreignConstructors.end())
		return it->second;
	return nullptr;
}

static WrenFinalizerFn WrenFinalizer(const SizeType classHash)
{
	const auto it = s_foreignDestructors.find(classHash);
	if (it != s_foreignDestructors.end())
		return it->second;
	return nullptr;
}

static WrenForeignClassMethods WrenBindForeignClass(WrenVM* vm, const char* moduleName, const char* className)
{
	WrenForeignClassMethods methods{};
	methods.allocate = nullptr;
	methods.finalize = nullptr;

	if (std::strcmp(moduleName, "random") == 0) return methods;
	if (std::strcmp(moduleName, "meta") == 0) return methods;

	const SizeType classHash = GetClassHash(moduleName, className);
	methods.allocate = WrenAllocate(classHash);
	methods.finalize = WrenFinalizer(classHash);

	return methods;
}

static WrenLoadModuleResult WrenLoadModule(WrenVM* vm, const char* name)
{
	WrenLoadModuleResult res{};

	if (strcmp(name, "meta") == 0)
		return res;
	if (strcmp(name, "random") == 0)
		return res;

	String moduleName = name;
	if (s_wrenModulesSource.find(moduleName) == s_wrenModulesSource.end())
	{
		String filepath;
		if (!File::Find("[assets]", String(name) + ".wren", filepath))
		{
			BX_LOGE("Module '{}' can not be found!", name);
			return res;
		}

		String moduleSrc = File::ReadTextFile(filepath);
		s_wrenModulesSource.insert(std::make_pair(moduleName, moduleSrc));
	}

	res.source = s_wrenModulesSource[moduleName].c_str();
	return res;
}

static void WrenWrite(WrenVM* vm, const char* text)
{
	if (strcmp(text, "\n") == 0)
		return;

	Log::Print(LogLevel::LOG_INFO, text);
}

static void WrenError(WrenVM* vm, WrenErrorType errorType, const char* module, const i32 line, const char* msg)
{
	switch (errorType)
	{
	case WREN_ERROR_COMPILE:
		BX_LOGE("[Compile: {} line {}] {}", module, line, msg); break;
	case WREN_ERROR_STACK_TRACE:
		BX_LOGE("[StackTrace: {} line {}] in {}", module, line, msg); break;
	case WREN_ERROR_RUNTIME:
		BX_LOGE("[Runtime] {}", msg); break;
	}

	s_error = true;
}

static void CreateVm()
{
	s_initialized = false;
	s_error = false;

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
	s_vm = wrenNewVM(&config);
}

static void DestroyVm()
{
	if (s_vm)
	{
		// TODO: Release these handles?
		//static WrenHandle* s_goNewMethod = nullptr;
		//static WrenHandle* s_goEntityMethod = nullptr;
		//static WrenHandle* s_goStartMethod = nullptr;
		//static WrenHandle* s_goUpdateMethod = nullptr;
		//if (s_gameClass)
		//{
		//	wrenReleaseHandle(s_vm, s_gameClass);
		//	s_gameClass = nullptr;
		//}

		wrenFreeVM(s_vm);
		s_vm = nullptr;
	}

	s_wrenModulesSource.clear();
	
	s_foreignMethods.clear();
	s_foreignConstructors.clear();
	s_foreignDestructors.clear();

	s_foreignClassRegistry.clear();
	s_wrenTypeIdMap.clear();

	//s_resourceClassWrappers.clear();
	s_componentClassWrappers.clear();
}

static void WrenCompile(WrenVM* vm, const char* name, const char* src)
{
	String moduleName = name;
	if (s_wrenModulesSource.find(moduleName) == s_wrenModulesSource.end())
	{
		switch (wrenInterpret(vm, name, src))
		{
		case WREN_RESULT_COMPILE_ERROR:
			s_error = true;
			BX_LOGE("Wren compilation error on module: {}", moduleName);
			break;

		case WREN_RESULT_RUNTIME_ERROR:
			s_error = true;
			BX_LOGE("Wren runtime error on module: {}", moduleName);
			break;

		case WREN_RESULT_SUCCESS:
			s_initialized = true;
			BX_LOGI("Wren successfully compiled module: {}", moduleName);
			break;
		}

		String moduleSrc = src;
		s_wrenModulesSource.insert(std::make_pair(moduleName, moduleSrc));
	}
}

#if defined BX_DEBUG_BUILD || defined BX_EDITOR_BUILD
#define LOAD_ENGINE_MODULE(ModuleName) { String src = File::ReadTextFile(BX_PATH"/wren/"#ModuleName".wren"); WrenCompile(s_vm, #ModuleName, src.c_str()); }

#else
extern "C" {
#include "core_wren.h"
#include "device_wren.h"
#include "math_wren.h"
#include "graphics_wren.h"
#include "physics_wren.h"
#include "audio_wren.h"
#include "ecs_wren.h"
#include "framework_wren.h"
}

#define LOAD_ENGINE_MODULE(ModuleName) { WrenCompile(s_vm, #ModuleName, ModuleName##_wren_data); }
#endif

static void Configure()
{
	LOAD_ENGINE_MODULE(core);
	LOAD_ENGINE_MODULE(device);
	LOAD_ENGINE_MODULE(math);
	LOAD_ENGINE_MODULE(graphics);
	LOAD_ENGINE_MODULE(physics);
	LOAD_ENGINE_MODULE(audio);
	LOAD_ENGINE_MODULE(ecs);
	LOAD_ENGINE_MODULE(framework);

	File::FindEach("[assets]", ".wren",
		[](const String& path, const String& name)
		{
			WrenCompile(s_vm, name.c_str(), File::ReadTextFile(path).c_str());
		});

	for (auto& it : s_foreignClassRegistry)
	{
		auto& info = it.second;

		wrenEnsureSlots(s_vm, 1);
		wrenGetVariable(s_vm, info.moduleName, info.className, 0);
		info.handle = wrenGetSlotHandle(s_vm, 0);
		auto objClass = wrenGetClass(s_vm, info.handle->value);

		s_wrenTypeIdMap.insert(std::make_pair(objClass->name->hash, info.typeId));
	}

	if (s_initialized && !s_error)
	{
		//wrenEnsureSlots(s_vm, 1);
		//wrenGetVariable(s_vm, "game", "Game", 0);
		//s_gameClass = wrenGetSlotHandle(s_vm, 0);
		//wrenSetSlotHandle(s_vm, 0, s_gameClass);
		//
		//s_configMethod = wrenMakeCallHandle(s_vm, "config()");
		//s_initMethod = wrenMakeCallHandle(s_vm, "init()");
		//s_updateMethod = wrenMakeCallHandle(s_vm, "update()");
		//s_renderMethod = wrenMakeCallHandle(s_vm, "render()");

		s_goNewMethod = wrenMakeCallHandle(s_vm, "new(_)");
		s_goStartMethod = wrenMakeCallHandle(s_vm, "start()");
		s_goUpdateMethod = wrenMakeCallHandle(s_vm, "update()");

		//wrenCall(s_vm, s_configMethod);
	}
}

void Script::RegisterApi(BindApiFn bindApi)
{
	g_bindApis.emplace_back(bindApi);
}

bool Script::HasError()
{
	return s_error;
}

void Script::ClearError()
{
	s_error = false;
}

void Script::BeginModule(const char* moduleName)
{
	s_moduleName = moduleName;
}

void Script::EndModule()
{
	s_moduleName = nullptr;
}

void Script::BeginClass(const char* className)
{
	s_className = className;
}

void Script::EndClass()
{
	s_className = nullptr;
}

//void Script::RegisterResourceClass(TypeId typeId, const ResourceClassWrapper& wrapper)
//{
//	s_resourceClassWrappers.insert(std::make_pair(typeId, wrapper));
//}
//
//static void GetResourceCallInfo(WrenVM* vm, ResourceClassWrapper& wrapper)
//{
//	WrenHandle* handle = wrenGetSlotHandle(vm, 1);
//	ObjClass* objClass = wrenGetClass(vm, handle->value);
//
//	auto it = s_wrenTypeIdMap.find(objClass->name->hash);
//	ENGINE_ASSERT(it != s_wrenTypeIdMap.end(), "Class is not registered!");
//	const auto& typeId = it->second;
//
//	auto it2 = s_resourceClassWrappers.find(typeId);
//	ENGINE_ASSERT(it2 != s_resourceClassWrappers.end(), "Resource is not registered!");
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
//	auto it = s_wrenTypeIdMap.find(objClass->name->hash);
//	ENGINE_ASSERT(it != s_wrenTypeIdMap.end(), "Class is not registered!");
//	const auto& typeId = it->second;
//
//	auto it2 = s_resourceClassWrappers.find(typeId);
//	ENGINE_ASSERT(it2 != s_resourceClassWrappers.end(), "Resource is not registered!");
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

void Script::RegisterComponentClass(TypeId typeId, const ComponentClassWrapper& wrapper)
{
	s_componentClassWrappers.insert(std::make_pair(typeId, wrapper));
}

static void GetComponentCallInfo(WrenVM* vm, Entity& entity, ComponentClassWrapper& wrapper)
{
	wrenEnsureSlots(vm, 2);
	entity = ScriptArg<Entity&>::Get(vm, 0);

	WrenHandle* handle = wrenGetSlotHandle(vm, 1);
	ObjClass* objClass = wrenGetClass(vm, handle->value);

	auto it = s_wrenTypeIdMap.find(objClass->name->hash);
	BX_ASSERT(it != s_wrenTypeIdMap.end(), "Class is not registered!");
	const auto& typeId = it->second;

	auto it2 = s_componentClassWrappers.find(typeId);
	BX_ASSERT(it2 != s_componentClassWrappers.end(), "Component is not registered!");
	wrapper = it2->second;
}

static void EntityHasComponent(WrenVM* vm)
{
	Entity entity;
	ComponentClassWrapper wrapper;
	GetComponentCallInfo(vm, entity, wrapper);
	wrapper.hasComponentFn(vm, entity);
}

static void EntityAddComponent(WrenVM* vm)
{
	Entity entity;
	ComponentClassWrapper wrapper;
	GetComponentCallInfo(vm, entity, wrapper);
	wrapper.addComponentFn(vm, entity);
}

static void EntityGetComponent(WrenVM* vm)
{
	Entity entity;
	ComponentClassWrapper wrapper;
	GetComponentCallInfo(vm, entity, wrapper);
	wrapper.getComponentFn(vm, entity);
}

static void EntityRemoveComponent(WrenVM* vm)
{
	Entity entity;
	ComponentClassWrapper wrapper;
	GetComponentCallInfo(vm, entity, wrapper);
	wrapper.removeComponentFn(vm, entity);
}

static void RegisterGameObject(WrenVM* vm)
{
	WrenHandle* classHandle = wrenGetSlotHandle(vm, 1);
	ObjClass* classObj = wrenGetClass(vm, classHandle->value);

	String className = classObj->name->value;
	className = className.substr(0, className.find(" "));

	GameObjectMetaData metaData
	{
		[vm, classHandle]()
		{
			wrenSetSlotHandle(vm, 0, classHandle);
		},
		[vm](const GameObjectData& data)
		{
			ScriptArg<const GameObjectData&>::Set(vm, 1, data);
			auto result = wrenCall(vm, s_goNewMethod);
			return result == WREN_RESULT_SUCCESS;
		}
	};

	GameObject::Register(className, metaData);
}

static void GetTypeGameObjectData(WrenVM* vm)
{
	wrenEnsureSlots(vm, 0);
	const auto& data = ScriptArg<const GameObjectData&>::Get(vm, 0);
	const auto& metaData = GameObject::GetClassMetaData(data.className);
	metaData.bindClassFn();
}

static void CreateGameObjectData(WrenVM* vm)
{
	wrenEnsureSlots(vm, 1);
	WrenHandle* objHandle = wrenGetSlotHandle(vm, 1);
	ObjClass* classObj = wrenGetClass(vm, objHandle->value);
	String className = classObj->name->value;
	className = className.substr(0, className.find(" "));

	GameObjectData data{ className, className, Entity::Invalid() };
	ScriptArg<GameObjectData>::Set(vm, 0, data);
}

static void ConstructGameObjectBase(WrenVM* vm)
{
	wrenEnsureSlots(vm, 2);

	const auto& data = ScriptArg<const GameObjectData&>::Get(vm, 1);

	WrenHandle* objHandle = wrenGetSlotHandle(vm, 2);
	ObjClass* classObj = wrenGetClass(vm, objHandle->value);
	String className = classObj->name->value;
	className = className.substr(0, className.find(" "));

	void* memory = ScriptArg<void*>::Set(vm, 0, 0, sizeof(ScriptObjVal<GameObjectBase>));
	ScriptObjVal<GameObjectBase>* obj = new (memory) ScriptObjVal<GameObjectBase>{};

	new (obj->Ptr()) GameObjectBase(
		Scene::GetCurrent(), data,
		[vm, objHandle]()
		{
			wrenSetSlotHandle(vm, 0, objHandle);
		},
		[vm, objHandle]()
		{
			//auto fiber = wrenNewFiber(vm, NULL);
			//wrenSetSlotHandle(vm, 0, objHandle);
			//wrenCallFunction(vm, fiber, AS_CLOSURE(s_goStartMethod->value), 0);

			wrenSetSlotHandle(vm, 0, objHandle);
			wrenCall(vm, s_goStartMethod);
		},
		[vm, objHandle]()
		{
			wrenSetSlotHandle(vm, 0, objHandle);
			wrenCall(vm, s_goUpdateMethod);
		});
}

static void FinalizeGameObjectBase(void* data)
{
	ScriptObj* obj = static_cast<ScriptObj*>(data);
	obj->~ScriptObj();
}

static void BindApi()
{
	Script::BeginModule("core");
	{
		Script::BeginClass("Time");
		{
			Script::BindFunction<decltype(&Time::GetTime), &Time::GetTime>(true, "time");
			Script::BindFunction<decltype(&Time::GetDeltaTime), &Time::GetDeltaTime>(true, "deltaTime");
		}
		Script::EndClass();

		Script::BeginClass<DataTarget, i32>("DataTarget");
		{
			Script::BindEnumVal<DataTarget, i32, DataTarget::NONE>("none");
			Script::BindEnumVal<DataTarget, i32, DataTarget::SYSTEM>("system");
			Script::BindEnumVal<DataTarget, i32, DataTarget::DEBUG>("debug");
			Script::BindEnumVal<DataTarget, i32, DataTarget::GAME>("game");
			Script::BindEnumVal<DataTarget, i32, DataTarget::PLAYER>("player");
		}
		Script::EndClass();

		Script::BeginClass("Data");
		{
			Script::BindFunction<decltype(&Data::GetBool), &Data::GetBool>(true, "getBool(_,_,_)");
			Script::BindFunction<decltype(&Data::GetInt), &Data::GetInt>(true, "getInt(_,_,_)");
			Script::BindFunction<decltype(&Data::GetUInt), &Data::GetUInt>(true, "getUInt(_,_,_)");
			Script::BindFunction<decltype(&Data::GetFloat), &Data::GetFloat>(true, "getFloat(_,_,_)");
			Script::BindFunction<decltype(&Data::GetDouble), &Data::GetDouble>(true, "getDouble(_,_,_)");
			Script::BindFunction<decltype(&Data::GetString), &Data::GetString>(true, "getString(_,_,_)");

			Script::BindFunction<decltype(&Data::SetBool), &Data::SetBool>(true, "setBool(_,_,_)");
			Script::BindFunction<decltype(&Data::SetInt), &Data::SetInt>(true, "setInt(_,_,_)");
			Script::BindFunction<decltype(&Data::SetUInt), &Data::SetUInt>(true, "setUInt(_,_,_)");
			Script::BindFunction<decltype(&Data::SetFloat), &Data::SetFloat>(true, "setFloat(_,_,_)");
			Script::BindFunction<decltype(&Data::SetDouble), &Data::SetDouble>(true, "setDouble(_,_,_)");
			Script::BindFunction<decltype(&Data::SetString), &Data::SetString>(true, "setString(_,_,_)");
		}
		Script::EndClass();

		Script::BeginClass<Timer>("Timer");
		{
			Script::BindFunction<decltype(&Timer::Start), &Timer::Start>(false, "start()");
			Script::BindFunction<decltype(&Timer::Elapsed), &Timer::Elapsed>(false, "elapsed()");
		}
		Script::EndClass();

		Script::BeginClass("File");
		{
			Script::BindFunction<decltype(&File::ReadTextFile), &File::ReadTextFile>(true, "readTextFile(_)");
			Script::BindFunction<decltype(&File::WriteTextFile), &File::WriteTextFile>(true, "writeTextFile(_,_)");
			Script::BindFunction<decltype(&File::Exists), &File::Exists>(true, "exists(_)");
		}
		Script::EndClass();

		//Script::BeginClass<MetaResource>("Resource");
		//{
		//	Script::BindCFunction(true, "create(_,_)", ResourceCreate);
		//	Script::BindCFunction(false, "getData(_)", ResourceGetData);
		//
		//	Script::BindFunction<decltype(&MetaResource::GetHandle), &MetaResource::GetHandle>(false, "handle");
		//	Script::BindFunction<decltype(&MetaResource::IsValid), &MetaResource::IsValid>(false, "isValid");
		//	Script::BindFunction<decltype(&MetaResource::IsLoaded), &MetaResource::IsLoaded>(false, "isLoaded");
		//	Script::BindFunction<decltype(&MetaResource::Load), &MetaResource::Load>(false, "load(_)");
		//	//Script::BindFunction<decltype(&MetaResource::LoadData), &MetaResource::LoadData>(false, "loadData(_,_)");
		//	Script::BindFunction<decltype(&MetaResource::Save), &MetaResource::Save>(false, "save(_)");
		//	Script::BindFunction<decltype(&MetaResource::Unload), &MetaResource::Unload>(false, "unload()");
		//}
		//Script::EndClass();
	}
	Script::EndModule();

	Script::BeginModule("device");
	{
		Script::BeginClass<GamepadButton, i32>("GamepadButton");
		{
			Script::BindEnumVal<GamepadButton, i32, GamepadButton::BUTTON_SOUTH>("south");
			Script::BindEnumVal<GamepadButton, i32, GamepadButton::BUTTON_EAST>("east");
			Script::BindEnumVal<GamepadButton, i32, GamepadButton::BUTTON_WEST>("west");
			Script::BindEnumVal<GamepadButton, i32, GamepadButton::BUTTON_NORTH>("north");
			Script::BindEnumVal<GamepadButton, i32, GamepadButton::SHOULDER_LEFT>("shoulderLeft");
			Script::BindEnumVal<GamepadButton, i32, GamepadButton::SHOULDER_RIGHT>("shoulderRight");
			Script::BindEnumVal<GamepadButton, i32, GamepadButton::BUTTON_SELECT>("select");
			Script::BindEnumVal<GamepadButton, i32, GamepadButton::BUTTON_START>("start");
			Script::BindEnumVal<GamepadButton, i32, GamepadButton::STICK_LEFT>("leftStickPress");
			Script::BindEnumVal<GamepadButton, i32, GamepadButton::STICK_RIGHT>("rightStickPress");
			Script::BindEnumVal<GamepadButton, i32, GamepadButton::DPAD_UP>("dPadUp");
			Script::BindEnumVal<GamepadButton, i32, GamepadButton::DPAD_RIGHT>("dPadRight");
			Script::BindEnumVal<GamepadButton, i32, GamepadButton::DPAD_DOWN>("dPadDown");
			Script::BindEnumVal<GamepadButton, i32, GamepadButton::DPAD_LEFT>("dPadLeft");
		}
		Script::EndClass();

		Script::BeginClass<GamepadAxis, i32>("GamepadAxis");
		{
			Script::BindEnumVal<GamepadAxis, i32, GamepadAxis::STICK_LEFT_X>("leftStickX");
			Script::BindEnumVal<GamepadAxis, i32, GamepadAxis::STICK_LEFT_Y>("leftStickY");
			Script::BindEnumVal<GamepadAxis, i32, GamepadAxis::STICK_RIGHT_X>("rightStickX");
			Script::BindEnumVal<GamepadAxis, i32, GamepadAxis::STICK_RIGHT_Y>("rightStickY");
			Script::BindEnumVal<GamepadAxis, i32, GamepadAxis::TRIGGER_LEFT>("leftTrigger");
			Script::BindEnumVal<GamepadAxis, i32, GamepadAxis::TRIGGER_RIGHT>("rightTrigger");
		}
		Script::EndClass();

		Script::BeginClass<MouseButton, i32>("MouseButton");
		{
			Script::BindEnumVal<MouseButton, i32, MouseButton::MOUSE_BUTTON_LEFT>("left");
			Script::BindEnumVal<MouseButton, i32, MouseButton::MOUSE_BUTTON_RIGHT>("right");
			Script::BindEnumVal<MouseButton, i32, MouseButton::MOUSE_BUTTON_MIDDLE>("middle");
		}
		Script::EndClass();

		Script::BeginClass<Key, i32>("Key");
		{
			Script::BindEnumVal<Key, i32, Key::RIGHT>("right");
			Script::BindEnumVal<Key, i32, Key::LEFT>("left");
			Script::BindEnumVal<Key, i32, Key::DOWN>("down");
			Script::BindEnumVal<Key, i32, Key::UP>("up");
			Script::BindEnumVal<Key, i32, Key::SPACE>("space");
			Script::BindEnumVal<Key, i32, Key::ESCAPE>("escape");
			Script::BindEnumVal<Key, i32, Key::ENTER>("enter");

			Script::BindEnumVal<Key, i32, Key::A>("a");
			Script::BindEnumVal<Key, i32, Key::B>("b");
			Script::BindEnumVal<Key, i32, Key::C>("c");
			Script::BindEnumVal<Key, i32, Key::D>("d");
			Script::BindEnumVal<Key, i32, Key::E>("e");
			Script::BindEnumVal<Key, i32, Key::F>("f");
			Script::BindEnumVal<Key, i32, Key::G>("g");
			Script::BindEnumVal<Key, i32, Key::H>("h");
			Script::BindEnumVal<Key, i32, Key::I>("i");
			Script::BindEnumVal<Key, i32, Key::J>("j");
			Script::BindEnumVal<Key, i32, Key::K>("k");
			Script::BindEnumVal<Key, i32, Key::L>("l");
			Script::BindEnumVal<Key, i32, Key::M>("m");
			Script::BindEnumVal<Key, i32, Key::N>("n");
			Script::BindEnumVal<Key, i32, Key::O>("o");
			Script::BindEnumVal<Key, i32, Key::P>("p");
			Script::BindEnumVal<Key, i32, Key::Q>("q");
			Script::BindEnumVal<Key, i32, Key::R>("r");
			Script::BindEnumVal<Key, i32, Key::S>("s");
			Script::BindEnumVal<Key, i32, Key::T>("t");
			Script::BindEnumVal<Key, i32, Key::U>("u");
			Script::BindEnumVal<Key, i32, Key::V>("v");
			Script::BindEnumVal<Key, i32, Key::W>("w");
			Script::BindEnumVal<Key, i32, Key::X>("x");
			Script::BindEnumVal<Key, i32, Key::Y>("y");
			Script::BindEnumVal<Key, i32, Key::Z>("z");
		}
		Script::EndClass();

		Script::BeginClass("Input");
		{
			Script::BindFunction<decltype(&Input::GetAxis), &Input::GetAxis>(true, "getAxis(_)");
			Script::BindFunction<decltype(&Input::GetButton), &Input::GetButton>(true, "getButton(_)");
			Script::BindFunction<decltype(&Input::GetButtonOnce), &Input::GetButtonOnce>(true, "getButtonOnce(_)");

			Script::BindFunction<decltype(&Input::GetKey), &Input::GetKey>(true, "getKey(_)");
			Script::BindFunction<decltype(&Input::GetKeyOnce), &Input::GetKeyOnce>(true, "getKeyOnce(_)");

			Script::BindFunction<decltype(&Input::GetMouse), &Input::GetMouse>(true, "getMouse()");
			Script::BindFunction<decltype(&Input::GetMouseButton), &Input::GetMouseButton>(true, "getMouseButton(_)");
			Script::BindFunction<decltype(&Input::GetMouseButtonOnce), &Input::GetMouseButtonOnce>(true, "getMouseButtonOnce(_)");
			Script::BindFunction<decltype(&Input::GetMouseX), &Input::GetMouseX>(true, "getMouseX()");
			Script::BindFunction<decltype(&Input::GetMouseY), &Input::GetMouseY>(true, "getMouseY()");

			Script::BindFunction<decltype(&Input::GetNumTouches), &Input::GetNumTouches>(true, "getNumTouches()");
			Script::BindFunction<decltype(&Input::GetTouchId), &Input::GetTouchId>(true, "getTouchId(_)");
			Script::BindFunction<decltype(&Input::GetTouchX), &Input::GetTouchX>(true, "getTouchX(_)");
			Script::BindFunction<decltype(&Input::GetTouchY), &Input::GetTouchY>(true, "getTouchY(_)");

			Script::BindFunction<decltype(&Input::SetPadVibration), &Input::SetPadVibration>(true, "setPadVibration(_,_)");
			Script::BindFunction<decltype(&Input::SetPadLightbarColor), &Input::SetPadLightbarColor>(true, "setPadLightbarColor(_,_,_)");
			Script::BindFunction<decltype(&Input::ResetPadLightbarColor), &Input::ResetPadLightbarColor>(true, "resetPadLightbarColor()");
		}
		Script::EndClass();

		Script::BeginClass("Screen");
		{
			Script::BindFunction<decltype(&Screen::GetWidth), &Screen::GetWidth>(true, "width");
			Script::BindFunction<decltype(&Screen::GetHeight), &Screen::GetHeight>(true, "height");
		}
		Script::EndClass();
	}
	Script::EndModule();

	Script::BeginModule("math");
	{
		Script::BeginClass<Vec2, f32, f32>("Vec2");
		{
			Script::BindGetter<Vec2, decltype(Vec2::x), &Vec2::x>("x");
			Script::BindSetter<Vec2, decltype(Vec2::x), &Vec2::x>("x=(_)");
			Script::BindGetter<Vec2, decltype(Vec2::y), &Vec2::y>("y");
			Script::BindSetter<Vec2, decltype(Vec2::y), &Vec2::y>("y=(_)");
		}

		Script::BeginClass<Vec3, f32, f32, f32>("Vec3");
		{
			Script::BindGetter<Vec3, decltype(Vec3::x), &Vec3::x>("x");
			Script::BindSetter<Vec3, decltype(Vec3::x), &Vec3::x>("x=(_)");
			Script::BindGetter<Vec3, decltype(Vec3::y), &Vec3::y>("y");
			Script::BindSetter<Vec3, decltype(Vec3::y), &Vec3::y>("y=(_)");
			Script::BindGetter<Vec3, decltype(Vec3::z), &Vec3::z>("z");
			Script::BindSetter<Vec3, decltype(Vec3::z), &Vec3::z>("z=(_)");
			Script::BindFunction<decltype(&Vec3::SqrMagnitude), &Vec3::SqrMagnitude>(false, "sqrMagnitude");
			Script::BindFunction<decltype(&Vec3::Magnitude), &Vec3::Magnitude>(false, "magnitude");
			Script::BindFunction<decltype(&Vec3::Normalized), &Vec3::Normalized>(false, "normalized");
			Script::BindFunction<decltype(&Vec3::At), &Vec3::At>(false, "at(_)");
			Script::BindFunction<decltype(&Vec3::Set), &Vec3::Set>(false, "set(_,_,_)");
			Script::BindFunction<decltype(&Vec3::Plus), &Vec3::Plus>(false, "+(_)");
			Script::BindFunction<decltype(&Vec3::Negate), &Vec3::Negate>(false, "negate");
			Script::BindFunction<decltype(&Vec3::Minus), &Vec3::Minus>(false, "-(_)");
			Script::BindFunction<decltype(&Vec3::Mul), &Vec3::Mul>(false, "*(_)");
			Script::BindFunction<decltype(&Vec3::Div), &Vec3::Div>(false, "/(_)");
			Script::BindFunction<decltype(&Vec3::Dot), &Vec3::Dot>(true, "dot(_,_)");
			Script::BindFunction<decltype(&Vec3::Normalize), &Vec3::Normalize>(true, "normalize(_)");
			Script::BindFunction<decltype(&Vec3::Cross), &Vec3::Cross>(true, "cross(_,_)");
			Script::BindFunction<decltype(&Vec3::Lerp), &Vec3::Lerp>(true, "lerp(_,_,_)");
		}
		Script::EndClass();

		Script::BeginClass<Vec4, f32, f32, f32, f32>("Vec4");
		{
			Script::BindGetter<Vec4, decltype(Vec4::x), &Vec4::x>("x");
			Script::BindSetter<Vec4, decltype(Vec4::x), &Vec4::x>("x=(_)");
			Script::BindGetter<Vec4, decltype(Vec4::y), &Vec4::y>("y");
			Script::BindSetter<Vec4, decltype(Vec4::y), &Vec4::y>("y=(_)");
			Script::BindGetter<Vec4, decltype(Vec4::z), &Vec4::z>("z");
			Script::BindSetter<Vec4, decltype(Vec4::z), &Vec4::z>("z=(_)");
			Script::BindGetter<Vec4, decltype(Vec4::w), &Vec4::w>("w");
			Script::BindSetter<Vec4, decltype(Vec4::w), &Vec4::w>("w=(_)");
		}
		Script::EndClass();

		Script::BeginClass<Vec4i, i32, i32, i32, i32>("Vec4i");
		{
			Script::BindGetter<Vec4i, decltype(Vec4i::x), &Vec4i::x>("x");
			Script::BindSetter<Vec4i, decltype(Vec4i::x), &Vec4i::x>("x=(_)");
			Script::BindGetter<Vec4i, decltype(Vec4i::y), &Vec4i::y>("y");
			Script::BindSetter<Vec4i, decltype(Vec4i::y), &Vec4i::y>("y=(_)");
			Script::BindGetter<Vec4i, decltype(Vec4i::z), &Vec4i::z>("z");
			Script::BindSetter<Vec4i, decltype(Vec4i::z), &Vec4i::z>("z=(_)");
			Script::BindGetter<Vec4i, decltype(Vec4i::w), &Vec4i::w>("w");
			Script::BindSetter<Vec4i, decltype(Vec4i::w), &Vec4i::w>("w=(_)");
		}
		Script::EndClass();

		Script::BeginClass<Quat, f32, f32, f32, f32>("Quat");
		{
			Script::BindGetter<Quat, decltype(Quat::x), &Quat::x>("x");
			Script::BindSetter<Quat, decltype(Quat::x), &Quat::x>("x=(_)");
			Script::BindGetter<Quat, decltype(Quat::y), &Quat::y>("y");
			Script::BindSetter<Quat, decltype(Quat::y), &Quat::y>("y=(_)");
			Script::BindGetter<Quat, decltype(Quat::z), &Quat::z>("z");
			Script::BindSetter<Quat, decltype(Quat::z), &Quat::z>("z=(_)");
			Script::BindGetter<Quat, decltype(Quat::w), &Quat::w>("w");
			Script::BindSetter<Quat, decltype(Quat::w), &Quat::w>("w=(_)");
			Script::BindFunction<decltype(&Quat::MulQuat), &Quat::MulQuat>(false, "mulQuat(_)");
			Script::BindFunction<decltype(&Quat::MulVec3), &Quat::MulVec3>(false, "mulVec3(_)");
			Script::BindFunction<decltype(&Quat::EulerAngles), &Quat::EulerAngles>(false, "eulerAngles");
			Script::BindFunction<decltype(&Quat::Euler), &Quat::Euler>(true, "euler(_,_,_)");
		}
		Script::EndClass();

		Script::BeginClass<Mat4, Vec4, Vec4, Vec4, Vec4>("Mat4");
		{
			//Script::BindGetter<Mat4, decltype(Mat4::x), &Mat4::x>("x");
			//Script::BindSetter<Mat4, decltype(Mat4::x), &Mat4::x>("x=(_)");
			//Script::BindGetter<Mat4, decltype(Mat4::y), &Mat4::y>("y");
			//Script::BindSetter<Mat4, decltype(Mat4::y), &Mat4::y>("y=(_)");
			//Script::BindGetter<Mat4, decltype(Mat4::z), &Mat4::z>("z");
			//Script::BindSetter<Mat4, decltype(Mat4::z), &Mat4::z>("z=(_)");
			//Script::BindGetter<Mat4, decltype(Mat4::w), &Mat4::w>("w");
			//Script::BindSetter<Mat4, decltype(Mat4::w), &Mat4::w>("w=(_)");
			Script::BindFunction<decltype(&Mat4::Mul), &Mat4::Mul>(false, "mulMat4(_)");
			Script::BindFunction<decltype(&Mat4::LookAt), &Mat4::LookAt>(true, "lookAt(_,_,_)");
			Script::BindFunction<decltype(&Mat4::Ortho), &Mat4::Ortho>(true, "ortho(_,_,_,_,_,_)");
			Script::BindFunction<decltype(&Mat4::Perspective), &Mat4::Perspective>(true, "perspective(_,_,_,_)");
			Script::BindFunction<decltype(&Mat4::TRS), &Mat4::TRS>(true, "trs(_,_,_)");
		}
		Script::EndClass();
	}
	Script::EndModule();

	Script::BeginModule("ecs");
	{
		// Disable constructor from wren
		Script::BeginClass<Entity, EntityId>("Entity");
		{
			Script::BindFunction<decltype(&EntityManager::CreateEntity), &EntityManager::CreateEntity>(true, "create()");

			Script::BindFunction<decltype(&Entity::Invalid), &Entity::Invalid>(true, "invalid");

			Script::BindFunction<decltype(&Entity::IsValid), &Entity::IsValid>(false, "isValid");
			//Script::BindFunction<decltype(&Entity::GetComponentMask), &Entity::GetComponentMask>(false, "getComponentMask()");
			//Script::BindFunction<decltype(&Entity::HasComponents), &Entity::HasComponents>(false, "hasComponents()");
			Script::BindCFunction(false, "hasComponent(_)", EntityHasComponent);
			Script::BindCFunction(false, "addComponent(_)", EntityAddComponent);
			Script::BindCFunction(false, "getComponent(_)", EntityGetComponent);
			Script::BindCFunction(false, "removeComponent(_)", EntityRemoveComponent);
			Script::BindFunction<decltype(&Entity::Destroy), &Entity::Destroy>(false, "destroy()");
		}
		Script::EndClass();
	}
	Script::EndModule();

	Script::BeginModule("graphics");
	{
		Script::BeginClass<TextureFormat, i32>("TextureFormat");
		{
			Script::BindEnumVal<TextureFormat, i32, TextureFormat::UNKNOWN>("unknown");
			Script::BindEnumVal<TextureFormat, i32, TextureFormat::RGBA8_UNORM>("rgba8_unorm");
		}
		Script::EndClass();

		Script::BeginClass<PipelineTopology, i32>("GeomTopology");
		{
			Script::BindEnumVal<PipelineTopology, i32, PipelineTopology::UNDEFINED>("undefined");
			Script::BindEnumVal<PipelineTopology, i32, PipelineTopology::POINTS>("points");
			Script::BindEnumVal<PipelineTopology, i32, PipelineTopology::LINES>("lines");
			Script::BindEnumVal<PipelineTopology, i32, PipelineTopology::TRIANGLES>("triangles");
		}
		Script::EndClass();

		Script::BeginClass<PipelineFaceCull, i32>("FaceCull");
		{
			Script::BindEnumVal<PipelineFaceCull, i32, PipelineFaceCull::NONE>("none");
			Script::BindEnumVal<PipelineFaceCull, i32, PipelineFaceCull::CW>("cw");
			Script::BindEnumVal<PipelineFaceCull, i32, PipelineFaceCull::CCW>("ccw");
		}
		Script::EndClass();

		Script::BeginClass<ShaderType, i32>("ShaderType");
		{
			Script::BindEnumVal<ShaderType, i32, ShaderType::UNKNOWN>("none");
			Script::BindEnumVal<ShaderType, i32, ShaderType::VERTEX>("vertex");
			Script::BindEnumVal<ShaderType, i32, ShaderType::PIXEL>("pixel");
			Script::BindEnumVal<ShaderType, i32, ShaderType::GEOMETRY>("geometry");
			Script::BindEnumVal<ShaderType, i32, ShaderType::COMPUTE>("compute");
		}
		Script::EndClass();

		Script::BeginClass<GraphicsClearFlags, i32>("ClearFlags");
		{
			Script::BindEnumVal<GraphicsClearFlags, i32, GraphicsClearFlags::NONE>("none");
			Script::BindEnumVal<GraphicsClearFlags, i32, GraphicsClearFlags::DEPTH>("depth");
			Script::BindEnumVal<GraphicsClearFlags, i32, GraphicsClearFlags::STENCIL>("stencil");
		}
		Script::EndClass();

		Script::BeginClass<GraphicsValueType, i32>("ValueType");
		{
			Script::BindEnumVal<GraphicsValueType, i32, GraphicsValueType::UNDEFINED>("undefined");
			Script::BindEnumVal<GraphicsValueType, i32, GraphicsValueType::INT8>("int8");
			Script::BindEnumVal<GraphicsValueType, i32, GraphicsValueType::INT16>("int16");
			Script::BindEnumVal<GraphicsValueType, i32, GraphicsValueType::INT32>("int32");
			Script::BindEnumVal<GraphicsValueType, i32, GraphicsValueType::UINT8>("uint8");
			Script::BindEnumVal<GraphicsValueType, i32, GraphicsValueType::UINT16>("uint16");
			Script::BindEnumVal<GraphicsValueType, i32, GraphicsValueType::UINT32>("uint32");
			Script::BindEnumVal<GraphicsValueType, i32, GraphicsValueType::FLOAT16>("float16");
			Script::BindEnumVal<GraphicsValueType, i32, GraphicsValueType::FLOAT32>("float32");
		}
		Script::EndClass();

		Script::BeginClass<BufferUsage, i32>("UsageFlags");
		{
			Script::BindEnumVal<BufferUsage, i32, BufferUsage::IMMUTABLE>("immutable");
			Script::BindEnumVal<BufferUsage, i32, BufferUsage::DEFAULT>("default");
			Script::BindEnumVal<BufferUsage, i32, BufferUsage::DYNAMIC>("dynamic");
		}
		Script::EndClass();

		Script::BeginClass<BufferType, i32>("BindFlags");
		{
			Script::BindEnumVal<BufferType, i32, BufferType::VERTEX_BUFFER>("vertex");
			Script::BindEnumVal<BufferType, i32, BufferType::INDEX_BUFFER>("index");
			Script::BindEnumVal<BufferType, i32, BufferType::UNIFORM_BUFFER>("uniform");
		}
		Script::EndClass();

		Script::BeginClass<BufferAccess, i32>("CpuAccessFlags");
		{
			Script::BindEnumVal<BufferAccess, i32, BufferAccess::NONE>("none");
			Script::BindEnumVal<BufferAccess, i32, BufferAccess::READ>("read");
			Script::BindEnumVal<BufferAccess, i32, BufferAccess::WRITE>("write");
		}
		Script::EndClass();

		Script::BeginClass("Graphics");
		{
			Script::BindFunction<decltype(&Graphics::GetColorBufferFormat), &Graphics::GetColorBufferFormat>(true, "getColorBufferFormat()");
			Script::BindFunction<decltype(&Graphics::GetDepthBufferFormat), &Graphics::GetDepthBufferFormat>(true, "getDepthBufferFormat()");

			//Script::BindFunction<decltype(&Graphics::CreateCubemap), &Graphics::CreateCubemap>(true, "createCubemap(_)");
			//Script::BindFunction<decltype(&Graphics::CreateMesh), &Graphics::CreateMesh>(true, "createMesh(_)");
			//
			//Script::BindFunction<decltype(&Graphics::BatchBegin), &Graphics::BatchBegin>(true, "batchBegin()");
			//Script::BindFunction<decltype(&Graphics::BatchInstance), &Graphics::BatchInstance>(true, "batchInstance(_,_)");
			//Script::BindFunction<decltype(&Graphics::BatchDraw), &Graphics::BatchDraw>(true, "batchDraw(_,_)");
			//Script::BindFunction<decltype(&Graphics::BatchEnd), &Graphics::BatchEnd>(true, "batchEnd()");
			//
			//Script::BindFunction<decltype(&Graphics::DrawBegin), &Graphics::DrawBegin>(true, "drawBegin(_)");
			//Script::BindFunction<decltype(&Graphics::DrawSetFloat), &Graphics::DrawSetFloat>(true, "drawSetFloat(_,_)");
			//Script::BindFunction<decltype(&Graphics::DrawSetVec2), &Graphics::DrawSetVec2>(true, "drawSetVec2(_,_)");
			//Script::BindFunction<decltype(&Graphics::DrawSetVec3), &Graphics::DrawSetVec3>(true, "drawSetVec3(_,_)");
			//Script::BindFunction<decltype(&Graphics::DrawSetVec4), &Graphics::DrawSetVec4>(true, "drawSetVec4(_,_)");
			//Script::BindFunction<decltype(&Graphics::DrawSetMat4), &Graphics::DrawSetMat4>(true, "drawSetMat4(_,_)");
			//Script::BindFunction<decltype(&Graphics::DrawBindTexture2D), &Graphics::DrawBindTexture2D>(true, "drawBindTexture(_,_,_)");
			//Script::BindFunction<decltype(&Graphics::DrawBindCubemap), &Graphics::DrawBindCubemap>(true, "drawBindCubemap(_,_,_)");
			//Script::BindFunction<decltype(&Graphics::DrawEnd), &Graphics::DrawEnd>(true, "drawEnd(_)");
			//
			//Script::BindFunction<decltype(&Graphics::ConvoluteEnvMap), &Graphics::ConvoluteEnvMap>(true, "convoluteEnvMap(_,_,_)");
			//Script::BindFunction<decltype(&Graphics::PrefilterEnvMap), &Graphics::PrefilterEnvMap>(true, "prefilterEnvMap(_,_,_)");
			//Script::BindFunction<decltype(&Graphics::GenBrdfLut), &Graphics::GenBrdfLut>(true, "genBrdfLut(_,_)");
			//
			//Script::BindFunction<decltype(&Graphics::DrawScreen), &Graphics::DrawScreen>(true, "drawScreen(_,_)");
			//Script::BindFunction<decltype(&Graphics::DrawSkybox), &Graphics::DrawSkybox>(true, "drawSkybox(_,_,_,_)");

			Script::BindFunction<decltype(&Graphics::DebugLine), &Graphics::DebugLine>(true, "debugLine(_,_,_,_)");
		}
		Script::EndClass();

		Script::BeginClass<Shader>("Shader");
		{
			Script::BindFunction<decltype(&Shader::GetSource), &Shader::GetSource>(false, "source");
		}
		Script::EndClass();

		Script::BeginClass<Texture>("Texture");
		{
		}
		Script::EndClass();

		Script::BeginClass<Material>("Material");
		{
		}
		Script::EndClass();

		Script::BeginClass<Mesh>("Mesh");
		{
		}
		Script::EndClass();
	}
	Script::EndModule();

	Script::BeginModule("physics");
	{
		Script::BeginClass<ColliderShape, i32>("ColliderShape");
		{
			Script::BindEnumVal<ColliderShape, i32, ColliderShape::PLANE>("plane");
			Script::BindEnumVal<ColliderShape, i32, ColliderShape::BOX>("box");
			Script::BindEnumVal<ColliderShape, i32, ColliderShape::SPHERE>("sphere");
			Script::BindEnumVal<ColliderShape, i32, ColliderShape::CAPSULE>("capsule");
			Script::BindEnumVal<ColliderShape, i32, ColliderShape::MESH>("mesh");
		}
		Script::EndClass();

		Script::BeginClass<ColliderAxis, i32>("ColliderAxis");
		{
			Script::BindEnumVal<ColliderAxis, i32, ColliderAxis::AXIS_X>("axisX");
			Script::BindEnumVal<ColliderAxis, i32, ColliderAxis::AXIS_Y>("axisY");
			Script::BindEnumVal<ColliderAxis, i32, ColliderAxis::AXIS_Z>("axisZ");
		}
		Script::EndClass();

		Script::BeginClass<ColliderFlags, i32>("ColliderFlags");
		{
			// TODO: Make this without an explicit wren function
			Script::BindCFunction(false, "|(_)", [](WrenVM* vm) { ScriptArg<ColliderFlags>::Set(vm, 0, ScriptArg<ColliderFlags>::Get(vm, 0) | ScriptArg<ColliderFlags>::Get(vm, 1)); });
			Script::BindCFunction(false, "&(_)", [](WrenVM* vm) { ScriptArg<ColliderFlags>::Set(vm, 0, ScriptArg<ColliderFlags>::Get(vm, 0) & ScriptArg<ColliderFlags>::Get(vm, 1)); });
			Script::BindCFunction(false, "^(_)", [](WrenVM* vm) { ScriptArg<ColliderFlags>::Set(vm, 0, ScriptArg<ColliderFlags>::Get(vm, 0) ^ ScriptArg<ColliderFlags>::Get(vm, 1)); });

			Script::BindEnumVal<ColliderFlags, i32, ColliderFlags::DYNAMIC>("dynamic");
			Script::BindEnumVal<ColliderFlags, i32, ColliderFlags::STATIC>("Static");
			Script::BindEnumVal<ColliderFlags, i32, ColliderFlags::KINEMATIC>("kinematic");
			Script::BindEnumVal<ColliderFlags, i32, ColliderFlags::CHARACTER>("character");
		}
		Script::EndClass();

		Script::BeginClass<CollisionFlags, i32>("CollisionFlags");
		{
			// TODO: Make this without an explicit wren function
			Script::BindCFunction(false, "|(_)", [](WrenVM* vm) { ScriptArg<CollisionFlags>::Set(vm, 0, ScriptArg<CollisionFlags>::Get(vm, 0) | ScriptArg<CollisionFlags>::Get(vm, 1)); });
			Script::BindCFunction(false, "&(_)", [](WrenVM* vm) { ScriptArg<CollisionFlags>::Set(vm, 0, ScriptArg<CollisionFlags>::Get(vm, 0) & ScriptArg<CollisionFlags>::Get(vm, 1)); });
			Script::BindCFunction(false, "^(_)", [](WrenVM* vm) { ScriptArg<CollisionFlags>::Set(vm, 0, ScriptArg<CollisionFlags>::Get(vm, 0) ^ ScriptArg<CollisionFlags>::Get(vm, 1)); });
			
			Script::BindEnumVal<CollisionFlags, i32, CollisionFlags::DEFAULT>("default");
			Script::BindEnumVal<CollisionFlags, i32, CollisionFlags::STATIC>("Static");
			Script::BindEnumVal<CollisionFlags, i32, CollisionFlags::KINEMATIC>("kinematic");
			Script::BindEnumVal<CollisionFlags, i32, CollisionFlags::DEBRIS>("debris");
			Script::BindEnumVal<CollisionFlags, i32, CollisionFlags::TRIGGER>("trigger");
			Script::BindEnumVal<CollisionFlags, i32, CollisionFlags::CHARACTER>("character");
			Script::BindEnumVal<CollisionFlags, i32, CollisionFlags::ALL>("all");
		}
		Script::EndClass();

		Script::BeginClass<CastHitResult>("CastHitResult");
		{
			Script::BindGetter<CastHitResult, decltype(CastHitResult::hasHit), &CastHitResult::hasHit>("hasHit");
			Script::BindGetter<CastHitResult, decltype(CastHitResult::point), &CastHitResult::point>("point");
			Script::BindGetter<CastHitResult, decltype(CastHitResult::normal), &CastHitResult::normal>("normal");

			Script::BindCFunction(false, "gameObject", [](WrenVM* vm)
				{
					const auto& hit = ScriptArg<const CastHitResult&>::Get(vm, 0);
					const auto& gameObj = GameObject::Find(Scene::GetCurrent(), hit.id);
					gameObj.Bind();
				});
		}
		Script::EndClass();

		Script::BeginClass("Physics");
		{
			Script::BindFunction<decltype(&Physics::RayCast), &Physics::RayCast>(true, "rayCast(_,_,_,_,_)");
			Script::BindFunction<decltype(&Physics::RayCastAll), &Physics::RayCastAll>(true, "rayCastAll(_,_,_,_,_)");
			Script::BindFunction<decltype(&Physics::SphereCast), &Physics::SphereCast>(true, "sphereCast(_,_,_,_,_,_)");
			Script::BindFunction<decltype(&Physics::SphereCastAll), &Physics::SphereCastAll>(true, "sphereCastAll(_,_,_,_,_,_)");
		}
		Script::EndClass();
	}
	Script::EndModule();
	
	Script::BeginModule("framework");
	{
		Script::BeginClass<GameObjectData>("GameObjectData");
		{
			Script::BindCFunction(false, "type", GetTypeGameObjectData);
			Script::BindCFunction(true, "create(_)", CreateGameObjectData);
			Script::BindFunction<decltype(&GameObjectData::Load), &GameObjectData::Load>(true, "load(_)");
		}
		Script::EndClass();

		Script::BeginClass<GameObjectBase>("GameObjectBase", ConstructGameObjectBase, FinalizeGameObjectBase);
		{
			Script::BindFunction<decltype(&GameObjectBase::Initialize), &GameObjectBase::Initialize>(false, "initialize(_)");			Script::BindFunction<decltype(&GameObjectBase::Initialize), &GameObjectBase::Initialize>(false, "initialize(_)");
			Script::BindFunction<decltype(&GameObjectBase::Destroy), &GameObjectBase::Destroy>(false, "destroy()");
			Script::BindFunction<decltype(&GameObjectBase::GetEntity), &GameObjectBase::GetEntity>(false, "entity");
			Script::BindFunction<decltype(&GameObjectBase::GetName), &GameObjectBase::GetName>(false, "name");
			Script::BindFunction<decltype(&GameObjectBase::SetName), &GameObjectBase::SetName>(false, "name=(_)");
		}
		Script::EndClass();

		Script::BeginClass("GameObject");
		{
			Script::BindCFunction(true, "register(_)", RegisterGameObject);
		}
		Script::EndClass();

		Script::BeginClass<Scene>("Scene");
		{
		}
		Script::EndClass();

		Script::BeginComponentClass<Animator>("Animator");
		{
			Script::BindFunction<decltype(&Animator::GetCurrent), &Animator::GetCurrent>(false, "current");
			Script::BindFunction<decltype(&Animator::SetCurrent), &Animator::SetCurrent>(false, "current=(_)");

			Script::BindFunction<decltype(&Animator::HasEnded), &Animator::HasEnded>(false, "hasEnded");
			Script::BindFunction<decltype(&Animator::GetDuration), &Animator::GetDuration>(false, "duration(_)");

			Script::BindFunction<decltype(&Animator::GetSpeed), &Animator::GetSpeed>(false, "speed");
			Script::BindFunction<decltype(&Animator::SetSpeed), &Animator::SetSpeed>(false, "speed=(_)");
			Script::BindFunction<decltype(&Animator::GetLooping), &Animator::GetLooping>(false, "looping");
			Script::BindFunction<decltype(&Animator::SetLooping), &Animator::SetLooping>(false, "looping=(_)");

			Script::BindFunction<decltype(&Animator::GetBoneMatrix), &Animator::GetBoneMatrix>(false, "getBoneMatrix(_)");
		}
		Script::EndClass();

		Script::BeginComponentClass<Attributes>("Attributes");
		{
		}
		Script::EndClass();

		Script::BeginComponentClass<AudioListener>("AudioListener");
		{
		}
		Script::EndClass();

		Script::BeginComponentClass<AudioSource>("AudioSource");
		{
		}
		Script::EndClass();

		Script::BeginComponentClass<Camera>("Camera");
		{
			Script::BindFunction<decltype(&Camera::GetFov), &Camera::GetFov>(false, "fov");
			Script::BindFunction<decltype(&Camera::SetFov), &Camera::SetFov>(false, "fov=(_)");
			Script::BindFunction<decltype(&Camera::GetAspect), &Camera::GetAspect>(false, "aspect");
			Script::BindFunction<decltype(&Camera::SetAspect), &Camera::SetAspect>(false, "aspect=(_)");
			Script::BindFunction<decltype(&Camera::GetZNear), &Camera::GetZNear>(false, "near");
			Script::BindFunction<decltype(&Camera::SetZNear), &Camera::SetZNear>(false, "near=(_)");
			Script::BindFunction<decltype(&Camera::GetZFar), &Camera::GetZFar>(false, "far");
			Script::BindFunction<decltype(&Camera::SetZFar), &Camera::SetZFar>(false, "far=(_)");
		}
		Script::EndClass();

		Script::BeginComponentClass<CharacterController>("CharacterController");
		{
			Script::BindFunction<decltype(&CharacterController::GetOffset), &CharacterController::GetOffset>(false, "offset");
			Script::BindFunction<decltype(&CharacterController::SetOffset), &CharacterController::SetOffset>(false, "offset=(_)");
			Script::BindFunction<decltype(&CharacterController::GetWidth), &CharacterController::GetWidth>(false, "width");
			Script::BindFunction<decltype(&CharacterController::SetWidth), &CharacterController::SetWidth>(false, "width=(_)");
			Script::BindFunction<decltype(&CharacterController::GetHeight), &CharacterController::GetHeight>(false, "height");
			Script::BindFunction<decltype(&CharacterController::SetHeight), &CharacterController::SetHeight>(false, "height=(_)");

			Script::BindFunction<decltype(&CharacterController::SetMoveVector), &CharacterController::SetMoveVector>(false, "moveVector=(_)");
			Script::BindFunction<decltype(&CharacterController::SetLinearVelocity), &CharacterController::SetLinearVelocity>(false, "linearVelocity=(_)");
			Script::BindFunction<decltype(&CharacterController::SetRotation), &CharacterController::SetRotation>(false, "rotation=(_)");
			Script::BindFunction<decltype(&CharacterController::ApplyImpulse), &CharacterController::ApplyImpulse>(false, "applyImpulse(_)");
		}
		Script::EndClass();

		Script::BeginComponentClass<Collider>("Collider");
		{
		}
		Script::EndClass();

		Script::BeginComponentClass<Light>("Light");
		{
			Script::BindFunction<decltype(&Light::GetIntensity), &Light::GetIntensity>(false, "intensity");
			Script::BindFunction<decltype(&Light::SetIntensity), &Light::SetIntensity>(false, "intensity=(_)");

			Script::BindFunction<decltype(&Light::GetConstant), &Light::GetConstant>(false, "constant");
			Script::BindFunction<decltype(&Light::SetConstant), &Light::SetConstant>(false, "constant=(_)");

			Script::BindFunction<decltype(&Light::GetLinear), &Light::GetLinear>(false, "linear");
			Script::BindFunction<decltype(&Light::SetLinear), &Light::SetLinear>(false, "linear=(_)");

			Script::BindFunction<decltype(&Light::GetQuadratic), &Light::GetQuadratic>(false, "quadratic");
			Script::BindFunction<decltype(&Light::SetQuadratic), &Light::SetQuadratic>(false, "quadratic=(_)");

			Script::BindFunction<decltype(&Light::GetColor), &Light::GetColor>(false, "color");
			Script::BindFunction<decltype(&Light::SetColor), &Light::SetColor>(false, "color=(_)");
		}
		Script::EndClass();

		Script::BeginComponentClass<MeshFilter>("MeshFilter");
		{
			//Script::BindFunction<decltype(&MeshFilter::GetMesh), &MeshFilter::GetMesh>(false, "mesh");
			//Script::BindFunction<decltype(&MeshFilter::SetMesh), &MeshFilter::SetMesh>(false, "mesh=(_)");
		}
		Script::EndClass();
		
		Script::BeginComponentClass<MeshRenderer>("MeshRenderer");
		{
			//Script::BindFunction<decltype(&MeshRenderer::GetMaterial), &MeshRenderer::GetMaterial>(false, "material");
			//Script::BindFunction<decltype(&MeshRenderer::SetMaterial), &MeshRenderer::SetMaterial>(false, "material=(_)");
		}
		Script::EndClass();

		Script::BeginComponentClass<RigidBody>("RigidBody");
		{
		}
		Script::EndClass();

		Script::BeginComponentClass<Spline>("Spline");
		{
		}
		Script::EndClass();

		Script::BeginComponentClass<Transform>("Transform");
		{
			Script::BindFunction<decltype(&Transform::GetPosition), &Transform::GetPosition>(false, "position");
			Script::BindFunction<decltype(&Transform::SetPosition), &Transform::SetPosition>(false, "position=(_)");
			Script::BindFunction<decltype(&Transform::GetRotation), &Transform::GetRotation>(false, "rotation");
			Script::BindFunction<decltype(&Transform::SetRotation), &Transform::SetRotation>(false, "rotation=(_)");
			Script::BindFunction<decltype(&Transform::GetScale), &Transform::GetScale>(false, "scale");
			Script::BindFunction<decltype(&Transform::SetScale), &Transform::SetScale>(false, "scale=(_)");
			Script::BindFunction<decltype(&Transform::GetMatrix), &Transform::GetMatrix>(false, "matrix");
			Script::BindFunction<decltype(&Transform::SetMatrix), &Transform::SetMatrix>(false, "matrix=(_)");
		}
		Script::EndClass();
	}
	Script::EndModule();
}

void Script::RegisterClass(TypeId typeId)
{
	if (s_foreignClassRegistry.find(typeId) == s_foreignClassRegistry.end())
	{
		ScriptClassInfo info;
		info.moduleName = s_moduleName;
		info.className = s_className;
		info.typeId = typeId;

		s_foreignClassRegistry.insert(std::make_pair(typeId, info));
	}
}

const ScriptClassInfo& Script::GetClassInfo(TypeId typeId)
{
	auto it = s_foreignClassRegistry.find(typeId);
	BX_ASSERT(it != s_foreignClassRegistry.end(), "Class is not registered!");
	return it->second;
}

void Script::SetClass(WrenVM* vm, i32 slot, TypeId typeId)
{
	const auto& info = Script::GetClassInfo(typeId);

	wrenEnsureSlots(vm, slot + 1);
	wrenGetVariable(vm, info.moduleName, info.className, slot);

	// TODO: Test this
	//auto it = s_foreignClassRegistry.find(typeId);
	//ENGINE_ASSERT(it != s_foreignClassRegistry.end(), "Class is not registered!");
	//const auto& info = it->second;
	//wrenSetSlotHandle(vm, slot, info.handle);
}

void Script::RegisterConstructor(SizeType arity, const char* signature, WrenForeignMethodFn func)
{
	const SizeType hash = GetClassHash(s_moduleName, s_className);
	s_foreignConstructors.insert(std::make_pair(hash, func));
}

void Script::RegisterDestructor(WrenFinalizerFn func)
{
	const SizeType hash = GetClassHash(s_moduleName, s_className);
	s_foreignDestructors.insert(std::make_pair(hash, func));
}

void Script::RegisterFunction(bool isStatic, const char* signature, WrenForeignMethodFn func)
{
	const SizeType hash = GetMethodHash(s_moduleName, s_className, isStatic, signature);
	s_foreignMethods.insert(std::make_pair(hash, func));
}

void Script::EnsureSlots(WrenVM* vm, i32 numSlots)
{
	wrenEnsureSlots(vm, numSlots);
}

bool ScriptArg<bool>::Get(WrenVM* vm, i32 slot) { return wrenGetSlotBool(vm, slot); }
void ScriptArg<bool>::Set(WrenVM* vm, i32 slot, bool val) { wrenSetSlotBool(vm, slot, val); }

i32 ScriptArg<i32>::Get(WrenVM* vm, i32 slot) { return static_cast<i32>(wrenGetSlotDouble(vm, slot)); }
void ScriptArg<i32>::Set(WrenVM* vm, i32 slot, i32 val) { wrenSetSlotDouble(vm, slot, static_cast<f64>(val)); }

u32 ScriptArg<u32>::Get(WrenVM* vm, i32 slot) { return static_cast<u32>(wrenGetSlotDouble(vm, slot)); }
void ScriptArg<u32>::Set(WrenVM* vm, i32 slot, u32 val) { wrenSetSlotDouble(vm, slot, static_cast<f64>(val)); }

// TODO: I'm leaving this for now, however, for entity IDs this breaks since they exceed the max stable double value
u64 ScriptArg<u64>::Get(WrenVM* vm, i32 slot) { return static_cast<u64>(wrenGetSlotDouble(vm, slot)); }
void ScriptArg<u64>::Set(WrenVM* vm, i32 slot, u64 val) { wrenSetSlotDouble(vm, slot, static_cast<u64>(val)); }

f32 ScriptArg<f32>::Get(WrenVM* vm, i32 slot) { return static_cast<f32>(wrenGetSlotDouble(vm, slot)); }
void ScriptArg<f32>::Set(WrenVM* vm, i32 slot, f32 val) { wrenSetSlotDouble(vm, slot, static_cast<f64>(val)); }

f64 ScriptArg<f64>::Get(WrenVM* vm, i32 slot) { return wrenGetSlotDouble(vm, slot); }
void ScriptArg<f64>::Set(WrenVM* vm, i32 slot, f64 val) { wrenSetSlotDouble(vm, slot, val); }

const char* ScriptArg<const char*>::Get(WrenVM* vm, i32 slot) { return wrenGetSlotString(vm, slot); }
void ScriptArg<const char*>::Set(WrenVM* vm, i32 slot, const char* val) { wrenSetSlotString(vm, slot, val); }

String ScriptArg<String>::Get(WrenVM* vm, i32 slot) { return String(wrenGetSlotString(vm, slot)); }
void ScriptArg<String>::Set(WrenVM* vm, i32 slot, const String& val) { wrenSetSlotString(vm, slot, val.c_str()); }

String ScriptArg<String&>::Get(WrenVM* vm, i32 slot) { return String(wrenGetSlotString(vm, slot)); }
void ScriptArg<String&>::Set(WrenVM* vm, i32 slot, const String& val) { wrenSetSlotString(vm, slot, val.c_str()); }

String ScriptArg<const String&>::Get(WrenVM* vm, i32 slot) { return String(wrenGetSlotString(vm, slot)); }
void ScriptArg<const String&>::Set(WrenVM* vm, i32 slot, const String& val) { wrenSetSlotString(vm, slot, val.c_str()); }

void* ScriptArg<void*>::Get(WrenVM* vm, i32 slot) { return wrenGetSlotForeign(vm, slot); }
void* ScriptArg<void*>::Set(WrenVM* vm, i32 slot, i32 classSlot, SizeType size) { return wrenSetSlotNewForeign(vm, slot, classSlot, size); }

void ScriptArg<void*>::SetList(WrenVM* vm, i32 slot)
{
	wrenSetSlotNewList(vm, 0);
}

i32 ScriptArg<void*>::GetListCount(WrenVM* vm, i32 slot) { return wrenGetListCount(vm, slot); }
void ScriptArg<void*>::GetListElement(WrenVM* vm, i32 listSlot, i32 index, i32 elementSlot) { wrenGetListElement(vm, listSlot, index, elementSlot); }
void ScriptArg<void*>::SetListElement(WrenVM* vm, i32 listSlot, i32 index, i32 elementSlot) { wrenSetListElement(vm, listSlot, index, elementSlot); }

void ScriptArg<void*>::InsertInList(WrenVM* vm, i32 listSlot, i32 index, i32 elementSlot)
{
	wrenInsertInList(vm, listSlot, index, elementSlot);
}

bool Script::Initialize()
{
	//if (s_initialized && !s_error)
	//{
	//	wrenEnsureSlots(s_vm, 1);
	//	wrenSetSlotHandle(s_vm, 0, s_gameClass);
	//	wrenCall(s_vm, s_initMethod);
	//}

	CreateVm();

	BindApi();
	for (const auto& bindApi : g_bindApis)
		bindApi();

	Configure();

	return true;
}

void Script::Reload()
{
	Shutdown();
	Initialize();
}

void Script::Shutdown()
{
	DestroyVm();
}

void Script::Update()
{
	PROFILE_FUNCTION();

	//if (s_initialized && !s_error)
	//{
	//	wrenEnsureSlots(s_vm, 1);
	//	wrenSetSlotHandle(s_vm, 0, s_gameClass);
	//	wrenCall(s_vm, s_updateMethod);
	//}
}

void Script::Render()
{
	PROFILE_FUNCTION();

	//if (s_initialized && !s_error)
	//{
	//	wrenEnsureSlots(s_vm, 1);
	//	wrenSetSlotHandle(s_vm, 0, s_gameClass);
	//	wrenCall(s_vm, s_renderMethod);
	//}
}

void Script::CollectGarbage()
{
	PROFILE_FUNCTION();
	wrenCollectGarbage(s_vm);
}