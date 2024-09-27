#pragma once

/*
template<typename>
struct Reflector
{
	static constexpr bool sIsSpecialized = false;
};

class MetaType;

struct ReflectAccess
{
private:
	template <class T>
	static auto test(int) -> decltype(T::Reflect(), std::true_type());

	template <class> static std::false_type test(...);

public:

	template<typename T>
	static constexpr bool HasReflectFunc();

	template <typename T>
	static auto GetReflectFunc()
	{
		return &T::Reflect;
	}
	};

template <typename T>
constexpr bool ReflectAccess::HasReflectFunc()
{
	return std::is_same<decltype(test<T>(0)), std::true_type>::value;
}
*/

#include <iostream>

namespace Reflection
{
	/*
#ifdef BX_PLATFORM_WINDOWS
	template<typename T>
	static constexpr bool sHasInternalReflect = ReflectAccess::HasReflectFunc<T>();

	template<typename T>
	static constexpr bool sHasExternalReflect = Reflector<T>::sIsSpecialized;
#else
	static_assert(false, "No platform");
#endif

	void RegisterReflectFunc(TypeId typeId, MetaType(*reflectFunc)());
	*/
	template<typename T>
	struct ReflectAtStartup
	{
		static bool Reflect()
		{
			std::cout << "HEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE";
			return true;
			/*static_assert(!sHasExternalReflect<T> || !sHasInternalReflect<T>, "Both an internal and external reflect function.");
			static_assert(sHasExternalReflect<T> || sHasInternalReflect<T>,
				R"(No external or internal reflect function. You need to make sure the Reflect function is included from wherever you are trying to reflect it.
If you are trying to reflect an std::vector<AssetHandle<Material>>, you need to include AssetHandle.h, Material.h and ReflectVector.h.)");

			if constexpr (sHasInternalReflect<T>)
			{
				Internal::RegisterReflectFunc(MakeTypeId<T>(), ReflectAccess::GetReflectFunc<T>());
			}
			else if constexpr (sHasExternalReflect<T>)
			{
				Internal::RegisterReflectFunc(MakeTypeId<T>(), &Reflector<T>::Reflect);
			}
			return true;*/
		}

		static bool g_reflectedAtStartup;
	};

	template<typename T>
	bool ReflectAtStartup<T>::g_reflectedAtStartup = ReflectAtStartup<T>::Reflect();
}

//template<typename T>
//static constexpr bool sIsReflectable = Reflection::sHasInternalReflect<T> != Reflection::sHasExternalReflect<T>;

#define CONCAT(a, b) CONCAT_INNER(a, b)
#define CONCAT_INNER(a, b) a ## b
#define GET_MACRO(_1, _2, NAME,...) NAME
#define REFLECT_AT_START_UP_2(name, type) const bool volatile CONCAT(g_reflected, name) = Reflection::ReflectAtStartup<type>::g_reflectedAtStartup;
#define REFLECT_AT_START_UP_1(type) REFLECT_AT_START_UP_2(type, type)

#define EXPAND( x ) x

#define REFLECT_AT_START_UP(...) EXPAND(GET_MACRO(__VA_ARGS__, REFLECT_AT_START_UP_2, REFLECT_AT_START_UP_1)(__VA_ARGS__))