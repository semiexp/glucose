#include <algorithm>
#include <string>
#include <vector>

class TestRunner {
public:
    TestRunner(const std::string& name);

    virtual void Run() = 0;

    static void RunAllTests();

private:
    static std::vector<std::pair<std::string, TestRunner*>>& GetRegisteredTests();
};

#define CONCAT_IMPL(a,b) a##b

#define DEFINE_TEST_WITH_CLASSNAME(name,classname) class classname : TestRunner {\
public:\
    classname () : TestRunner(#name) {} \
    void Run() override;\
};\
classname CONCAT_IMPL(unique_instance_, classname);\
void classname::Run()

#define DEFINE_TEST(name) DEFINE_TEST_WITH_CLASSNAME(name, CONCAT_IMPL(TestRunner, name))
