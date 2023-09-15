# ARCHIVED - Moved to gitlab


# MpUnit C++ unit-test framework

## Introduction
An easy but featured testing free framework for C++, contained in **one header only**!\
It is inspired to _CppUnit_, since basically test classes  are subclasses of _MpuTest_\
and override the _Run()_ method, suites are subclasses of _MpuSuite_

You have _setup()/teardown()_ virtual methods for **both** tests and suites.\
Suites are a set of tests that share the same environment.

Inside tests you can put your assertions.

## Features
 - Just one header: No complex setup/installation
 - Highly Portable: Tested on windows, linux, freebsd, C++98/11/14/17,20,... (see [compatibility](#compatibility) section)
 - Very easy to use (see [examples](./examples))
 - Handle tests and suites (that are a set of tests)
 - Suites can set _setup()/teardown()_ methods before/after each test execution (test level)
 - Suites can set _SuiteSetup()/SuiteTeardown()_ before/after each suite execution (suite level)
 - Suppport stdout/file output
 - Support 4 different output formats (see [output formats](#output-formats) section)
 - Handle system exceptions (can be disabled)
 - Full support for unicode on windows (linux handle it automatically)

## Output Formats
 1. stdout (The default format)
 2. HTML (A single .html page with pie chart, legend and detailed tests report)
 3. JUNIT (The classic xml format used by test frameworks)
 4. NULL (To be completely quiet)

## Command Line
MpUnit supports some options at run-time, that can be set in code and overridden in cli.\
You can run your test executable with _-h_ to have a list of available options:
 - -h Show the help message
 - -f \<format\> Where format can be one of: _stdout_, _html_, _junit_, _null_
 - -o \<filename\> Write the output on _filename_
 - -x Disable the system exception handling (Define **MPU_NO_SEH** to disable at compile-time)
 - -e Stops execution at the first failed test

Except the _-h_ switch, all others can be set to the _MpUnit_ object:
```cpp
MpUnit mpunit;
mpunit.outputFormat = MPU_OUTPUT_HTML; // MPU_OUTPUT_STDOUT is default, see enum MpuOutputType
mpunit.outputFile = "out.txt";
mpunit.enableSystemExceptions = false; // default is true
mpunit.stopOnError = true; // default is false
// addsuites/tests...
mpunit.run(argc, argv); // pass argv/argc (optional) to make mpunit handle cli
```

## Compatibility
MpUnit has been tested with several compilers, from recent to very old ones:
 - g++ (mingw, cygwin, linux)
 - clang (windows, freebsd)
 - Visual Studio
 - Digitalmars (unicode do not works)
 - Borland 5.5.1 (Define MPU_NO_SEH, unicode do not works). Newer versions should be better

## Notes
 - All available assertions begin with _MPU_ASSERT_...\
   There is also _MPU_INFO_ to dump a custom message, and a **non-fatal** assertion _MPU_WARNING_
 - Tests/suites are executed in the order they are added. You can use the _Shuffle()_ method on both\
   _MpUnit_ and _MpuSuite_-derived objects to randomize tests execution order
 - You can query status with method _GetStatus_
 - You can reset the MpUnit object by calling method _Reset()_
