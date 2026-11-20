// EH3D-NODE-PNP-HEAD - tests/check.hpp
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
// The smallest test harness that does the job: TEST(name) { CHECK(...); } registers a test, run_all() runs them and says how many passed. No library is needed.
#pragma once

#include <cstdio>
#include <string>
#include <utility>
#include <vector>

namespace check {

inline std::vector<std::pair<const char*, void (*)()>>& registry() {
    static std::vector<std::pair<const char*, void (*)()>> r;
    return r;
}
inline int& failures() { static int n = 0; return n; }
inline int& checks() { static int n = 0; return n; }

struct Registrar {
    Registrar(const char* name, void (*fn)()) { registry().push_back({name, fn}); }
};

inline int run_all(const char* suite) {
    int bad_tests = 0;
    for (auto& t : registry()) {
        int before = failures();
        t.second();
        if (failures() != before) { ++bad_tests; std::printf("  FAILED  %s\n", t.first); }
    }
    std::printf("%s: %d tests, %d checks, %d tests failed\n", suite, static_cast<int>(registry().size()), checks(), bad_tests);
    return bad_tests == 0 ? 0 : 1;
}

}  // namespace check

#define TEST(name)                                          \
    static void name();                                     \
    static check::Registrar registrar_##name(#name, name);  \
    static void name()

#define CHECK(cond)                                                                \
    do {                                                                           \
        ++check::checks();                                                         \
        if (!(cond)) {                                                             \
            ++check::failures();                                                   \
            std::printf("    check failed at %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        }                                                                          \
    } while (0)
