#include "bx/engine/core/data.hpp"

#include "bx/engine/core/log.hpp"
#include "bx/engine/core/math.hpp"
#include "bx/engine/core/file.hpp"
#include "bx/engine/containers/hash_map.serial.hpp"

#include <cereal/cereal.hpp>
#include <cereal/archives/json.hpp>

#include <fstream>

//#define RESET_SAVES

void Data::Initialize()
{
	Load(DataTarget::SYSTEM);
	Load(DataTarget::GAME);
	Load(DataTarget::PLAYER);
	Load(DataTarget::DEBUG);
#ifdef BX_EDITOR_BUILD
	Load(DataTarget::EDITOR);
#endif
}

void Data::Shutdown()
{
	Save(DataTarget::SYSTEM);
	Save(DataTarget::GAME);
	Save(DataTarget::PLAYER);
	Save(DataTarget::DEBUG);
#ifdef BX_EDITOR_BUILD
	Save(DataTarget::EDITOR);
#endif
}

void Data::Save(DataTarget target)
{
	auto filepath = File::GetPath(GetFilepath(target));
	std::ofstream ofs(filepath);
	if (!ofs.is_open())
		return;
	
	cereal::JSONOutputArchive archive(ofs);
	archive(GetDatabase(target));
}

void Data::Load(DataTarget target)
{
#ifdef RESET_SAVES
	Data::Save(target);
#endif

	auto filepath = File::GetPath(GetFilepath(target));
	std::ifstream ifs(filepath);
	if (!ifs.is_open())
		return;

	cereal::JSONInputArchive archive(ifs);
	archive(GetDatabase(target));
}

const String& Data::GetFilepath(DataTarget target)
{
	static const String no_path = "";
	static const String system_path = "[settings]/system.json";
	static const String game_path = "[settings]/game.json";
	static const String player_path = "[save]/player.json";
	static const String debug_path = "[save]/debug.json";
#ifdef BX_EDITOR_BUILD
	static const String editor_path = "[editor]/settings.json";
#endif
	
	switch (target)
	{
	case DataTarget::SYSTEM:
		return system_path;
	case DataTarget::GAME:
		return game_path;
	
	case DataTarget::PLAYER:
		return player_path;
	case DataTarget::DEBUG:
		return debug_path;

#ifdef BX_EDITOR_BUILD
	case DataTarget::EDITOR:
		return editor_path;
#endif

	case DataTarget::NONE:
	default: return no_path;
	}
}