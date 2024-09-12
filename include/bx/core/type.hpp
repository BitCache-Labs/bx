#pragma once

#include "bx/engine/core/byte_types.hpp"
#include "bx/engine/core/meta.hpp"
#include "bx/engine/core/hash.hpp"
#include "bx/engine/core/macros.hpp"
#include "bx/engine/containers/string.hpp"

#include <wnaabi/type_info.hpp>

using TypeId = SizeType;
constexpr TypeId INVALID_TYPEID = -1;

template <typename TType>
class TypeImpl
{
private:
	template <typename T>
	friend class Type;

	static TypeId Id()
	{
		static Hash<String> hashFn;
		static const TypeId id = hashFn(ClassName());
		return id;
	}

	static String ClassName()
	{
		//return BX_FUNCTION;
		return wnaabi::type_info<TType>::name_tokens(wnaabi::runtime_visitors::stringify_t{}).str;
	}
};

template <typename TType>
class Type
{
public:
	static TypeId Id()
	{
		return TypeImpl<meta::decay_t<TType>>::Id();
	}

	static String ClassName()
	{
		return TypeImpl<meta::decay_t<TType>>::ClassName();
	}
};

// Enum -----------------------------------------------

#include "bx/engine/containers/list.hpp"

//#include <cereal/cereal.hpp>
namespace cereal { class access; }

#include <sstream>
#include <regex>

namespace enum_detail
{
	template <typename T>
	static List<T> get_handles(const char* args)
	{
		List<T> handles;

		std::string input = args;

		if (input.find('=') != std::string::npos)
		{
			// Regular expression to match "name = value" pairs
			std::regex re(R"(\s*([^=]+)\s*=\s*([0-9]+)\s*)");
			std::sregex_iterator iter(input.begin(), input.end(), re);
			std::sregex_iterator end;

			// Iterate through all matches
			for (; iter != end; ++iter)
			{
				std::smatch match = *iter;
				String name = match[1].str();
				int value = std::stoi(match[2].str());

				// Trim whitespace from the name
				name.erase(name.find_last_not_of(" \n\r\t") + 1);
				name.erase(0, name.find_first_not_of(" \n\r\t"));

				handles.emplace_back(T{ name, value });
			}
		}
		else
		{
			// Regular expression to match names separated by commas
			std::regex re(R"(\s*([^,]+)\s*)");
			std::sregex_iterator iter(input.begin(), input.end(), re);
			std::sregex_iterator end;

			int i = 0;
			for (; iter != end; ++iter) {
				std::smatch match = *iter;
				String name = match[1].str();
				int value = i++;

				// Trim whitespace from the name
				name.erase(name.find_last_not_of(" \n\r\t") + 1);
				name.erase(0, name.find_first_not_of(" \n\r\t"));

				handles.emplace_back(T{ name, value });
			}
		}

		return handles;
	}

	template <typename T>
	static List<const char*> split_names(const List<T>& handles)
	{
		List<const char*> names;
		for (const auto& handle : handles)
			names.emplace_back(handle.name.c_str());
		return names;
	}

	template <typename T>
	static List<int> split_values(const List<T>& handles)
	{
		List<int> values;
		for (const auto& handle : handles)
			values.emplace_back(handle.value);
		return values;
	}

	template <typename T>
	static int get_index(const List<T>& handles, int value)
	{
		for (int i = 0; i < handles.size(); ++i)
			if (handles[i].value == value)
				return i;
		return 0;
	}

	// TODO: Make this solution constexpr and template only.
	//// See: https://stackoverflow.com/questions/7124969/recursive-variadic-template-to-print-out-the-contents-of-a-parameter-pack
	//template <std::size_t Count>
	//static std::array<std::string, Count> get_names(const char* args)
	//{
	//    std::array<std::string, Count> names;
	//
	//    std::string nameArgs = args;
	//    std::string temp;
	//    std::istringstream ss(nameArgs);
	//    for (std::size_t i = 0; i < Count; ++i)
	//    {
	//        std::getline(ss, temp, ',');
	//        while (temp.front() == ' ')
	//            temp.erase(0, 1);
	//        names[i] = temp;
	//    }
	//
	//    return names;
	//}
	//// See: https://stackoverflow.com/questions/2124339/c-preprocessor-va-args-number-of-arguments
	//template<typename ...Args>
	//static constexpr std::size_t va_count(Args&&...) { return sizeof...(Args); }
}

#ifndef MEMORY_CUSTOM_CONTAINERS

// See: https://stackoverflow.com/questions/21295935/can-a-c-enum-class-have-methods
#define ENUM(Enum, ...) \
    struct Enum \
    { \
        enum Value { __VA_ARGS__ }; \
        Enum() = default; \
        constexpr Enum(Value v) : value(v) { } \
        constexpr Enum(int v) : value((Value)v) { } \
        operator Value() const { return value; } \
		explicit operator bool() = delete; \
        constexpr bool operator ==(Enum rhs) const { return value == rhs.value; } \
        constexpr bool operator !=(Enum rhs) const { return value != rhs.value; } \
        constexpr bool operator ==(Value rhs) const { return value == rhs; } \
        constexpr bool operator !=(Value rhs) const { return value != rhs; } \
		struct Handle { String name; int value; }; \
		inline int GetIndex() const { return enum_detail::get_index(GetHandles(), (int)value); } \
		static const List<const char*>& GetNames() { static List<const char*> names = enum_detail::split_names<Handle>(GetHandles()); return names; } \
		static const List<int>& GetValues() { static List<int> values = enum_detail::split_values<Handle>(GetHandles()); return values; } \
		static const List<Handle>& GetHandles() { static List<Handle> handles = enum_detail::get_handles<Handle>(#__VA_ARGS__); return handles; } \
        private: \
		friend struct std::hash<Enum>; \
        friend class cereal::access; \
		template<class Archive> void serialize(Archive& archive) { archive(value); } \
		Value value; \
    }; \
	namespace std { template <> struct hash<Enum> { inline std::size_t operator()(const Enum& e) const { return e.value; } }; }

#else

#define ENUM_HASH(Enum) \
template <> struct Hash<Enum> { inline SizeType operator()(const Enum& e) const { return e.value; } };

#endif


// --------------------------------------------------------------------------------
// --------------------------------- EXPERIMENTAL ---------------------------------
// --------------------------------------------------------------------------------
 
// TODO: WIP, DON'T USE!
template <typename TType>
class TypeInfo
{
public:
	static TypeId Id()
	{
		return TypeImpl<meta::decay_t<TType>>::Id();
	}

	static const char* Info()
	{
		return TType::Info();
	}
};

#define TYPE(Type) \
	const char TypeName_##Type[] { #Type };

#define CLASS(Class) \
    TYPE(Class) class Class \

#define DEFINE(Type) \
	private: template <typename T> friend class TypeInfo; \
	static const char* Info() { return TypeName_##Type; }

#define STRUCT(Struct) \
	struct Struct


// WIP, this is also an experiment, DONT USE!
// If this works then perhaps classes can be inherited using this way to introduce reflection
// E.g. class A : public Class<A, PublicDerived1, PublicDerived2>, private DerivedPrivate
// Note, this pattern might also be useful for structs and enums.
template <typename TClass, typename... TBases>
class Class : public TBases...
{
public:
};

// In case we need to set visibility in the bases...
//template <typename Base, typename Visibility>
//struct BaseWithVisibility;
//
//template <typename Base>
//struct BaseWithVisibility<Base, std::true_type> : public Base {};
//
//template <typename Base>
//struct BaseWithVisibility<Base, std::false_type> : private Base {};
//
//// Template class inheriting from multiple base classes with specified visibility
//template <typename... Bases>
//class MultiDerived : public BaseWithVisibility<Bases, std::true_type>... { // Use std::true_type for public inheritance
//public:
//	using BaseWithVisibility<Bases, std::true_type>::showA...;
//	using BaseWithVisibility<Bases, std::true_type>::showB...;
//	using BaseWithVisibility<Bases, std::true_type>::showC...;
//};

// --------------------------------------------------------------------------------
// --------------------------------- EXPERIMENTAL ---------------------------------
// --------------------------------------------------------------------------------