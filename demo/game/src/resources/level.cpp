#include <resources/level.hpp>

#include <rttr/registration.h>
RTTR_REGISTRATION
{
	rttr::registration::class_<Level>("Level").constructor();
}

Level::Level()
{
}