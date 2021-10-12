#ifndef MPUNIT_HPP_
#define MPUNIT_HPP_

#include <string>
#include <vector>
#include <set>
#include <iostream>
#include <sstream>
#include <fstream>
#include <exception>
#include <algorithm>
#include <iomanip>
#include <ctime>
#include <cctype>
#include <cstring>

#if ((defined(_WIN32)) || (defined(__CYGWIN__)))
#include <Windows.h>
#else
#include <sys/time.h>
#define IsDebuggerPresent() false
#endif

#if __cplusplus >= 201103L
#include <random>
#define mpu_null nullptr
#define _mpu_override override
#define _mpu_noexcp noexcept
#else
#define mpu_null 0
#define _mpu_override
#define _mpu_noexcp throw()
#endif

#define MPUNIT_MAJOR 1
#define MPUNIT_MINOR 0


#if ((defined(UNICODE) || defined(_UNICODE)) && defined(_WIN32))
#define MPWUNICODE
#define mpu_main wmain
#include <wchar.h>
#include <io.h>
#include <fcntl.h>
#include <cstdio>
#define U(str) L##str
typedef std::wstring mpustring;
typedef std::wostringstream mpustringstream;
typedef std::wostream mpuostream;
typedef std::wofstream mpuofstream;
typedef wchar_t mpuchar;
static std::wostream& mpucout(std::wcout);
#define mpu_isprint(c) iswprint(c)
#else
#define mpu_main main
#define U(str) str
typedef std::string mpustring;
typedef std::ostringstream mpustringstream;
typedef std::ostream mpuostream;
typedef std::ofstream mpuofstream;
typedef char mpuchar;
static std::ostream& mpucout(std::cout);
#define mpu_isprint(c) isprint(static_cast<unsigned char>(c))
#endif
#define MPUSTR U

#ifdef __DMC__
extern "C" {
BOOL WINAPI IsDebuggerPresent();
}
#endif

#ifndef MPU_NO_SEH

#include <csetjmp>

static mpustring _mpuExceptionString;

#if (defined(_WIN32))

#include <map>

#define mpu_set_jump(env) setjmp(env)
#define mpu_jump(env, n)  longjmp(env, (n))

static jmp_buf _mpuPosition;

static LPTOP_LEVEL_EXCEPTION_FILTER _mpuPreviousExceptionFilter;

static mpustring _mpuGetExceptionString(DWORD code) {
    static std::map<DWORD, mpustring> m;
    static std::map<DWORD, mpustring>::const_iterator iter, ender;
    mpustring msg;
    if (m.empty()) {
        m[EXCEPTION_ACCESS_VIOLATION] = MPUSTR("Access violation");
        m[EXCEPTION_ARRAY_BOUNDS_EXCEEDED] = MPUSTR("Array out of bound access");
        m[EXCEPTION_DATATYPE_MISALIGNMENT] = MPUSTR("Misaligned memory access");
        m[EXCEPTION_FLT_DENORMAL_OPERAND] = MPUSTR("Denormal floating-point operand");
        m[EXCEPTION_FLT_DIVIDE_BY_ZERO] = MPUSTR("Floating-point division by ZERO");
        m[EXCEPTION_FLT_INEXACT_RESULT] = MPUSTR("Unrepresentable floating-point operation");
        m[EXCEPTION_FLT_INVALID_OPERATION] = MPUSTR("Floating-point exception");
        m[EXCEPTION_FLT_OVERFLOW] = MPUSTR("Floating-point overflow");
        m[EXCEPTION_FLT_STACK_CHECK] = MPUSTR("Floating-point stack underflow or overflow");
        m[EXCEPTION_FLT_UNDERFLOW] = MPUSTR("Access violation");
        m[EXCEPTION_ILLEGAL_INSTRUCTION] = MPUSTR("Illegal instruction");
        m[EXCEPTION_IN_PAGE_ERROR] = MPUSTR("Bad page access");
        m[EXCEPTION_INT_DIVIDE_BY_ZERO] = MPUSTR("Integer division by zero");
        m[EXCEPTION_INT_OVERFLOW] = MPUSTR("Integer overflow");
        m[EXCEPTION_INVALID_DISPOSITION] = MPUSTR("Buggy exception handler");
        m[EXCEPTION_NONCONTINUABLE_EXCEPTION] = MPUSTR("Non-continuable exception resume");
        m[EXCEPTION_PRIV_INSTRUCTION] = MPUSTR("Privileged instruction not allowed");
        m[EXCEPTION_STACK_OVERFLOW] = MPUSTR("Stack overflow");
        ender = m.end();
    }
    iter = m.find(code);
    if (iter != ender) msg.assign(iter->second);
    return msg;
}

static LONG WINAPI _mpuExceptionFilter(EXCEPTION_POINTERS* pEexceptionPointers)
{
    const EXCEPTION_RECORD* const pExceptionRecord =
        pEexceptionPointers->ExceptionRecord;
    const mpustring&
        msg(_mpuGetExceptionString(pExceptionRecord->ExceptionCode));
    if (!msg.empty()) {
        mpustringstream s;
        s << msg << MPUSTR(" at address ")
          << pExceptionRecord->ExceptionAddress;
        _mpuExceptionString.assign(s.str());
        mpu_jump(_mpuPosition, 1);
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

static void _mpuSetSignal(bool mine) {
    if (mine) {
        _mpuPreviousExceptionFilter =
            SetUnhandledExceptionFilter(_mpuExceptionFilter);
    }
    else SetUnhandledExceptionFilter(_mpuPreviousExceptionFilter);
}

#else

#include <csignal>

#define mpu_set_jump(env) sigsetjmp(env, 1)
#define mpu_jump(env, n)  siglongjmp((env), (n))

#define MPU_ARRAY_SIZE(array) (sizeof(array) / sizeof(*(array)))

static sigjmp_buf _mpuPosition;

static const int _mpuSignals[] = {
    SIGFPE,
    SIGILL,
    SIGSEGV,
#ifdef SIGBUS
    SIGBUS,
#endif
#ifdef SIGSYS
    SIGSYS,
#endif
};

static struct sigaction _mpuOldActions[MPU_ARRAY_SIZE(_mpuSignals)];

static void _mpuSignalHandler(int sig) {
    switch (sig) {
        case SIGFPE:
            _mpuExceptionString.assign(MPUSTR("Floating-point exception"));
            break;
        case SIGILL:
            _mpuExceptionString.assign(MPUSTR("Illegal instruction"));
            break;
        case SIGSEGV:
            _mpuExceptionString.assign(MPUSTR("Segmentation fault"));
            break;
#ifdef SIGBUS
        case SIGBUS:
            _mpuExceptionString.assign(MPUSTR("Bus error"));
            break;
#endif
#ifdef SIGSYS
        case SIGSYS:
            _mpuExceptionString.assign(MPUSTR("Bad system-call parameter"));
            break;
#endif
        default:
            _mpuExceptionString.assign(MPUSTR("Unknown system error"));
            break;
    }
    mpu_jump(_mpuPosition, 1);
}

static void _mpuSetSignal(bool mine) {
    struct sigaction newAction;
    newAction.sa_handler = _mpuSignalHandler;
    sigemptyset(&newAction.sa_mask);
    newAction.sa_flags = 0;
    for (size_t i = 0U, len = MPU_ARRAY_SIZE(_mpuOldActions); i < len; i++) {
      sigaction(_mpuSignals[i], mine ? &newAction         : &_mpuOldActions[i],
                                mine ? &_mpuOldActions[i] : NULL);
    }
}

#endif
#endif

#if ((defined(_WIN32)) || (defined(__CYGWIN__)))

static mpustring _mpuGetOsVersionString()
{
    CHAR buffer[64];
    HKEY handle;
    BOOL detected = false;
    DWORD sz = sizeof(buffer);
#ifndef LSTATUS
#define LSTATUS LONG
#endif
    LSTATUS status = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
      "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0U, KEY_QUERY_VALUE,
                                   &handle);
    if (status == ERROR_SUCCESS)  {
        status = RegQueryValueExA(handle, "ProductName", mpu_null, mpu_null,
                                  reinterpret_cast<LPBYTE>(&buffer[0]), &sz);
        if (status == ERROR_SUCCESS)  {
            detected = true;
        }
        RegCloseKey(handle);
    }
    if (!detected) {
        lstrcpyA(buffer, "Windows");
        sz = 8U;
    }
    lstrcatA(buffer, " 64-bit");
    if (GetProcAddress(GetModuleHandleA("kernel32"), "IsWow64Process")
        == mpu_null)
    {
        buffer[sz] = '3';
        buffer[sz + 1] = '2';
    }
    const std::string str(buffer);
#ifdef MPWUNICODE
    return mpustring(str.begin(), str.end());
#else
    return str;
#endif
}

static mpustring _mpuGetUserName()
{
    mpuchar buffer[32];
    DWORD sz = 32U;
    if (!GetUserName(buffer, &sz)) lstrcpy(buffer, MPUSTR("Unknown"));
    return mpustring(buffer);
}

#else

#include <cstdio>

static void _mpuGetCmdOutput(const char* cmd, char* buffer, unsigned int len)
{
    FILE* h = popen(cmd, "r");
    if (h != NULL) {
        char* const buf = fgets(buffer, static_cast<int>(len), h);
        if (buf != NULL) {
            const size_t sz = strlen(buf);
            if ((sz > 0U) && (buf[sz - 1U] == '\n')) {
                buf[sz - 1U] = '\0';
            }
        }
        else {
            *buffer = '\0';
        }
        pclose(h);
    }
}

static mpustring _mpuGetOsVersionString()
{
    mpustring osv;
    char buffer[64];
    _mpuGetCmdOutput("uname -sm", buffer,
                     static_cast<unsigned int>(sizeof(buffer)));
    if (*buffer == '\0') strcpy(buffer, "Unix");
    osv.assign(buffer);
    return osv;
}

static mpustring _mpuGetUserName()
{
    mpustring un;
    char buffer[64];
    _mpuGetCmdOutput("id -un", buffer,
                     static_cast<unsigned int>(sizeof(buffer)));
    if (*buffer == '\0') strcpy(buffer, "Unknown");
    un.assign(buffer);
    return un;
}

#endif

static mpustring _mpuGetTimestamp(const mpustring& fmt=mpustring())
{
    mpuchar buffer[64];
    std::tm* ptm;
    const std::time_t t(std::time(mpu_null));
    ptm = std::localtime(&t);
    mpustring actualFmt(MPUSTR("%a %d-%b-%Y - %H:%M:%S"));
    if (!fmt.empty()) actualFmt.assign(fmt);
#ifdef MPWUNICODE
    std::wcsftime(buffer, 64U, actualFmt.c_str(), ptm);
#else
    std::strftime(buffer, sizeof(buffer), actualFmt.c_str(), ptm);
#endif
    return mpustring(buffer);
}

#ifndef MPWUNICODE
#define VNUT_MPU_FILE baseName(__FILE__)
#else
#define VNUT_MPU_STRINGIFY(x) MPUSTR(x)
#define VNUT_MPU_FILE baseName(VNUT_MPU_STRINGIFY(__FILE__))
#endif

#define MPU_UNUSED(v) ((void)(v))

#define MPU_INFO(msg) _mpuInfo((msg), VNUT_MPU_FILE, __LINE__)
#define MPU_WARNING(cond) \
    assertBool(0, VNUT_MPU_FILE, __LINE__, (#cond), (cond))
#define MPU_ASSERT_TRUE(cond) \
    assertBool(1, VNUT_MPU_FILE, __LINE__, (#cond), (cond))
#define MPU_ASSERT_FALSE(cond) \
    assertBool(2, VNUT_MPU_FILE, __LINE__, (#cond), (cond))
#define MPU_ASSERT_ZERO(val) \
    assertInt(0, VNUT_MPU_FILE, __LINE__, (#val), (val))
#define MPU_ASSERT_ONE(val) \
    assertInt(1, VNUT_MPU_FILE, __LINE__, (#val), (val))
#define MPU_ASSERT_NOT_ZERO(val) \
    assertInt(2, VNUT_MPU_FILE, __LINE__, (#val), (val))
#define MPU_ASSERT_NULL(pntr) \
    assertNULL(true, VNUT_MPU_FILE, __LINE__, (#pntr), (pntr))
#define MPU_ASSERT_NOT_NULL(pntr) \
    assertNULL(false, VNUT_MPU_FILE, __LINE__, (#pntr), (pntr))
#define MPU_ASSERT_EQUAL(a, b) \
    assertEqual(true, VNUT_MPU_FILE, __LINE__, (a), (b))
#define MPU_ASSERT_NOT_EQUAL(a, b) \
    assertEqual(false, VNUT_MPU_FILE, __LINE__, (a), (b))
#define MPU_ASSERT_FLOAT_EQUAL(a, b, t) \
    compareFloats(true, VNUT_MPU_FILE, __LINE__, (a), (b), (t))
#define MPU_ASSERT_FLOAT_NOT_EQUAL(a, b, t) \
    compareFloats(false, VNUT_MPU_FILE, __LINE__, (a), (b), (t))
#define MPU_ASSERT_NOT_THROW(stmt) do { \
    bool vnut_mpu_fired = false; this->_mpuLvl = __LINE__; \
    _mpuStatus.assertions++; try { stmt; } \
    catch(...) { vnut_mpu_fired = true; } if (vnut_mpu_fired) \
    _mpuAssertions(0, MPUSTR("Passed statement threw an exception"), \
      VNUT_MPU_FILE, __LINE__, MPUSTR("MPU_ASSERT_NOT_THROW")); } while (0)
#define MPU_ASSERT_THROW(stmt) do { \
    bool vnut_mpu_fired = false; this->_mpuLvl = __LINE__; \
    _mpuStatus.assertions++; try { stmt; } \
    catch(...) { vnut_mpu_fired = true; } \
    if (!vnut_mpu_fired) _mpuAssertions(0, \
      MPUSTR("Passed statement didn't throw any exception"), \
      VNUT_MPU_FILE, __LINE__, MPUSTR("MPU_ASSERT_THROW")); } while (0)
#define MPU_ASSERT_NOT_THROW_EX(stmt, ex) do { \
    bool vnut_mpu_fired = false; this->_mpuLvl = __LINE__; \
    _mpuStatus.assertions++; try { stmt; } \
    catch(ex&) { vnut_mpu_fired = true; } catch(...) {}   \
    if (vnut_mpu_fired) _mpuAssertions(0, \
        MPUSTR("Passed statement threw unexpected exception"), \
        VNUT_MPU_FILE, __LINE__, MPUSTR("MPU_ASSERT_NOT_THROW_EX")); } while (0)
#define MPU_ASSERT_THROW_EX(stmt, ex) do {                           \
    bool vnut_mpu_fired = false; _mpuStatus.assertions++; \
    this->_mpuLvl = __LINE__; try { stmt; } \
    catch(ex&) { vnut_mpu_fired = true; } catch(...) {} \
    if (!vnut_mpu_fired) _mpuAssertions(0, \
        MPUSTR("Passed statement didn't throw expected exception"), \
        VNUT_MPU_FILE, __LINE__, MPUSTR("MPU_ASSERT_THROW_EX")); } while (0)

enum MpuOutputType {
    MPU_OUTPUT_STDOUT,
    MPU_OUTPUT_HTML,
    MPU_OUTPUT_JUNIT,
    MPU_OUTPUT_NULL
};

class MpuException : public std::exception {
public:
    MpuException() {}
    virtual ~MpuException() _mpu_noexcp _mpu_override {}
    MpuException(const MpuException&) {}
};

class MpuStatus {
public:
    MpuStatus() { Clear(); }

    size_t testsNum;
    size_t suitesNum;
    size_t failed;
    size_t skipped;
    size_t assertions;
    size_t warnings;
    size_t infos;
    mpustring abortMsg;

    void Clear() {
        testsNum = 0U;
        suitesNum = 0U;
        failed = 0U;
        skipped = 0U;
        assertions = 0U;
        warnings = 0U;
        infos = 0U;
    }

    void UpdateWith(const MpuStatus& other) {
        testsNum += other.testsNum;
        suitesNum += other.suitesNum;
        failed += other.failed;
        skipped += other.skipped;
        assertions += other.assertions;
        warnings += other.warnings;
        infos += other.infos;
        if (!other.abortMsg.empty()) abortMsg.assign(other.abortMsg);
    }
};

class MpuOutput
{
public:
    explicit MpuOutput(std::ostream* pStream) : o(pStream) {}
    virtual ~MpuOutput() _mpu_noexcp {}
    virtual void Banner(const mpustring&) {}
    virtual void Error(const mpustring&, const mpustring& file=mpustring(),
                       int line=-1)
    { MPU_UNUSED(file); MPU_UNUSED(line); }
    virtual void Warning(const mpustring&, const mpustring&, int) {} // text,f,l
    virtual void Info(const mpustring&, const mpustring&, int) {} // text,f,l
    virtual void SuiteBefore(const mpustring&, bool) {} // name, skip
    virtual void TestBefore(const mpustring&, bool skip=false,
                            const mpustring& sname=mpustring())
    { MPU_UNUSED(skip); MPU_UNUSED(sname); }
    virtual void TestAfter(const MpuStatus&) {}
    virtual void SuiteAfter(bool, const MpuStatus&) {}
    virtual void Summary(const MpuStatus&) {}

    static size_t GetTimeStamp() {
#if ((defined(_WIN32)) || (defined(__CYGWIN__)))
        const size_t milliseconds = GetTickCount();
#else
        struct timeval tv;
        gettimeofday(&tv, mpu_null);
        const size_t milliseconds = static_cast<size_t>(tv.tv_sec * 1000U
                                                        + tv.tv_usec / 1000U);
#endif
        return milliseconds;
    }

    static int GetPercentage(size_t x, size_t t) {
        int p = 0;
        if ((x > 0U) && (t > 0U)) {
            p = static_cast<int>(((x * 100U) + (t / 2U)) / t);
        }
        return p;
    }

    static mpustring GetElapsedTime(size_t start, size_t end)
    {
        mpustringstream s;
        if (end > start) {
            size_t total = end - start;
            size_t seconds, minutes, hours, days;
            timeUnits(s, total, seconds, minutes, hours, days);
            if ((total > 0U) && ((minutes + hours + days) == 0U)) {
                mpustringstream s2;
                if (seconds == 0U) s << MPUSTR('0');
                s << MPUSTR('.');
                s2 << total;
                mpustring str(s2.str());
                str.resize(3, MPUSTR('0'));
                s << str << MPUSTR(" seconds");
            }
        }
        else s << MPUSTR("0 secs");
        return s.str();
    }

    static mpustring sanitize(const mpustring& str) {
        const size_t len = str.size();
        mpustring sanitized;
        for (size_t i = 0U; i < len; i++) {
            const mpuchar c = str[i];
            if (mpu_isprint(c) != 0) {
                if (c == MPUSTR('&')) sanitized.append(MPUSTR("&amp;"));
                else if (c == MPUSTR('<')) sanitized.append(MPUSTR("&lt;"));
                else if (c == MPUSTR('>')) sanitized.append(MPUSTR("&gt;"));
                else sanitized.append(1U, c);
            }
        }

        return sanitized;
    }

    bool isStdout;
    mpustring assertionName;
#ifdef MPWUNICODE
    std::FILE* whandle;
#endif

protected:
    mpustringstream so;
    std::ostream* o;

    void dumpStream(mpustringstream& s) {
        const mpustring temp(s.str());
        if (!temp.empty()) {
            if (isStdout) mpucout << temp;
            else {
#ifdef MPWUNICODE
                static std::vector<char> converted;
                int status = WideCharToMultiByte(CP_UTF8, 0U, temp.c_str(), -1,
                                                 mpu_null, 0U,
                                                 mpu_null, mpu_null);
                if (status > 0) {
                    converted.reserve(static_cast<size_t>(status));
                    status = WideCharToMultiByte(CP_UTF8, 0U, temp.c_str(), -1,
                                                 &converted[0], status,
                                                 mpu_null, mpu_null);
                    if (status > 0) {
                        fwrite(&converted[0],
                               1U, static_cast<size_t>(status - 1),
                               whandle);
                    }
                }
                if (status == 0) fputs("???", whandle);
#else
                *o << temp;
#endif
            }
        }
        s.clear();
        s.str(mpustring());
    }

    mpustring gethostname() const {
        mpustring hn;
        mpuchar buffer[64];
#if ((defined(_WIN32)) || (defined(__CYGWIN__)))
        DWORD len = 64U;
        if (GetComputerName(buffer, &len)) hn.assign(buffer);
        else hn.assign(MPUSTR("Unknown"));
#else
        _mpuGetCmdOutput("hostname", buffer,
                         static_cast<unsigned int>(sizeof(buffer)));
        if (*buffer == '\0') strcpy(buffer, "Unknown");
        hn.assign(buffer);
#endif
        return hn;
    }

private:
        static void timeUnits(mpustringstream& s,
                              size_t& total,
                              size_t& seconds,
                              size_t& minutes,
                              size_t& hours,
                              size_t& days)
        {
            const mpustring spc(MPUSTR(", "));
            bool something = false;
            days = total / 86400000U;
            if (days > 0U) {
                s << days << MPUSTR(" days");
                total -= (days * 86400000U);
                something = true;
            }
            hours = total / 3600000U;
            if (hours > 0U) {
                if (something) s << spc;
                s << hours << MPUSTR(" hours");
                total -= (hours * 3600000U);
            }
            minutes = total / 60000U;
            if (minutes > 0U) {
                if (something) s << spc;
                s << minutes << MPUSTR(" minutes");
                total -= (minutes * 60000U);
                something = true;
            }
            seconds = total / 1000U;
            if (seconds > 0) {
                if (something) s << spc;
                s << seconds;
                total -= (seconds * 1000U);
                if (total == 0U) s << MPUSTR(" seconds");
            }
        }
};

class MpuOutputNull : public MpuOutput {
public:
    MpuOutputNull() : MpuOutput(mpu_null) {}
};

class MpuOutputHtml : public MpuOutput
{
private:
    class UserInfo {
    public:
        mpustring t;
        mpustring f;
        int l;
    };

    class HTest {
    public:
        HTest() {}
        mpustring name;
        mpustring suiteName;
        int status; // -1=failed, 0=skipped, 1=ok
        UserInfo error;
        std::vector<UserInfo> warnings;
        std::vector<UserInfo> informations;
        size_t timeStamp;
        size_t timeStamp2;
    };

    std::vector<HTest> tests;
    HTest* p;

    UserInfo HandleMsg(const mpustring& t, const mpustring& f, int l) const {
        UserInfo ui;
        ui.t = t;
        ui.f = f;
        ui.l = l;
        return ui;
    }

    void generateHeader(const mpustring& batName) {
        so << MPUSTR("<!DOCTYPE html><html lang=\"ru\">\n<head><meta charset=")
           << MPUSTR("\"utf-8\"><title>MPUnit battery report: ")
           << sanitize(batName)
           << MPUSTR("</title><style>.rt{ border: 0.1em solid black;}</style>")
           << MPUSTR("</head>\n<body style=\"background-color: #ebf5ff;\"><h1 ")
           << MPUSTR("style=\"text-align: center\">MPUnit test report</h1>\n")
           << MPUSTR("<h2 style=\"text-align: center; color:burlywood\">")
           << sanitize(batName)
           << MPUSTR("</h2>\n<table style=\"width: 80%; margin-left: ")
           << MPUSTR("auto; margin-right: auto\"><tr><td style=\"width: 35%\">")
           << MPUSTR("<table style=\"font-size: 1.2em; border: 0.3em double ")
           << MPUSTR("gray\"><tr><td style=\"font-weight: bold; padding-right:")
           << MPUSTR("1.5em\">Battery name:</td>\n<td>") << sanitize(batName)
           << MPUSTR("</td></tr>\n<tr><td style=\"font-weight: bold\">Creator:")
           << MPUSTR("</td><td>") << sanitize(_mpuGetUserName())
           << MPUSTR("</td></tr>")
           << MPUSTR("<tr><td style=\"font-weight: bold\">Host:")
           << MPUSTR("</td><td>") << sanitize(gethostname())
           << MPUSTR("</td></tr>\n")
           << MPUSTR("<tr><td style=\"font-weight: bold\">System:</td><td>")
           << sanitize(_mpuGetOsVersionString())
           << MPUSTR("</td></tr>\n<tr><td style=\"")
           << MPUSTR("font-weight: bold\">Timestamp:</td><td>")
           << sanitize(_mpuGetTimestamp())
           << MPUSTR("</td></tr><tr><td style=\"")
           << MPUSTR("font-weight: bold\">Duration:</td>\n");
    }

    void writeLeftHeader(const MpuStatus& status) {
        const int sp =
            GetPercentage(status.testsNum - status.skipped - status.failed,
                          status.testsNum);
        so << MPUSTR("<td>");
        if (status.testsNum == 0U) so << MPUSTR("0 secs");
        else so << GetElapsedTime(tests.front().timeStamp,
                                  tests.back().timeStamp2);
        so << MPUSTR("</td></tr></table></td>\n<td style=\"width: 30%\"><svg ")
           << MPUSTR("viewBox=\"0 0 42 42\" class=\"donut\">");
        if (sp > 0) {
            so << MPUSTR("<circle class=\"donut-hole\" cx=\"21\" cy=\"21\" ")
               << MPUSTR("r=\"15.91549430918954\" fill=\"black\"/>\n")
               << MPUSTR("<circle class=\"donut-segment\" cx=\"21\" cy=\"21\" ")
               << MPUSTR("r=\"15.91549430918954\" fill=\"transparent\" stroke=")
               << MPUSTR("\"green\" stroke-width=\"3\" stroke-dasharray=\"")
               << sp << MPUSTR(' ') << (100 - sp)
               << MPUSTR("\" stroke-dashoffset=\"25\"></circle>\n");
        }
        if (sp < 100) {
            int offset, ap = 0;
            if (status.skipped > 0U) {
                ap = GetPercentage(status.skipped, status.testsNum);
                so << MPUSTR("<circle class=\"donut-segment\" cx=\"21\" ")
                   << MPUSTR("cy=\"21\" r=\"15.91549430918954\" fill=\"")
                   << MPUSTR("transparent\" stroke=\"gray\" stroke-width=\"3\"")
                   << MPUSTR(" stroke-dasharray=\"");
                offset = (125 - sp) % 100;
                so << ap << MPUSTR(' ') << (100 - ap)
                   << MPUSTR("\" stroke-dashoffset=\"") << offset
                   << MPUSTR("\"></circle>\n");
            }
            if (status.failed > 0U) {
                offset = (125 - (sp + ap)) % 100;
                so << MPUSTR("<circle class=\"donut-segment\" cx=\"21\" ")
                   << MPUSTR("cy=\"21\" r=\"15.91549430918954\" fill=\"")
                   << MPUSTR("transparent\" stroke=\"red\" stroke-width=\"3\"")
                   << MPUSTR(" stroke-dasharray=\"")
                   << (100 - sp - ap) << MPUSTR(' ') << (sp + ap)
                   << MPUSTR("\" stroke-dashoffset=\"") << offset
                   << MPUSTR("\"></circle>\n");
            }
        }
        so << MPUSTR("<text style=\"fill: white; font-size: 0.5em\" x=\"28%\"")
           << MPUSTR(" y=\"52%\">");
        if (sp < 100) {
            so << MPUSTR("&nbsp;");
            if (sp < 10) {
                so << MPUSTR("&nbsp;");
            }
        }
        so << sp << MPUSTR("%</text>")
           << MPUSTR("<text style=\"fill: white; font-size: 0.25em\"")
           << MPUSTR(" x=\"35%\" y=\"62%\">Success</text></svg></td>\n")
           << MPUSTR("<td style=\"width: 10%\"><svg viewbox=\"0 0 32 14\">")
           << MPUSTR("<rect x=\"0\" y=\"0\" width=\"3\" height=\"3\" rx=\"1\" ")
           << MPUSTR("fill=\"green\"/><text x=\"4\" y=\"2.5\" fill=\"black\" ")
           << MPUSTR("style=\"font-size: 0.2em\">Passed</text>\n<rect x=\"0\" ")
           << MPUSTR("y=\"4.5\" width=\"3\" height=\"3\" rx=\"1\" fill=\"gray")
           << MPUSTR("\"/><text x=\"4\" y=\"7.25\" fill=\"gray\" style=\"font-")
           << MPUSTR("size: 0.2em\">Skipped</text>\n<rect x=\"0\" y=\"9\" ")
           << MPUSTR("width=\"3\" height=\"3\" rx=\"1\" fill=\"red\"/><text ")
           << MPUSTR("x=\"4\" y=\"11.7\" fill=\"red\" style=\"font-size: ")
           << MPUSTR("0.2em\">Failed</text></svg></td>\n");
   }

    void writeRightHeader(const MpuStatus& status) {
        so << MPUSTR("<td><table style=\"width: 60%; margin-left: auto; ")
           << MPUSTR("font-size: 1.3em; border: 0.4em double gray\">")
           << MPUSTR("<tr><td class=\"rt\" style=\"font-weight: bold; ")
           << MPUSTR("padding-right: 3em\">Total tests:</td>\n")
           << MPUSTR("<td class=\"rt\" style=\"text-align: center; ")
           << MPUSTR("padding: 0.4em 0.8em 0.4em 0.8em\">") << status.testsNum
           << MPUSTR("</td></tr>\n")
           << MPUSTR("<tr><td class=\"rt\" style=\"font-weight: bold\">Failed ")
           << MPUSTR("tests:</td><td class=\"rt\" style=\"text-align: center; ")
           << MPUSTR("color: ");
        if (status.failed > 0U) so << MPUSTR("red"); else so << MPUSTR("black");
        so << MPUSTR("; padding: 0.4em 0.8em 0.4em 0.8em\">") << status.failed
           << MPUSTR("</td></tr>\n")
           << MPUSTR("<tr><td class=\"rt\" style=\"font-weight: bold")
           << MPUSTR("\">Skipped tests:</td><td class=\"rt\" style=\"text-")
           << MPUSTR("align: center; padding: 0.4em 0.8em 0.4em 0.8em\">")
           << status.skipped << MPUSTR("</td></tr>")
           << MPUSTR("<tr><td class=\"rt\" style=\"font-weight: bold")
           << MPUSTR("\">Total suites:</td>\n<td class=\"rt\" style=\"text-")
           << MPUSTR("align: center; padding: 0.4em 0.8em 0.4em 0.8em\">")
           << status.suitesNum << MPUSTR("</td></tr>")
           << MPUSTR("<tr><td class=\"rt\" style=\"font-weight: bold")
           << MPUSTR("\">Assertions:</td>\n<td class=\"rt\" style=\"text-")
           << MPUSTR("align: center; padding: 0.4em 0.8em 0.4em 0.8em\">")
           << status.assertions << MPUSTR("</td></tr>\n")
           << MPUSTR("<tr><td class=\"rt\" style=\"font-weight: bold")
           << MPUSTR("\">Warnings:</td><td class=\"rt\" style=\"text-")
           << MPUSTR("align: center; padding: 0.4em 0.8em 0.4em 0.8em\">")
           << status.warnings << MPUSTR("</td></tr>\n")
           << MPUSTR("<tr><td class=\"rt\" style=\"font-weight: bold")
           << MPUSTR("\">Informations:</td><td class=\"rt\" style=\"text-")
           << MPUSTR("align: center; padding: 0.4em 0.8em 0.4em 0.8em\">")
           << status.infos << MPUSTR("</td></tr>\n")
           << MPUSTR("</table></td></tr></table>\n");
    }

    void prepareDetailedTable() {
        so << MPUSTR("<h2 style=\"text-align: center\">Detailed test ")
           << MPUSTR("report</h2><table style=\"border: 0.4em double gray; ")
           << MPUSTR("margin-left: auto; margin-right: auto; text-align:")
           << MPUSTR(" center\"><thead>\n<tr><th class=\"rt\" style=\"")
           << MPUSTR("padding: 1em; font-size: x-large\">Name</th><th ")
           << MPUSTR("class=\"rt\" style=\"font-size: x-large\">Duration")
           << MPUSTR("</th><th class=\"rt\" style=\"font-size: x-large\">")
           << MPUSTR("Warnings</th>\n<th class=\"rt\" style=\"font-size: ")
           << MPUSTR("x-large\">Informations</th></tr></thead>\n<tbody>");
    }

    void dumpWarningsInfo(const std::vector<UserInfo>& warnings,
                          const std::vector<UserInfo>& informations)
    {
        const size_t ws = warnings.size();
        const size_t is = informations.size();
        if (ws == 0U) so << MPUSTR("<td class=\"rt\">None</td>");
        else {
            so << MPUSTR("<td class=\"rt\" style=\"vertical-align: top")
               << MPUSTR("\"><table style=\"text-align: left\"><tr>")
               << MPUSTR("<td colspan=\"3\" style=\"text-align: center")
               << MPUSTR("; padding-bottom: 0.4em; padding-top: 0.8em;")
               << MPUSTR(" border: 0.2em ridge gray\">Total: ") << ws
               << MPUSTR("</td></tr>\n")
               << MPUSTR("<tr style=\"font-weight: bold; \"><td style=")
               << MPUSTR("\"border: 0.2em outset gray\">File</td><td ")
               << MPUSTR("style=\"border: 0.2em inset gray\">Line</td>")
               << MPUSTR("<td style=\"border: 0.2em outset gray\">")
               << MPUSTR("Message</td></tr>");
            for (size_t j = 0U; j < ws; j++) {
                const UserInfo& w(warnings[j]);
                so << MPUSTR("\n<tr><td>") << w.f << MPUSTR(":</td>")
                   << MPUSTR("<td>") << w.l << MPUSTR("</td>")
                   << MPUSTR("<td>") << w.t << MPUSTR("</td></tr>");
            }
            so << MPUSTR("</table></td>\n");
        }
        if (is == 0U) so << MPUSTR("<td class=\"rt\">None</td>");
        else {
            so << MPUSTR("<td class=\"rt\" style=\"vertical-align: top")
               << MPUSTR("\"><table style=\"text-align: left\"><tr>")
               << MPUSTR("<td colspan=\"3\" style=\"text-align: center")
               << MPUSTR("; padding-bottom: 0.4em; padding-top: 0.8em;")
               << MPUSTR(" border: 0.2em ridge gray\">Total: ") << is
               << MPUSTR("</td></tr>\n")
               << MPUSTR("<tr style=\"font-weight: bold; \"><td style=")
               << MPUSTR("\"border: 0.2em outset gray\">File</td><td ")
               << MPUSTR("style=\"border: 0.2em inset gray\">Line</td>")
               << MPUSTR("<td style=\"border: 0.2em outset gray\">")
               << MPUSTR("Message</td></tr>\n");
            for (size_t j = 0U; j < is; j++) {
                const UserInfo& info(informations[j]);
                so << MPUSTR("<tr><td>") << info.f << MPUSTR(":</td>")
                   << MPUSTR("<td>") << info.l << MPUSTR("</td>")
                   << MPUSTR("<td style=\"font-size: 0.8em\">")
                   << info.t << MPUSTR("</td></tr>\n");
            }
            so << MPUSTR("</table></td></tr>\n");
        }
    }

    void dumpTests() {
        for (size_t i = 0U, n = tests.size(); i < n; i++) {
            const HTest& test(tests[i]);
            so << MPUSTR("<tr style=\"font-size: larger\"><td class=\"rt\" ")
               << MPUSTR("style=\"padding: 0.8em\">");
            if (!test.suiteName.empty()) {
                so << MPUSTR("Suite: ") << test.suiteName <<MPUSTR("<br/>");
            }
            if (test.name.empty()) so << MPUSTR("&lt;&gt;");
            else so << test.name;
            so << MPUSTR("<br/><span style=\"color: ");
            if (test.status > 0) {
                so << MPUSTR("green\">Passed</span></td><td class=\"rt\">")
                   << GetElapsedTime(test.timeStamp, test.timeStamp2)
                   << MPUSTR("</td>");
            }
            else if (test.status == 0) {
                so << MPUSTR("gray\">Skipped</span></td><td class=\"rt\">")
                   <<MPUSTR("<span style=\"color: gray\">None</span></td>");
            }
            else {
                so << MPUSTR("red; font-size: 1.3em\">FAILED</span><br/>\n")
                   << MPUSTR("<br/>");
                if (test.error.l >= 0) {
                    so << test.error.f << MPUSTR(':') << test.error.l
                       << MPUSTR(": ") << test.error.t << MPUSTR("</td>");
                }
                else {
                    so << MPUSTR("<span style=\"color: brown\">")
                       << test.error.t << MPUSTR("</span></td>");
                }
                so <<MPUSTR("<td class=\"rt\"><span style=\"color: gray\"")
                   << MPUSTR(">None</span></td>");
            }
            so << MPUSTR("\n");
            dumpWarningsInfo(test.warnings, test.informations);
            dumpStream(so);
        }
    }

public:
    explicit MpuOutputHtml(std::ostream* pOutStream) : MpuOutput(pOutStream) {}
     ~MpuOutputHtml() _mpu_noexcp _mpu_override {}
    void Banner(const mpustring& batName) _mpu_override {
        generateHeader(batName);
    }
    void Error(const mpustring& t, const mpustring& f, int l) _mpu_override {
        p->status = -1;
        p->error = HandleMsg(sanitize(t), sanitize(f), l);
    }
    void Warning(const mpustring& t, const mpustring& f, int l) _mpu_override {
        p->warnings.push_back(HandleMsg(sanitize(t), sanitize(f), l));
    }
    void Info(const mpustring& t, const mpustring& f, int l) _mpu_override {
        p->informations.push_back(HandleMsg(sanitize(t), sanitize(f), l));
    }
    void TestBefore(const mpustring& name, bool skip, const mpustring& sname)
    _mpu_override
    {
        HTest test;
        test.status = skip ? 0 : 1;
        test.name.assign(sanitize(name));
        test.suiteName.assign(sanitize(sname));
        tests.push_back(test);
        p = &tests.back();
        p->timeStamp = MpuOutput::GetTimeStamp();
    }
    void TestAfter(const MpuStatus&) _mpu_override {
        p->timeStamp2 = MpuOutput::GetTimeStamp();
    }
    void Summary(const MpuStatus& status) _mpu_override {
        const int majVer = MPUNIT_MAJOR;
        const int minVer = MPUNIT_MINOR;

        writeLeftHeader(status);
        dumpStream(so);
        writeRightHeader(status);
        dumpStream(so);

        if (status.testsNum > 0U) {
            prepareDetailedTable();
            dumpStream(so);
            dumpTests();
            so << MPUSTR("</tbody></table>");
        }

        so << MPUSTR("<p style=\"font-size: smaller; color: ")
           << MPUSTR("cornflowerblue\">This report was generated by<a href=\"")
           << MPUSTR("https://github.com/mp81ss/mpunit\" target=\"_blank\">")
           << MPUSTR("MPUnit</a> v") << majVer << MPUSTR('.') << minVer
           << MPUSTR("</p></body></html>");

        dumpStream(so);
    }
};

class MpuOutputJunit : public MpuOutput
{
private:

    class JTest {
    public:
        int status; // -2 = error, -1 = failure, 0 = skipped, 1 = passed
        mpustring name;
        mpustring error;
        mpustring errorType;
        size_t ts;
        size_t ts2;
    };

    class JSuite { // Code-added hostname, duration added as property of sum
    public:
        bool skipped;
        mpustring name;
        mpustring timeStamp;
        std::vector<JTest> tests;
    };

    JSuite freeTests;
    std::vector<JSuite> suites;
    mpustring name;
    JTest* pTest;

public:

    explicit MpuOutputJunit(std::ostream* pOutStream) : MpuOutput(pOutStream) {}
    ~MpuOutputJunit() _mpu_noexcp _mpu_override {}
    void Banner(const mpustring& batName) _mpu_override {
        name.assign(batName);
        freeTests.name.assign(MPUSTR("Free tests"));
    }
    void Error(const mpustring& t, const mpustring& file, int line)
        _mpu_override
    {
        if (line < 0) {
            pTest->status = -2;
            pTest->error.assign(t);
        }
        else {
            mpustringstream s;
            s << file << MPUSTR(':') << line << MPUSTR(": ") << t;
            pTest->status = -1;
            pTest->error.assign(s.str());
        }
        pTest->errorType.assign(assertionName);
    }
    void SuiteBefore(const mpustring& sname, bool skipped)
        _mpu_override
    {
        JSuite suite;
        //suite.name.assign(sname.empty() ? MPUSTR("no name") : sname);
        if (sname.empty()) suite.name.assign(MPUSTR("no name"));
        else suite.name.assign(sname);
        suite.skipped = skipped;
        suites.push_back(suite);
    }
    void TestBefore(const mpustring& tname, bool skip, const mpustring& sname)
        _mpu_override
    {
        JSuite& suite(sname.empty() ? freeTests : suites.back());
        JTest test;
        test.status = skip ? 0 : 1;
        //test.name.assign(tname.empty() ? MPUSTR("no name") : tname);
        if (tname.empty()) test.name.assign(MPUSTR("no name"));
        else test.name.assign(tname);
        if (suite.tests.empty()) {
            suite.timeStamp.assign(_mpuGetTimestamp(
                                       MPUSTR("%Y-%m-%dT%H:%M:%S")));
        }
        suite.tests.push_back(test);
        pTest = &suite.tests.back();
        pTest->ts = GetTimeStamp();
    }
    void TestAfter(const MpuStatus&) _mpu_override {
        pTest->ts2 = GetTimeStamp();
    }
    void Summary(const MpuStatus& status) _mpu_override {
        so << MPUSTR("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<testsuites");
        if (!name.empty()) {
           so << MPUSTR(" name=\"") << sanitize(name) << MPUSTR("\"");
        }
        so << MPUSTR(" tests=\"") << status.testsNum << MPUSTR("\"");
        if ((status.testsNum > 0U) && (status.failed > 0U)) {
            so << MPUSTR(" failures=\"") << status.failed << MPUSTR("\"");
        }

        if (status.testsNum == 0U) {
            so << MPUSTR("/>\n");
        }
        else {
            const mpustring hostname(sanitize(gethostname()));
            so << MPUSTR(">\n");
            if (!freeTests.tests.empty()) {
                dumpTestSuite(freeTests, hostname);
            }
            dumpStream(so);
            for (size_t i = 0, n = suites.size(); i < n; i++) {
                dumpTestSuite(suites[i], hostname);
                dumpStream(so);
            }
            so << MPUSTR("</testsuites>\n");
        }
        dumpStream(so);
    }

private:

    void dumpTestSuite(const JSuite& suite, const mpustring& hostname) {
        const size_t tot = suite.tests.size();
        size_t skp = 0U;
        size_t fl = 0U;
        size_t err = 0U;

        if (suite.skipped) {
            skp = tot;
        }
        else {
            for (size_t i = 0U; i < tot; i++) {
                const int status = suite.tests[i].status;
                switch (status) {
                    case -2:
                        err++;
                        break;
                    case -1:
                        fl++;
                        break;
                    case 0:
                        skp++;
                        break;
                    default:
                        break;
                }
            }
        }

        so << MPUSTR("\t<testsuite name=\"") << sanitize(suite.name)
           << MPUSTR("\" tests=\"") << tot << MPUSTR("\"");
        if (skp > 0U) {
            so << MPUSTR(" skipped=\"") << skp << MPUSTR("\"");
        }
        so << MPUSTR(" failures=\"") << fl << MPUSTR("\"")
           << MPUSTR(" errors=\"") << err << MPUSTR("\"")
           << MPUSTR(" timestamp=\"") << suite.timeStamp << MPUSTR("\"");
        if (!hostname.empty()) {
            so << MPUSTR(" hostname=\"") << hostname << MPUSTR("\"");
        }

        if (tot > 0U) {
            const mpustring duration(getSuiteDuration(suite));
            if (!duration.empty()) {
                so << MPUSTR(">\n\t\t<properties><property name=\"duration\"")
                   << MPUSTR(" value=\"") << duration
                   << MPUSTR("\"/></properties");
            }
            so << MPUSTR(">\n");
            dumpSuiteTests(suite);
            so << MPUSTR("\t</testsuite>\n");
        }
        else {
            so << MPUSTR("/>\n");
        }
    }

    void dumpSuiteTests(const JSuite& suite) {
        for (size_t i = 0U, n = suite.tests.size(); i < n; i++) {
            const JTest& test(suite.tests[i]);
            const int status = (suite.skipped ? 0 : test.status);

            so << MPUSTR("\t\t<testcase name=\"") << sanitize(test.name)
               << MPUSTR("\"");
            if (status != 0) {
                so << MPUSTR(" time=\"") << MpuOutput::GetElapsedTime(test.ts,
                                                                      test.ts2)
                   << MPUSTR("\"");
            }
            switch (status) {
                case -2:
                    so << MPUSTR(">\n\t\t\t<error type=\"") << test.errorType
                       << MPUSTR("\" message=\"") << sanitize(test.error)
                       << MPUSTR("\"/>\n\t\t</testcase>\n");
                    break;
                case -1:
                    so << MPUSTR(">\n\t\t\t<failure type=\"") << test.errorType
                       << MPUSTR("\" message=\"") << sanitize(test.error)
                       << MPUSTR("\"/>\n\t\t</testcase>\n");
                    break;
                case 0:
                    so << MPUSTR(">\n\t\t\t<skipped/>\n\t\t</testcase>\n");
                    break;
                default:
                    so << MPUSTR("/>\n");
                    break;
            }
        }
    }

    mpustring getSuiteDuration(const JSuite& suite) {
        return MpuOutput::GetElapsedTime(suite.tests.front().ts,
                                         suite.tests.back().ts2);
    }
};

class MpuOutputStdout : public MpuOutput
{
public:
    explicit MpuOutputStdout(std::ostream* pOutStream)
        : MpuOutput(pOutStream) {}
    ~MpuOutputStdout() _mpu_noexcp _mpu_override {}
    void Banner(const mpustring& batteryName) _mpu_override {
        const int majVer = MPUNIT_MAJOR;
        const int minVer = MPUNIT_MINOR;

        so << MPUSTR("MPUNIT v") << majVer << MPUSTR('.')
           << minVer << std::endl << MPUSTR("Running test battery");

        if (batteryName.empty()) so << MPUSTR("...");
        else so << MPUSTR(": ") << batteryName << std::endl;
        so << std::endl << MPUSTR("User:   ") << _mpuGetUserName()
           << std::endl
           << MPUSTR("Host:   ") << gethostname() << std::endl
           << MPUSTR("System: ") << _mpuGetOsVersionString() << std::endl
           << MPUSTR("Date:   ") << _mpuGetTimestamp() << std::endl;
        ms = GetTimeStamp();
    }
    void Error(const mpustring& t, const mpustring& f, int l) _mpu_override {
        Dump(0, t, f, l);
    }
    void Warning(const mpustring& t, const mpustring& f, int l) _mpu_override {
        Dump(1, t, f, l);
    }
    void Info(const mpustring& t, const mpustring& f, int l) _mpu_override {
        Dump(2, t, f, l);
    }
    void SuiteBefore(const mpustring&, bool) _mpu_override {
        so << std::endl;
    }
    void TestBefore(const mpustring& name, bool skip, const mpustring& sname)
    _mpu_override
    {
        so << std::endl;
        if (!sname.empty()) so << sname << MPUSTR("::");
        if (name.empty()) so << MPUSTR("Anonymous"); else so << name;
        if (skip) so << MPUSTR(": Skipped");
        so << std::endl;
        dumpStream(so);
    }
    void Summary(const MpuStatus& status) _mpu_override {
        const size_t ms2 = GetTimeStamp();
        so << std::endl
           << std::endl << MPUSTR("Total tests:   ") << status.testsNum
           << std::endl << MPUSTR("Failed tests:  ") << status.failed
           << std::endl << MPUSTR("Skipped tests: ") << status.skipped
           << std::endl << MPUSTR("Total suites:  ") << status.suitesNum
           << std::endl << MPUSTR("Assertions:    ") << status.assertions
           << std::endl << MPUSTR("Warnings:      ") << status.warnings
           << std::endl << MPUSTR("Info:          ") << status.infos
           << std::endl << MPUSTR("\nSuccess: ")
           << GetPercentage(status.testsNum - status.skipped - status.failed,
                            status.testsNum)
           << MPUSTR('%') << MPUSTR(" in ") << GetElapsedTime(ms, ms2)
           << std::endl;
        dumpStream(so);
    }

private:
    void Dump(int level, const mpustring& t, const mpustring& f, int l) {
        const mpustring p[] = {
            MPUSTR("ERROR"), MPUSTR("WARNING"), MPUSTR("INFO")
        };
        const mpustring s[] = { MPUSTR("  "), MPUSTR(""), MPUSTR("   ") };
        so << MPUSTR("  ") << p[level] << MPUSTR(": ") << s[level];
        if (!f.empty()) so << f <<  MPUSTR(":") << l << MPUSTR(": ");
        so << t << std::endl;
        dumpStream(so);
    }

    size_t ms;
};

class MpuObject
{
public:
    virtual ~MpuObject() _mpu_noexcp {}
    mpustring name;
    void* argument;
    bool skip;
    const bool _mpuIs;
    MpuOutput* _mpuOut;
    MpuStatus _mpuStatus;

    template <typename T> T GetArgument() { return static_cast<T>(argument); }
    template <typename T> void SetArgument(T arg) {
        argument = const_cast<void*>(static_cast<const void*>(arg));
    }

    MpuObject(const MpuObject& o)
        : name(o.name), argument(o.argument), skip(o.skip), _mpuIs(o._mpuIs),
          _mpuOut(o._mpuOut), _mpuStatus(o._mpuStatus)
    {}

protected:
    MpuObject(const mpustring& objName, void* arg, bool isSuite)
      : name(objName), argument(arg), skip(false), _mpuIs(isSuite) {}
};

class MpuTest : public MpuObject
{
public:
    virtual ~MpuTest() _mpu_noexcp _mpu_override {}
    virtual void Run() = 0;

    mpustring _mpuSn;
    void* _mpuSa;
    int _mpuLvl;

    MpuTest(const MpuTest& o) : MpuObject(static_cast<MpuObject>(o)), 
        _mpuSn(o._mpuSn), _mpuSa(o._mpuSa), _mpuLvl(o._mpuLvl)
    {}

    template <typename T> T GetSuiteArgument() {
        return static_cast<T>(_mpuSa);
    }

protected:

    MpuTest(const mpustring& testName=mpustring(), void* arg=mpu_null)
        : MpuObject(testName, arg, false), _mpuLvl(-1) {}

    template <typename T> static bool isNull(const T& t) {
        static const void* const pNull = mpu_null;
        return memcmp(&t, &pNull, (std::min)(sizeof(T), sizeof(void*))) == 0;
    }

    template <typename T> static mpustring getParamValue(const T& t) {
        mpustringstream s;
        if (isNull<T>(t)) s << MPUSTR("0x0");
        else s << t;
        return s.str();
    }

    template <typename T> void _mpuInfo(const T& t, const mpustring& f, int l) {
        this->_mpuLvl = l;
        _mpuOut->Info(getParamValue<T>(t), f, l);
        _mpuStatus.infos++;
    }

    void _mpuAssertions(int el, const mpustring& t, const mpustring& f, int l,
                        const mpustring& asName=mpustring())
    {
        if (el == 0) {
            _mpuStatus.failed++;
            _mpuOut->assertionName.assign(asName);
            _mpuOut->Error(t, f, l);
            throw MpuException();
        }
        else if (el == 1) {
            _mpuOut->Warning(t, f, l);
            _mpuStatus.warnings++;
        }
        else {}
    }

    template <typename T>
    void assertBool(int code, const mpustring& f, int l,
                    const char* condStr, T cond)
    {
        mpustringstream s;
        mpustring macroName;

        this->_mpuLvl = l;

        switch (code)
        {
            case 0:
                if (!cond) {
                    s << condStr << MPUSTR(" was false");
                    macroName.assign(MPUSTR("MPU_WARNING"));
                }
                break;

            case 1:
                _mpuStatus.assertions++;
                if (!cond) {
                    s << condStr << MPUSTR(" was false");
                    macroName.assign(MPUSTR("MPU_ASSERT_TRUE"));
                }
                break;

            case 2:
                _mpuStatus.assertions++;
                if (cond) {
                    s << condStr << MPUSTR(" was true");
                    macroName.assign(MPUSTR("MPU_ASSERT_FALSE"));
                }
                break;

            default:
                break;
        }

        if (!macroName.empty()) {
            _mpuAssertions(code == 0 ? 1 : 0, s.str(), f, l, macroName);
        }
    }

    template <typename T>
    void assertInt(int code, const mpustring& f, int l,
                   const char* intStr, T val)
    {
        mpustringstream s;
        mpustring macroName;

        this->_mpuLvl = l;
        _mpuStatus.assertions++;

        switch (code)
        {
            case 0:
                if (val != 0) {
                    s << intStr << MPUSTR(" was NOT ZERO");
                    macroName.assign(MPUSTR("MPU_ASSERT_ZERO"));
                }
                break;

            case 1:
                if (val != 1) {
                    s << intStr << MPUSTR(" was NOT 1");
                    macroName.assign(MPUSTR("MPU_ASSERT_ONE"));
                }
                break;

            case 2:
                if (val == 0) {
                    s << intStr << MPUSTR(" was ZERO");
                    macroName.assign(MPUSTR("MPU_ASSERT_NOT_ZERO"));
                }
                break;

            default:
                break;
        }

        if (!macroName.empty()) {
            _mpuAssertions(0, s.str(), f, l, macroName);
        }
    }

    template <typename T>
    void assertNULL(bool condIsNull, const mpustring& f, int l,
                    const char* condStr, T cond)
    {
        mpustringstream s;
        mpustring macroName;

        this->_mpuLvl = l;
        _mpuStatus.assertions++;

        if (condIsNull) {
            if (!isNull<T>(cond)) {
                s << condStr << MPUSTR(" was NOT NULL");
                macroName.assign(MPUSTR("MPU_ASSERT_NULL"));
            }
        }
        else {
            if (isNull<T>(cond)) {
                s << condStr << MPUSTR(" was NULL");
                macroName.assign(MPUSTR("MPU_ASSERT_NOT_NULL"));
            }
        }
        if (!macroName.empty()) {
            _mpuAssertions(0, s.str(), f, l, macroName);
        }
    }

    template <typename T1, typename T2>
    void assertEqual(bool isEqual, const mpustring& f, int l,
                     const T1& a, const T2& b)
    {
        mpustring s;
        mpustring macroName;

        this->_mpuLvl = l;
        _mpuStatus.assertions++;

        if (isEqual) {
            if ((a == b) == false) {
                s = MPUSTR("Expected ") + getParamValue<T1>(a)
                    + MPUSTR(", found ") + getParamValue<T2>(b);
                macroName.assign(MPUSTR("MPU_ASSERT_EQUAL"));
            }
        }
        else {
            if (a == b) {
                s = MPUSTR("Both values were ") + getParamValue<T1>(a);
                macroName.assign(MPUSTR("MPU_ASSERT_NOT_EQUAL"));
            }
        }

        if (!macroName.empty()) {
            _mpuAssertions(0, s, f, l, macroName);
        }
    }

    template <typename T> void compareFloats(bool isEqual, const mpustring& f,
                                             int l, T a, T b, T t)
    {
        T diff = a - b;
        mpustring macroName;
        mpustringstream s;

        this->_mpuLvl = l;
        _mpuStatus.assertions++;

        if (diff < static_cast<T>(0.0)) diff = -diff;

        if (isEqual) {
            if (diff > t) {
                s << MPUSTR("Expected ") << a << MPUSTR(", found ") << b
                << MPUSTR("> with tolerance ") << t;
                macroName.assign(MPUSTR("MPU_ASSERT_FLOAT_EQUAL"));
            }
        }
        else {
            if ((diff > t) == false) {
                s << a << MPUSTR(" was equal to ") << b
                  << MPUSTR(" with tolerance ") << t;
                macroName.assign(MPUSTR("MPU_ASSERT_FLOAT_NOT_EQUAL"));
            }
        }

        if (!macroName.empty()) {
            _mpuAssertions(0, s.str(), f, l, macroName);
        }
    }

    void MpuAbort(const mpustring& abortMsg) {
        mpustring s(MPUSTR("Aborted"));
        if (!abortMsg.empty()) s.append(MPUSTR(": ")).append(abortMsg);
        _mpuStatus.abortMsg.assign(s);
        _mpuStatus.failed++;
        _mpuOut->Error(s, mpustring(), 0);
        throw MpuException();
    }

    mpustring baseName(const mpustring& str) {
#ifdef _WIN32
        static const mpuchar sep = MPUSTR('\\');
#else
        static const mpuchar sep = MPUSTR('/');
#endif
        const size_t pos = str.find_last_of(sep);
        return (pos != mpustring::npos) ? str.substr(pos + 1U) : str;
    }

    const mpustring GetSuiteName() const { return _mpuSn; }
};

class MpuSuite : public MpuObject
{
public:
    virtual ~MpuSuite() _mpu_noexcp _mpu_override {}
    bool RemoveTest(const mpustring& rname) { return _mpuRm(rname, tests); }
    void Shuffle() { _mpuMkrd(tests); }
    void* _mpuGt() { return &tests; }
    template <typename C> static bool _mpuRm(const mpustring& str, C& array) {
        typename C::iterator iter(array.begin());
        const typename C::const_iterator& ender(array.end());
        bool erased = false;
        for (; (iter != ender) && !erased; ++iter) {
            if ((*iter)->name == str) {
                array.erase(iter);
                erased = true;
            }
        }
        return erased;
    }
    template <typename T> void AddTest(const T& test) {
        T* const t = new T(test);
        tests.push_back(t);
    }
    template <typename T> static void _mpuMkrd(T& array) {
#if __cplusplus < 201103L
        std::random_shuffle(array.begin(), array.end());
#else
        std::random_device rd;
        std::ranlux48 r(rd());
        std::shuffle(array.begin(), array.end(), r);
#endif
    }
    virtual void SuiteSetup() {}
    virtual void SuiteTeardown() {}
    virtual void Setup() {}
    virtual void Teardown() {}

    MpuSuite(const MpuSuite& o)
        : MpuObject(static_cast<MpuObject>(o)), tests(o.tests) 
    {}

protected:

    MpuSuite(const mpustring& suiteName=mpustring(), void* arg=mpu_null)
      : MpuObject(suiteName, arg, true) {}

private:

    std::vector<MpuTest*> tests;
};

class MpUnit
{
public:
    explicit MpUnit(const mpustring& batName=mpustring()) : name(batName) {
         Reset();
    } 
    ~MpUnit() _mpu_noexcp { clearObjects(); }

    MpuOutputType outputFormat;
    mpustring outputFile;
    bool stopOnError;
    bool enableSystemExceptions;

    static int GetMajor() { return MPUNIT_MAJOR; }
    static int GetMinor() { return MPUNIT_MINOR; }
    static mpustring GetVersion() {
        mpustringstream s;
        s << GetMajor() << MPUSTR('.') << GetMinor();
        return s.str();
    }

    template <typename T> void AddTest(const T& test) {
        T* const t = new T(test);
        objects.push_back(t);
    }

    bool RemoveTest(const mpustring& rname) {
        return MpuSuite::_mpuRm(rname, objects);
    }

    template <typename S> void AddSuite(const S& suite) {
        S* const s = new S(suite);
        objects.push_back(s);
    }

    bool RemoveSuite(const mpustring& rname) {
        return MpuSuite::_mpuRm(rname, objects);
    }

    void Shuffle() {
        for (size_t i = 0U, n = objects.size(); i < n; i++) {
            if (objects[i]->_mpuIs) {
                MpuSuite* const pSuite(static_cast<MpuSuite*>(objects[i]));
                pSuite->Shuffle();
            }
        }
        MpuSuite::_mpuMkrd(objects);
    }

    void mainLoop() {
        std::vector<MpuObject*>::iterator iter(objects.begin());
        const std::vector<MpuObject*>::const_iterator ender(objects.end());
        status.Clear();
        for (; (iter != ender) && status.abortMsg.empty()
                    && (!stopOnErr || (status.failed == 0U));
                ++iter)
        {
            MpuObject* const pObj(*iter);
            if (!pObj->_mpuIs) {
                launchTest(static_cast<MpuTest*>(pObj));
            }
            else {
                launchSuite(static_cast<MpuSuite*>(pObj));
            }
            status.UpdateWith(pObj->_mpuStatus);
        }
    }

    void GetStatus(MpuStatus& s) {
        s.Clear();
        s.UpdateWith(status);
    }

    void Reset() {
        clearObjects();
        outputFile.clear();
        outputFormat = MPU_OUTPUT_STDOUT;
        enableSystemExceptions = true;
        stopOnError = false;
    }

    void Run(int argc=0, mpuchar** argv=mpu_null) {
        mpustring parserMsg(parser(argc, argv));
        if (parserMsg.empty()) {
            out = mpu_null;
            parserMsg.assign(prepareRun());
            if (parserMsg.empty()) {
                sysEx = enableSystemExceptions && !IsDebuggerPresent();
                stopOnErr = stopOnError;

                out->Banner(name);
                mainLoop();
                out->Summary(status);

#ifdef MPWUNICODE
                if (!outputFile.empty()) fclose(out->whandle);
#endif
                delete out;
            }
        }
        if (!parserMsg.empty()) {
            mpucout << MPUSTR("MPUnit v") << GetVersion() << std::endl
                    << std::endl << MPUSTR("Run: $test_runner")
                    << MPUSTR(" [option[s]] [Suite/Test[s] to run]")
                    << std::endl << std::endl
                    << MPUSTR("-h Show this help screen") << std::endl
                    << MPUSTR("-f <format> <stdout|html|junit|null>")
                    << MPUSTR(" (Default is stdout)") << std::endl
                    << MPUSTR("-o <output_file> Writes output to output_file")
                    << MPUSTR(" (Default is stdout)") << std::endl
                    << MPUSTR("-x Disable system exception handling")
                    << std::endl
                    << MPUSTR("-e Stop all next tests if one fails")
                    << std::endl
                    << std::endl << MPUSTR("You can append tests and suites ")
                    << MPUSTR("to run. Prefix with 'suitename::' if any")
                    << std::endl;
            if (parserMsg.size() > 1U) {
                mpucout << std::endl << MPUSTR("ERROR: ") << parserMsg
                        << std::endl;
            }
        }
    }

private:

    void clearObjects() {
        std::vector<MpuObject*>::iterator iter(objects.begin());
        const std::vector<MpuObject*>::const_iterator ender(objects.end());
        for (; iter != ender; ++iter) {
            MpuObject* const pObj(*iter);
            if (!pObj->_mpuIs) {
                delete pObj;
            }
            else {
                MpuSuite* const pSuite(static_cast<MpuSuite*>(pObj));
                std::vector<MpuObject*>* pTests(
                    static_cast<std::vector<MpuObject*>*>(pSuite->_mpuGt()));
                for (size_t i = 0U, n = pTests->size(); i < n; i++) {
                    delete pTests->at(i);
                }
                delete pSuite;
            }
        }
        objects.clear();
    }

    mpustring setOutputFormat(int argc, const mpuchar** argv, int i) {
        mpustring msg;
        if (i < (argc - 1)) {
            const mpustring fmt(argv[i + 1]);
            if (fmt == MPUSTR("stdout")) {
                outputFormat = MPU_OUTPUT_STDOUT;
            }
            else if (fmt == MPUSTR("html")) {
                outputFormat = MPU_OUTPUT_HTML;
            }
            else if (fmt == MPUSTR("junit")) {
                outputFormat = MPU_OUTPUT_JUNIT;
            }
            else if (fmt == MPUSTR("null")) {
                outputFormat = MPU_OUTPUT_NULL;
            }
            else msg.assign(MPUSTR("Invalid output format"));
        }
        else msg.assign(MPUSTR("Missing output format"));
        return msg;
    }

    mpustring handleSwitch(int argc, mpuchar** argv, int* pi) {
        mpustring msg;
        int i = *pi;
        const mpustring arg(argv[i]);
        if (arg == MPUSTR("-f")) {
            msg.assign(setOutputFormat(argc,
                                       const_cast<const mpuchar**>(argv),
                                       i++));
        }
        else if (arg == MPUSTR("-o")) {
            if ((++i) < argc) {
                outputFile.assign(argv[i]);
            }
            else msg.assign(MPUSTR("Missing output file"));
        }
        else {
            if (!arg.empty() && (arg != MPUSTR("::"))) {
                selected.insert(arg);
            }
        }
        *pi = i;
        return msg;
    }

    mpustring parser(int argc, mpuchar** argv) {
        mpustring msg;
        for (int i = 1; (i < argc) && msg.empty(); i++) {
            const mpustring arg(argv[i]);
            if (arg == MPUSTR("-h")) {
                msg.append(1U, MPUSTR('H'));
            }
            else if (arg == MPUSTR("-x")) {
                enableSystemExceptions = false;
            }
            else if (arg == MPUSTR("-e")) {
                stopOnError = true;
            }
            else msg.assign(handleSwitch(argc, argv, &i));
        }
        return msg;
    }

    bool setFormat() {
        bool ok = true;
        switch (outputFormat) {
            case MPU_OUTPUT_STDOUT:
                out = new MpuOutputStdout(&ofStream);
                break;
            case MPU_OUTPUT_HTML:
                out = new MpuOutputHtml(&ofStream);
                break;
            case MPU_OUTPUT_JUNIT:
                out = new MpuOutputJunit(&ofStream);
                break;
            case MPU_OUTPUT_NULL:
                out = new MpuOutputNull;
                break;
            default:
                ok = false;
                break;
        }
        if (ok) out->isStdout = outputFile.empty();
        return ok;
    }

    mpustring prepareRun() {
        mpustring msg;

        if (!setFormat()) msg.assign(MPUSTR("Invalid output format"));
        else {
            if (!outputFile.empty()) {

#ifdef MPWUNICODE

                out->whandle = _wfopen(outputFile.c_str(), MPUSTR("w"));
                if (out->whandle == mpu_null) {
                    msg.assign(MPUSTR("Cannot open output file"));
                }
#else
                ofStream.open(outputFile.c_str());
                if (!ofStream.is_open()) {
                    msg.assign(MPUSTR("Cannot open output file"));
                }
#endif
            }

#if ((defined(MPWUNICODE)) && (!defined(__BORLANDC__)))
            else {

#ifndef _O_U16TEXT
#define _O_U16TEXT 0x20000
#endif
                _setmode(_fileno(stdout), _O_U16TEXT);
            }
#endif
        }

        return msg;
    }

    void runTest(MpuTest* pTest) {
        mpustringstream s;
        pTest->_mpuOut = out;
        try { pTest->Run(); }
        catch(const MpuException&) {}
        catch(const std::exception& e) {
            s << e.what();
            out->assertionName.assign(MPUSTR("Unexpected exception"));
            out->Error(s.str());
            pTest->_mpuStatus.failed++;
        }
        catch(int n) {
            s << MPUSTR("User threw number ") << n;
            out->assertionName.assign(MPUSTR("Unexpected integer exception"));
            out->Error(s.str());
            pTest->_mpuStatus.failed++;
        }
        catch(const mpuchar* pMsg) {
            mpustring msg;
            if (pMsg != mpu_null) msg.assign(mpustring(pMsg));
            out->assertionName.assign(MPUSTR("Unexpected tchar* exception"));
            out->Error(msg);
            pTest->_mpuStatus.failed++;
        }
        catch(const mpustring& msg) {
            out->assertionName.assign(MPUSTR("Unexpected string exception"));
            out->Error(msg);
            pTest->_mpuStatus.failed++;
        }
        catch(...) {
            out->assertionName.assign(MPUSTR("Unexpected error"));
            out->Error(MPUSTR("Unexpected error"));
            pTest->_mpuStatus.failed++;
        }
    }

    bool mustSkipTest(const mpustring& tname, const MpuSuite* pSuite) {
        bool mustSkip = false;
        mpustring sname;
        if ((pSuite != mpu_null) && !pSuite->name.empty()) {
            sname.append(pSuite->name).append(MPUSTR("::"));
        }
        if (!selected.empty()
            && (selected.find(sname + tname) == selected.end())
            && (selected.find(sname) == selected.end()))
        {
            mustSkip = true;
        }
        return mustSkip;
    }

    void launchTest(MpuTest* pTest, MpuSuite* pSuite=mpu_null) {
        MpuStatus* pStatus(&status);
        mpustring sname;

        if (pSuite == mpu_null) pTest->_mpuSa = mpu_null;
        else {
            if (pSuite->name.empty()) sname.assign(MPUSTR("Anonymous"));
            else sname.assign(pSuite->name);
            pTest->_mpuSa = pSuite->argument;
            pStatus = &pSuite->_mpuStatus;
        }

        pStatus->testsNum++;

        pTest->_mpuStatus.Clear();

        if (mustSkipTest(pTest->name, pSuite)) pTest->skip = true;
        out->TestBefore(pTest->name, pTest->skip, sname);

        if (!pTest->skip) {

#ifndef MPU_NO_SEH
            if (sysEx) {
                if (mpu_set_jump(_mpuPosition) == 0) {
                    _mpuSetSignal(true);
                    runTest(pTest);
                }
                else {
                    mpustringstream s;
                    s << _mpuExceptionString;
                    if (pTest->_mpuLvl > 0) {
                        s << MPUSTR(", line ") << pTest->_mpuLvl
                          << MPUSTR(" or subsequent");
                    }
                    pStatus->failed++;
                    out->assertionName.assign(MPUSTR("Critical error"));
                    out->Error(s.str());
                }
                _mpuSetSignal(false);
            }
            else
#endif
            runTest(pTest);
        }
        else {
            pStatus->skipped++;
        }

        out->TestAfter(pTest->_mpuStatus);
    }

    void launchSuite(MpuSuite* pSuite) {
        status.suitesNum++;
        if (!selected.empty() && (selected.find(pSuite->name + MPUSTR("::"))
                                  == selected.end()))
        {
            pSuite->skip = true;
        }
        const bool setupRun = !pSuite->skip;
        if (setupRun) pSuite->SuiteSetup();
        out->SuiteBefore(pSuite->name, pSuite->skip);
        const int done = pSuite->skip ? 0 : 1;
        runSuite(pSuite);
        if (setupRun) pSuite->SuiteTeardown();
        out->SuiteAfter(done, pSuite->_mpuStatus);
    }

    void runSuite(MpuSuite* pSuite) {
        std::vector<MpuTest*>* const pTests(
            static_cast<std::vector<MpuTest*>*>(pSuite->_mpuGt()));
        std::vector<MpuTest*>::iterator iter(pTests->begin());
        const std::vector<MpuTest*>::const_iterator ender(pTests->end());

        // const mpustring sname(pSuite->name.empty() ? MPUSTR("Anonymous")
        //                                            : pSuite->name);
        mpustring sname;
        if (pSuite->name.empty()) sname.assign(MPUSTR("Anonymous"));
        else sname.assign(pSuite->name);

        pSuite->_mpuStatus.Clear();
        for (; (iter != ender) && status.abortMsg.empty()
               && (!stopOnErr || (status.failed == 0U));
             ++iter)
        {
            MpuTest* const pTest(*iter);
            if (pSuite->skip) {
                out->TestBefore(pTest->name, true, sname);
                pTest->_mpuStatus.Clear();
                pTest->_mpuStatus.testsNum++;
                pTest->_mpuStatus.skipped++;
                out->TestAfter(pTest->_mpuStatus);
            }
            else {
                bool setupRun = !pTest->skip;
                if (setupRun) pSuite->Setup();
                launchTest(pTest, pSuite);
                if (setupRun) pSuite->Teardown();
            }
            pSuite->_mpuStatus.UpdateWith(pTest->_mpuStatus);
        }
    }

    mpustring name;
    std::vector<MpuObject*> objects;
    std::set<mpustring> selected;
    MpuOutput* out;
    bool stopOnErr;
    bool sysEx;
    MpuStatus status;

    std::ofstream ofStream;
#ifdef MPWUNICODE
    std::FILE* whandle;
#endif

    MpUnit(const MpUnit&);
    MpUnit& operator=(const MpUnit&);
};

#endif // MPUNIT_HPP_
