// 简单的 GTest 验证测试
#include <gtest/gtest.h>

// 简单的测试用例来验证 GTest 是否工作
TEST(GTestVerification, BasicTest) {
    EXPECT_EQ(1 + 1, 2);
    EXPECT_TRUE(true);
    EXPECT_FALSE(false);
}

TEST(GTestVerification, StringTest) {
    std::string hello = "Hello";
    std::string world = "World";
    EXPECT_EQ(hello + " " + world, "Hello World");
}
