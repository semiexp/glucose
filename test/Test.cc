#include "test/Test.h"

#include <iostream>

TestRunner::TestRunner(const std::string& name) {
    GetRegisteredTests().push_back({name, this});
}

void TestRunner::RunAllTests() {
    auto& registered_tests = GetRegisteredTests();
    for (size_t i = 0; i < registered_tests.size(); ++i) {
        std::cerr << "[" << (i + 1) << "/" << registered_tests.size() << "] " << registered_tests[i].first << std::endl;
        registered_tests[i].second->Run();
    }
}

std::vector<std::pair<std::string, TestRunner*>>& TestRunner::GetRegisteredTests() {
    static std::vector<std::pair<std::string, TestRunner*>> data;
    return data;
}

int main() {
    TestRunner::RunAllTests();
    return 0;
}
