#include "mpunit.hpp"

/*
Show advanced topics: name/argument in constructor, advanced assertions,
cross-platform unicode, output format/file, commandline, status, objects scope

UNICODE:
Save this file as utf-8 without BOM.
For unicode on windows, consider mpuchar/mpustring/mpucout and U("string")
Compile with /utf-8 with visual studio and define _UNICODE AND UNICODE
Use flags -municode with g++/clang++
Unicode in linux/bsd is automatically and silently supported...

COMMANDLINE:
When you generate an executable, run it with -h to see options
You can also choose which tests/suites to run by specifying them on cli
Pass argc/argv to Run() method
*/


class Test : public MpuTest
{
public:

    Test(const mpustring& tname, void* arg=mpu_null)
        : MpuTest(tname, arg) {} // MpuTest/MpuSuite accept name and argument

private:

    void not_throw() {
        if (this->argument == this) {
            throw U("Should not happen");
        } 
    }

    void Run() {
        const mpustring info(U("Hello in armenian: Բարեւ"));
        MPU_INFO(info);

        float a = 1.0f;
        float b = 2.0f;

        // a and b are equal with a tolerance of 5!
        MPU_ASSERT_FLOAT_EQUAL(a, b, 5.0f);

        // a and b are not equal with a tolerance of 0.9!
        MPU_ASSERT_FLOAT_NOT_EQUAL(a, b, 0.9f);

        MPU_ASSERT_THROW(throw 0);
        MPU_ASSERT_NOT_THROW(not_throw());
        
        MPU_ASSERT_THROW_EX(throw false, bool);
        MPU_ASSERT_NOT_THROW_EX(not_throw(), bool);
        MPU_ASSERT_NOT_THROW_EX(throw MPUSTR("throw a string"), Test);
        
        mpuchar* pNull = mpu_null; // 0 or nullptr, according to choosen c++ std
        *pNull = 0; // this cause a system exception, don't worry
    }
};

// SMART SCOPE: no need for user to keep tests/suites alive
static void addTest(MpUnit& mpu)
{
    Test test(U("Test name in russian: Имя теста"));
    mpu.AddTest(test);
}

// Add cli support to unicode application and pass options to Mpunit
int MPU_MAIN(int argc, mpuchar* argv[])
{
    MpUnit mpu(U("Battery example in georgian: ბატარეის მაგალითი"));

    addTest(mpu); // add test, see note above

    // This is the default, see enum MpuOutputType
    mpu.outputFormat = MPU_OUTPUT_STDOUT;

    // uncomment to write on file instead of stdout
    // mpu.outputFile = U("std_Иван_.txt");

    mpu.Run(argc, argv);

    MpuStatus status; // query status
    mpu.GetStatus(status);
    mpucout << std::endl << U("You run ") << status.testsNum << U(" tests")
            << std::endl;

    return 0;
}
