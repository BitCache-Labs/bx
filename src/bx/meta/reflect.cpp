#include <bx/meta/reflect.hpp>

#include <bx/meta/any.hpp>

#include <iostream>

/*template <>
struct Reflect<Any>
{
	Reflect()
	{
		//{
		//	auto p = &Any::m_pData; // test access
		//	MetaType metaType(Type<Any>::Id(), "Any");
		//	//metaType.AddFunction<Any, bool, TypeId>();
		//	ReflectManager::Register(metaType);
		//}
		//
		//{
		//	const auto& metaType = ReflectManager::GetType(Type<Any>::Id());
		//	std::cout << "REFLECT: " << metaType.GetName() << std::endl;
		//}
	}
};

REFLECT_AT_START_UP(Any);*/