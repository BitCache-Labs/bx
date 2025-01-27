#pragma once

#define STR(Str) #Str
#define XSTR(Str) STR(Str)

#define BX_CONCAT_IMPL(A, V) A##B
#define BX_CONCAT(A, B) BX_CONCAT_IMPL(A, B)

#define ARRAYSIZE(Arr) ((unsigned)(sizeof(Arr) / sizeof(*(Arr))))

#define BIT(x) (1 << (x))