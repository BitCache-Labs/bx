#include <bx/engine/application.hpp>

#include <bx/core/module.hpp>
#include <bx/core/serial.hpp>
#include <bx/engine/plugin.hpp>
#include <bx/editor/view.hpp>
#include <bx/editor/views/assets_view.hpp>

#include <rttr/type.h>

#include <ostream>
#include <iostream>

static int TestDLLs()
{
	//Module::Load("D:/Github/BitCacheLabs/bx/demo/libs");
	Module::LoadSingleLib("D:/Github/BitCacheLabs/bx/demo/libs/game.dll");
	//rttr::variant_cast<Plugin*>(rttr::type::get_by_name("Game").create())->Initialize();
	
	String json;

	// Shared linked DLL test
	{
		std::shared_ptr<View> assetEditor = std::make_shared<AssetEditor>();
		json = Serializer::Save(assetEditor);
		std::cout << "AssetEditor: " << json << std::endl;

		std::shared_ptr<View> assetsView = std::make_shared<AssetsView>();
		json = Serializer::Save(assetsView);
		std::cout << "AssetsView: " << json << std::endl;
	}

	// Runtime loaded DLL test
	{
		auto type = rttr::type::get_by_name("LevelView");
		if (type.is_valid())
		{
			auto view = type.create().get_value<std::shared_ptr<View>>();
			json = Serializer::Save(view);
			std::cout << "LevelView: " << json << std::endl;
		}
	}

	return 0;
}

int main(int argc, char** argv)
{
	return TestDLLs();

//#ifdef PROJECT_PATH
//	File::Initialize(config.argv[1]);
//#else
//	BX_ENSURE(config.argc >= 2);
//	File::Initialize(config.argv[1]);
//#endif

	//return Application::Launch(AppConfig{ argc, argv });
}