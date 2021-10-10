#include "mpunit.hpp"

/*
The basic test, essentially a subclass of MpuTest, just add assertions and run
*/

class Test : public MpuTest
{
    void Run() {
        std::string s("Hello, ");
        MPU_INFO(s + "world!");

        // See file mpunit.hpp, all assertions start with MPU_ASSERT_...

        MPU_ASSERT_FALSE(s.empty());

        MPU_WARNING(s.empty()); // trigger a warning

        MPU_ASSERT_TRUE(s.empty()); // trigger a test error

        MPU_ASSERT_TRUE(false); // never called
    }
};

int main(int argc, char* argv[])
{
    MpUnit mpu;
    Test test;
    mpu.AddTest(test);
    mpu.Run(argc, argv); // Add support to cli
    return 0;
}
