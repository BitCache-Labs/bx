#pragma once

#include <engine/api.hpp>

#include <rttr/type.h>
#include <rttr/rttr_enable.h>
#include <rttr/registration.h>
#include <rttr/registration_friend.h>
#include <rttr/rttr_cast.h>

#define BX_ENUM_TYPE(Name) enum struct BX_API Name

#define BX_TYPE(Name, ...) RTTR_ENABLE(__VA_ARGS__)

#define BX_TYPE_REGISTRATION RTTR_REGISTRATION
#define BX_TYPE_REGISTRATION_FRIEND RTTR_REGISTRATION_FRIEND

#define BX_PRIVATE(Name)                        \
private:                                        \
    Name() = default;                           \
    ~Name() = default;

#define BX_INTERFACE(Name)                      \
protected:                                      \
    Name() = default;                           \
public:                                         \
    virtual ~Name() = default;

#define BX_NOCOPY(Name)                         \
private:                                        \
    Name(const Name&) = delete;                 \
    Name& operator=(const Name&) = delete;      \
    Name(Name&&) = delete;                      \
    Name& operator=(Name&&) = delete;

#define BX_SINGLETON(Name)                     \
    BX_PRIVATE(Name)                           \
    BX_NOCOPY(Name)

using TypeId = rttr::type::type_id;

template <typename T>
struct BX_API Type
{
    static TypeId Id() { return rttr::type::get<T>().get_id(); }
};