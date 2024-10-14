#include <systems/drawer.hpp>

#include <rttr/registration.h>
RTTR_REGISTRATION
{
	rttr::registration::class_<Drawer>("Drawer").constructor();
}

Drawer::Drawer()
{
}

void Drawer::Initialize()
{
}

void Drawer::Shutdown()
{
}

void Drawer::Update()
{
}

void Drawer::Render()
{
}