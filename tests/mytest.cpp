#include<gtest/gtest.h>
#include<iostream>
using namespace std;
// #include "main.h"

// TEST(testCase1,add_test)
// {
//     EXPECT_EQ(add_func_for_test(2,1),3);
// }


int main(int argc,char **argv)
{
    testing::InitGoogleTest(&argc,argv);
    return RUN_ALL_TESTS();
}

