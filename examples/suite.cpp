#include "mpunit.hpp"

/*
Explore name and arguments, that can be set to tests and suites.
Note that arguments must be pointers to anything
*/

// Suite CAN override this 4 methods
class Suite : public MpuSuite
{
    void SuiteSetup() {
        std::cout << std::endl
                  << "Suite setup, called once before first test of the suite"
                  << std::endl;
    }

    void Setup() {
        std::cout << "Test setup, called before EACH test";
    }

    void Teardown()  {
        std::cout << std::endl << "Test teardown, called after EACH test"
                  << std::endl;
    }

    void SuiteTeardown()  {
        std::cout << "Suite Teardown, called once after last test" << std::endl;
    }
};

class Test : public MpuTest
{
    void Run() {
        int total = 10;
        const int* testArgument = GetArgument<int*>();
        const int* suiteArgument = GetSuiteArgument<int*>();
        MPU_ASSERT_EQUAL(total, *testArgument + *suiteArgument);
    }
};

int main(void)
{
    MpUnit mpu("Suite example"); // Set battery name
    int x = 6;
    int y = 4;

    // Set name and argument to suite
    Suite suite;
    suite.name = "Suite name";
    suite.SetArgument(&y); // All tests will receive this as suite argument

    // Set name and argument to test
    Test test;
    test.name = "Sum test";
    test.SetArgument(&x);

    // Add test to suite and suite to main obj
    suite.AddTest(test);
    mpu.AddSuite(suite);

    mpu.Run();

    return 0;
}
