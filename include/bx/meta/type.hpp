#pragma once

#include <string>
#include <vector>

#include <bx/core/byte_types.hpp>
#include "bx/core/hash.hpp"
#include "bx/core/macros.hpp"
#include "bx/containers/string.hpp"
#include "bx/containers/list.hpp"
#include "bx/containers/hash_map.hpp"

#include "bx/meta/meta.hpp"

using TypeId = u32;
constexpr TypeId INVALID_TYPEID = -1;

/*
template<typename T>
CONSTEVAL TypeId MakeTypeId();

template<typename T>
CONSTEVAL std::string_view MakeTypeName();

enum class TypeForm
{
	// Random prime numbers are assigned for better hashing of typetraits

	Value = 3574847,
	Ref = 2062657,
	ConstRef = 51182539,
	Ptr = 71661047,
	ConstPtr = 99105001,
	RValue = 1901583,
};

struct TypeTraits
{
	TypeId mStrippedTypeId{};
	TypeForm mForm{ TypeForm::Value };

	constexpr u32 Hash() const;

	constexpr auto operator==(const TypeTraits& other) const { return mStrippedTypeId == other.mStrippedTypeId && mForm == other.mForm; }
	constexpr auto operator!=(const TypeTraits& other) const { return mStrippedTypeId != other.mStrippedTypeId || mForm != other.mForm; }
};
static_assert(sizeof(TypeTraits) == sizeof(uint64));
*/

struct TypeInfo
{
	/*constexpr*/ TypeInfo() = default;
	/*constexpr*/ TypeInfo(TypeId typeId, u32 flags)
		: mTypeId(typeId)
		, mFlags(flags)
	{}
	/*constexpr*/ TypeInfo(
		TypeId typeId,
		u32 size,
		u32 alignment,
		bool isTriviallyDefaultConstructible,
		bool isTriviallyMoveConstructible,
		bool isTriviallyCopyConstructible,
		bool isTriviallyCopyAssignable,
		bool isTriviallyMoveAssignable,
		bool isDefaultConstructible,
		bool isMoveConstructible,
		bool isCopyConstructible,
		bool isCopyAssignable,
		bool isMoveAssignable,
		bool isTriviallyDestructible,
		bool userBit = 0)
		: mTypeId(typeId)
		, mFlags(size
			| (alignment << TypeInfo::sAlignShift)
			| (isTriviallyDefaultConstructible ? IsTriviallyDefaultConstructible : 0)
			| (isTriviallyMoveConstructible ? IsTriviallyMoveConstructible : 0)
			| (isTriviallyCopyConstructible ? IsTriviallyCopyConstructible : 0)
			| (isTriviallyCopyAssignable ? IsTriviallyCopyAssignable : 0)
			| (isTriviallyMoveAssignable ? IsTriviallyMoveAssignable : 0)
			| (isDefaultConstructible ? IsDefaultConstructible : 0)
			| (isMoveConstructible ? IsMoveConstructible : 0)
			| (isCopyConstructible ? IsCopyConstructible : 0)
			| (isCopyAssignable ? IsCopyAssignable : 0)
			| (isMoveAssignable ? IsMoveAssignable : 0)
			| (isTriviallyDestructible ? IsTriviallyDestructible : 0)
			| (userBit ? UserBit : 0))
	{}

	static constexpr u32 sNumOfBitsForSize = 14;	// Max size is 16384
	static constexpr u32 sNumOfBitsForAlign = 6;	// Max alignment is 64
	static constexpr u32 sMaxSize = (1 << sNumOfBitsForSize) - 1;
	static constexpr u32 sMaxAlign = (1 << sNumOfBitsForAlign) - 1;

	static constexpr u32 sAlignShift = sNumOfBitsForSize;

	enum Masks : u32
	{
		Size = (1 << sNumOfBitsForSize) - 1,
		Align = ((1 << sNumOfBitsForAlign) - 1) << sAlignShift,
		IsTriviallyDefaultConstructible = 1 << 20,
		IsTriviallyMoveConstructible = 1 << 21,
		IsTriviallyCopyConstructible = 1 << 22,
		IsTriviallyCopyAssignable = 1 << 23,
		IsTriviallyMoveAssignable = 1 << 24,
		IsDefaultConstructible = 1 << 25,
		IsMoveConstructible = 1 << 26,
		IsCopyConstructible = 1 << 27,
		IsCopyAssignable = 1 << 28,
		IsMoveAssignable = 1 << 29,
		IsTriviallyDestructible = 1 << 30,
		UserBit = 1u << 31,
	};
	//static_assert((Size & Align) == 0);
	//static_assert((Align & IsTriviallyDefaultConstructible) == 0);

	constexpr u32 GetSize() const { return mFlags & Size; }
	constexpr u32 GetAlign() const { return (mFlags & Align) >> sAlignShift; }

	TypeId mTypeId{};
	u32 mFlags{};
};
//static_assert(sizeof(TypeInfo) == 8);

/*
constexpr bool CanFormBeNullable(TypeForm form);

template<typename T>
CONSTEVAL TypeTraits MakeTypeTraits();

template<typename T>
CONSTEVAL TypeInfo MakeTypeInfo();

template<typename T>
CONSTEVAL TypeForm MakeTypeForm();

template<typename T>
CONSTEVAL TypeId MakeStrippedTypeId();
*/

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
		return BX_FUNCTION;
		//return wnaabi::type_info<TType>::name_tokens(wnaabi::runtime_visitors::stringify_t{}).str;
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

#include "bx/meta/function.hpp"

class MetaType
{
private:
	MetaType(TypeId typeId, const String& name)
		: m_typeId(typeId)
		, m_name(name)
	{}

public:
	template <typename T>
	static MetaType Create(const String& name)
	{
		return MetaType(Type<T>::Id(), name);
	}

	const std::string& GetName() const { return m_name; }
	u32 GetTypeId() const { return m_typeId; }

	template <typename TFunc, TFunc Func>
	inline void AddFunction(const String& name)
	{
		// Add to the function registry
		m_functions.insert(std::make_pair(name, MetaFunc::Create<TFunc, Func>(name)));
	}

	//inline MetaFunc GetFunction(const String& name) const
	//{
	//	auto it = m_functions.find(name);
	//	//if (it == m_functions.end()) return {};
	//	BX_ENSURE(it != m_functions.end());
	//	return it->second;
	//}

private:
	TypeId m_typeId{ INVALID_TYPEID };
	String m_name;

	//HashMap<String, MetaFunc> m_functions;
};

/*
	//MetaType(TypeInfo typeInfo, const std::string& name)
	//	: mTypeInfo(typeInfo)
	//	, mName(name)
	//	//, mProperties(std::make_unique<MetaProps>())
	//{}

	template<typename TypeT>
	struct T {};

	template<typename BaseType>
	struct Base {};

	template<typename... Args>
	struct Ctor {};

	template<typename TypeT, typename ...Args>
	MetaType(T<TypeT>, std::string_view name, Args&& ... args);

	MetaType(MetaType&& other) noexcept;
	MetaType(const MetaType&) = delete;

	~MetaType();

	MetaType& operator=(const MetaType&) = delete;
	MetaType& operator=(MetaType&&) = delete;

	bool operator==(const MetaType& other) const { return GetTypeId() == other.GetTypeId(); }
	bool operator!=(const MetaType& other) const { return GetTypeId() != other.GetTypeId(); }
	auto operator<(const MetaType& other) const { return GetTypeId() < other.GetTypeId(); }

	TypeInfo GetTypeInfo() const { return mTypeInfo; }
	void SetTypeInfo(TypeInfo typeInfo) { mTypeInfo = typeInfo; }

	template<typename... Args>
	MetaField& AddField(Args&& ...args);

	template<typename FuncPtr, typename... Args>
	MetaFunc& AddFunc(FuncPtr&& funcPtr, MetaFunc::NameOrTypeInit nameOrType, Args&& ...args);

	void AddBaseClass(const MetaType& baseClass);

	template<typename TypeT>
	void AddBaseClass();

	bool IsExactly(const u32 typeId) const { return GetTypeId() == typeId; }

	template<typename TypeT>
	bool IsExactly() const { return IsExactly(MakeTypeId<TypeT>()); }

	bool IsDerivedFrom(TypeId baseClassTypeId) const;

	template<typename TypeT>
	bool IsDerivedFrom() const { return IsDerivedFrom(MakeTypeId<TypeT>()); }

	bool IsBaseClassOf(TypeId derivedClassTypeId) const;

	template<typename TypeT>
	bool IsBaseClassOf() const { return IsBaseClassOf(MakeTypeId<TypeT>()); }

	template<typename... Args>
	FuncResult Construct(Args&&... args) const;

	template<typename... Args>
	FuncResult ConstructAt(void* atAdress, Args&&... args) const;

	void Destruct(void* ptrToObjectOfThisType, bool freeBuffer) const;

	std::vector<std::reference_wrapper<const MetaField>> EachField() const;

	std::vector<std::reference_wrapper<MetaField>> EachField();

	std::vector<std::reference_wrapper<const MetaFunc>> EachFunc() const;

	std::vector<std::reference_wrapper<MetaFunc>> EachFunc();

	const std::vector<std::reference_wrapper<const MetaType>>& GetDirectBaseClasses() const { return mDirectBaseClasses; }

	const std::vector<std::reference_wrapper<const MetaType>>& GetDirectDerivedClasses() const { return mDirectDerivedClasses; }

	std::vector<MetaField>& GetDirectFields() { return mFields; }

	const std::vector<MetaField>& GetDirectFields() const { return mFields; }

	std::vector<std::reference_wrapper<MetaFunc>> GetDirectFuncs();

	std::vector<std::reference_wrapper<const MetaFunc>>  GetDirectFuncs() const;

	const std::string& GetName() const { return mName; }
	u32 GetTypeId() const { return mTypeInfo.mTypeId; }

	MetaField* TryGetField(Name name);
	const MetaField* TryGetField(Name name) const;

	MetaFunc* TryGetFunc(const std::variant<Name, OperatorType>& nameOrType);
	const MetaFunc* TryGetFunc(const std::variant<Name, OperatorType>& nameOrType) const;

	template<typename... Args>
	FuncResult CallFunction(const std::variant<Name, OperatorType>& funcNameOrType, Args&&... args) const;

	template<typename... Args>
	FuncResult CallFunctionWithRVO(const std::variant<Name, OperatorType>& funcNameOrType, MetaFunc::RVOBuffer rvoBuffer, Args&&... args) const;

	FuncResult CallFunction(const std::variant<Name, OperatorType>& funcNameOrType, Span<MetaAny> args, Span<const TypeForm> formOfArgs, MetaFunc::RVOBuffer rvoBuffer = nullptr) const;

	MetaFunc* TryGetFunc(const std::variant<Name, OperatorType>& nameOrType, u32 funcId);

	const MetaFunc* TryGetFunc(const std::variant<Name, OperatorType>& nameOrType, u32 funcId) const;

	const MetaProps& GetProperties() const { return *mProperties; }
	MetaProps& GetProperties() { return *mProperties; }

	size_t RemoveFunc(const std::variant<Name, OperatorType>& nameOrType);

	size_t RemoveFunc(const std::variant<Name, OperatorType>& nameOrType, u32 funcId);

	void* Malloc(u32 amountOfObjects = 1) const;
	static void Free(void* buffer);

	u32 GetSize() const { return mTypeInfo.GetSize(); }
	u32 GetAlignment() const { return mTypeInfo.GetAlign(); }

	bool IsConstructible(const std::vector<TypeTraits>& parameters) const { return TryGetConstructor(parameters) != nullptr; }

	template<typename... Args>
	bool IsConstructible() const;

	bool IsDefaultConstructible() const { return mTypeInfo.mFlags & TypeInfo::IsDefaultConstructible; }
	bool IsMoveConstructible() const { return mTypeInfo.mFlags & TypeInfo::IsMoveConstructible; }
	bool IsCopyConstructible() const { return mTypeInfo.mFlags & TypeInfo::IsCopyConstructible; }
	bool IsMoveAssignable() const { return mTypeInfo.mFlags & TypeInfo::IsMoveAssignable; }
	bool IsCopyAssignable() const { return mTypeInfo.mFlags & TypeInfo::IsCopyAssignable; }

	const MetaFunc* TryGetConstructor(const std::vector<TypeTraits>& parameters) const;
	const MetaFunc* TryGetDefaultConstructor() const;
	const MetaFunc* TryGetCopyConstructor() const;
	const MetaFunc* TryGetMoveConstructor() const;
	const MetaFunc* TryGetCopyAssign() const;
	const MetaFunc* TryGetMoveAssign() const;
	const MetaFunc& GetDestructor() const;

private:
	template<typename FieldType, typename Self>
	static std::vector<std::reference_wrapper<FieldType>> EachField(Self& self);

	template<typename FuncType, typename Self>
	static std::vector<std::reference_wrapper<FuncType>> EachFunc(Self& self);

	template<typename FuncType, typename Self>
	static std::vector<std::reference_wrapper<FuncType>> GetDirectFuncs(Self& self);

	template<typename TypeT, typename BaseT>
	void AddFromArg(Base<BaseT>) { AddBaseClass<BaseT>(); }

	template<typename TypeT, typename... Args>
	void AddFromArg(Ctor<Args...>);

	template<typename... Args>
	FuncResult ConstructInternalGeneric(bool isOwner, void* address, Args&&... args) const;

	template<typename... Args>
	FuncResult ConstructInternal(bool isOwner, void* address, Args&&... args) const { return ConstructInternalGeneric(isOwner, address, std::forward<Args>(args)...); }

	FuncResult ConstructInternal(bool isOwner, void* address) const;

	template<typename TypeT>
	FuncResult ConstructInternal(bool isOwner, void* address, const TypeT& args) const;

	template<typename TypeT>
	FuncResult ConstructInternal(bool isOwner, void* address, TypeT& args) const;

	template<typename TypeT, std::enable_if_t<std::is_rvalue_reference_v<TypeT>, bool> = true>
	FuncResult ConstructInternal(bool isOwner, void* address, TypeT&& args) const;

	FuncResult ConstructInternal(bool isOwner, void* address, const MetaAny& args) const;

	FuncResult ConstructInternal(bool isOwner, void* address, MetaAny& args) const;

	FuncResult ConstructInternal(bool isOwner, void* address, MetaAny&& args) const;

	struct FuncKey
	{
		FuncKey() = default;
		FuncKey(Name::HashType hashType);
		FuncKey(OperatorType operatorType);
		FuncKey(const MetaFunc::NameOrType& nameOrType);
		FuncKey(const std::variant<Name, OperatorType>& nameOrType);
		FuncKey(const std::variant<std::string_view, OperatorType>& nameOrType);

		OperatorType mOperatorType{};
		Name::HashType mNameHash{};

		bool operator==(const FuncKey& other) const { return mOperatorType == other.mOperatorType && mNameHash == other.mNameHash; };
		bool operator!=(const FuncKey& other) const { return mOperatorType != other.mOperatorType || mNameHash != other.mNameHash; };
	};

	struct FuncHasher
	{
		uint64 operator()(const FuncKey& k) const
		{
			return (static_cast<uint64>(k.mOperatorType) << 32) | static_cast<uint64>(k.mNameHash);
		}
	};

	//TypeInfo mTypeInfo{};
	TypeId mTypeId{ INVALID_TYPEID };

	//std::unordered_multimap<FuncKey, MetaFunc, FuncHasher> mFunctions{};
	//std::vector<MetaField> mFields{};

	//mutable std::vector<std::reference_wrapper<const MetaType>> mDirectBaseClasses{};
	//mutable std::vector<std::reference_wrapper<const MetaType>> mDirectDerivedClasses{};

	std::string mName;

	//std::unique_ptr<MetaProps> mProperties;
};*/