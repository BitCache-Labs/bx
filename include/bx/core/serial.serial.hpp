#pragma once

#include "bx/engine/core/serial.hpp"
#include "bx/engine/core/type.hpp"

#include <cereal/archives/binary.hpp>
#include <cereal/archives/portable_binary.hpp>
#include <cereal/archives/xml.hpp>
#include <cereal/archives/json.hpp>

#include <cereal/types/polymorphic.hpp>

#define SERIAL_OP_ARCHIVE(Expr, Message) try { Expr; } catch (const cereal::Exception& e) { BX_LOGW(Message); }

#define REGISTER_SERIAL(TType) \
template<class Archive> \
void save(Archive& ar, const TType& data) \
{ \
try { \
	Serial<TType>::Save(ar, data); \
} catch (cereal::Exception& e) { BX_LOGW("Failed to save type: {}. {}", Type<TType>::ClassName(), e.what()); } \
} \
template<class Archive> \
void load(Archive& ar, TType& data) \
{ \
try { \
	Serial<TType>::Load(ar, data); \
} catch (cereal::Exception& e) { BX_LOGW("Failed to load type: {}. {}", Type<TType>::ClassName(), e.what()); } \
}

#define REGISTER_SERIAL_T(TType) \
template<class Archive, typename T> \
void save(Archive& ar, const TType<T>& data) \
{ \
try { \
	Serial<TType<T>>::Save(ar, data); \
} catch (cereal::Exception& e) { BX_LOGW("Failed to save type: {}. {}", Type<TType<T>>::ClassName(), e.what()); } \
} \
template<class Archive, typename T> \
void load(Archive& ar, TType<T>& data) \
{ \
try { \
	Serial<TType<T>>::Load(ar, data); \
} catch (cereal::Exception& e) { BX_LOGW("Failed to load type: {}. {}", Type<TType<T>>::ClassName(), e.what()); } \
}

#define REGISTER_POLYMORPHIC_SERIAL(TBase, TType) \
REGISTER_SERIAL(TType) \
CEREAL_REGISTER_TYPE(TType); \
CEREAL_REGISTER_POLYMORPHIC_RELATION(TBase, TType)

//#define REGISTER_POLYMORPHIC_SERIAL_T(Base, Type, T)