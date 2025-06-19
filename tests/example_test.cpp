#include <gtest/gtest.h>


TEST(ExampleTest, BasicAssertions) {
    EXPECT_EQ(7 * 6, 42);
    EXPECT_STRNE("hello", "world");
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv); //初始化、运行所有的test
    return RUN_ALL_TESTS();
}