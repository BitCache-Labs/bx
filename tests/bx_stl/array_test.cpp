#include <gtest/gtest.h>
#include <bx/array.hpp>

using namespace bx;

//
// array_fixed tests
//
TEST(array_fixed, basic_access)
{
    array_fixed<int, 4> a{ 1, 2, 3, 4 };

    EXPECT_EQ(a.size(), 4u);
    EXPECT_EQ(a[0], 1);
    EXPECT_EQ(a[3], 4);
}

TEST(array_fixed, iteration)
{
    array_fixed<int, 3> a{ 1, 2, 3 };

    int sum = 0;
    for (int v : a)
        sum += v;

    EXPECT_EQ(sum, 6);
}

//
// array tests
//
TEST(array, default_constructed)
{
    array<int> a;

    EXPECT_TRUE(a.empty());
    EXPECT_EQ(a.size(), 0u);
}

TEST(array, resize)
{
    array<int> a;
    a.resize(3);

    EXPECT_EQ(a.size(), 3u);

    a[0] = 10;
    a[1] = 20;
    a[2] = 30;

    EXPECT_EQ(a[1], 20);

    // Shrink
    a.resize(2);
    EXPECT_EQ(a.size(), 2u);
    EXPECT_EQ(a[1], 20);
}

TEST(array, push_back_and_emplace)
{
    array<int> a;

    a.push_back(1);
    a.push_back(2);
    a.push_back(3);
    a.emplace_back(4);

    EXPECT_EQ(a.size(), 4u);
    EXPECT_EQ(a[3], 4);
}

TEST(array, pop_back_and_clear)
{
    array<int> a;
    a.push_back(5);
    a.push_back(6);

    a.pop_back();
    EXPECT_EQ(a.size(), 1u);
    EXPECT_EQ(a[0], 5);

    a.clear();
    EXPECT_TRUE(a.empty());
}

TEST(array, move_semantics)
{
    array<int> a;
    a.push_back(42);

    array<int> b = std::move(a);

    EXPECT_EQ(b.size(), 1u);
    EXPECT_EQ(b[0], 42);
    EXPECT_TRUE(a.empty());
}

//
// array_view tests
//
TEST(array_view, from_array)
{
    array<int> a;
    a.push_back(5);
    a.push_back(6);

    array_view<int> v(a);

    EXPECT_TRUE(v);
    EXPECT_EQ(v.size(), 2u);
    EXPECT_EQ(v[1], 6);

    // iteration
    int sum = 0;
    for (auto x : v)
        sum += x;
    EXPECT_EQ(sum, 11);
}

TEST(array_view, from_array_fixed)
{
    array_fixed<int, 2> a{ 7, 8 };
    array_view<int> v(a);

    EXPECT_EQ(v.size(), 2u);
    EXPECT_EQ(v[0], 7);
}

TEST(array_view, from_c_array)
{
    int carray[] = { 1, 2, 3 };
    array_view<int> v(carray);

    EXPECT_EQ(v.size(), 3u);
    EXPECT_EQ(v[2], 3);
}

TEST(array_view, bool_operator)
{
    array_fixed<int, 0> empty_arr{};
    array_view<int> v_empty(empty_arr);

    EXPECT_FALSE(v_empty);

    array_fixed<int, 1> single{ 42 };
    array_view<int> v_single(single);

    EXPECT_TRUE(v_single);
}