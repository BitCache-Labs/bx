#pragma once

#include <engine/macros.hpp>

#define BX_VERSION_MAJOR 0
#define BX_VERSION_MINOR 1
#define BX_VERSION_DEV 0

#define BX_VERSION_STR \
BX_XSTR(VERSION_MAJOR) "." \
BX_XSTR(VERSION_MINOR) "." \
BX_XSTR(VERSION_DEV)