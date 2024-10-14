#include <components/sprite.hpp>

#include <rttr/registration.h>
RTTR_REGISTRATION
{
	rttr::registration::class_<Sprite>("Sprite").constructor();
}

Sprite::Sprite()
{
}