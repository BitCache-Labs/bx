#pragma once

#define BX_STR(X) #X
#define BX_XSTR(X) BX_STR(X)

#define BX_CONCAT_IMPL(A, V) A##B
#define BX_CONCAT(A, B) BX_CONCAT_IMPL(A, B)

#define BX_ARRAYSIZE(ARR) ((unsigned)(sizeof(ARR) / sizeof(*(ARR))))

#define BX_BIT(x) (1 << (x))