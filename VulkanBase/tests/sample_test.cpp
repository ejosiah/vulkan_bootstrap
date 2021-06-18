//
// Created by Josiah on 18/06/2021.
//

#include <gtest/gtest.h>

int factorial(int n){
    if(n <= 1) return 1;
    return n * factorial(n - 1);
}



TEST(FactorialTest, HandleZeroInput){
    EXPECT_EQ(factorial(0), 1);
}

TEST(FactorialTest, HandlesPositiveInput) {
    EXPECT_EQ(factorial(1), 1);
    EXPECT_EQ(factorial(2), 2);
    EXPECT_EQ(factorial(3), 6);
    EXPECT_EQ(factorial(8), 40320);
}

//int main(int argc, char** argv)
//{
//    testing::InitGoogleTest(&argc, argv);
//    RUN_ALL_TESTS();
//}