#pragma once

#include <engine/api.hpp>
#include <engine/type.hpp>
#include <engine/ecs.hpp>
#include <engine/string.hpp>
#include <engine/script.hpp>

struct GameObjectType
{
	CString<64> classModule{};
	CString<64> className{};
};

class BX_API GameObject
{
	BX_TYPE(GameObject)

public:
	GameObject();
	~GameObject();

	inline const String& GetClassName() const { return m_className; }
	inline Entity GetEntity() const { return m_entity; }

	inline void Bind() const
	{
	}

	inline void Start()
	{
	}

	inline void Update() const
	{
	}

	static List<GameObjectType> GetTypes(ScriptHandle vm);

private:
	String m_className{};
	Entity m_entity{};
};