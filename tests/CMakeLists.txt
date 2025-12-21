set(bx_test_srcs)
set(bx_test_libs)

if (BX_CORE)
    set(bx_test_srcs ${bx_test_srcs}
        "bx_core/bx_core_test.cpp"
    )
    set(bx_test_libs ${bx_test_libs}
        bx_core)
endif()

if (BX_STL)
    set(bx_test_srcs ${bx_test_srcs}
        "bx_stl/array_test.cpp"
    )
    set(bx_test_libs ${bx_test_libs}
        bx_stl)
endif()

if (BX_MATH)
    set(bx_test_srcs ${bx_test_srcs}
        "bx_math/bx_math_test.cpp"
    )
    set(bx_test_libs ${bx_test_libs}
        bx_math)
endif()

add_executable(bx_tests ${bx_test_srcs})

target_link_libraries(bx_tests
    PRIVATE
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
        ${bx_test_libs}
)

include(GoogleTest)
gtest_discover_tests(bx_tests)