// Enable assert() even on release build
#undef NDEBUG

#include <cassert>

#include "test/Test.h"
#include "test/TestUtil.h"
#include "constraints/Xor.h"

using namespace Glucose;

int XorTestDimension(const std::vector<std::vector<int>>& constraints, const std::vector<int>& parities, int n) {
    Solver S;

    std::vector<Var> vars;
    for (int i = 0; i < n; ++i) {
        vars.push_back(S.newVar());
    }

    for (int i = 0; i < constraints.size(); ++i) {
        const std::vector<int>& con = constraints[i];
        std::vector<Lit> lits;
        for (int j = 0; j < con.size(); ++j) {
            lits.push_back(mkLit(vars[con[j]]));
        }
        const int parity = 0;
        S.addConstraint(std::make_unique<Xor>(lits, parities[i]));
    }

    return CountNumAssignment(S, vars);
}

DEFINE_TEST(xor_dimension) {
    assert(XorTestDimension({{0, 1}}, {0}, 3) == 4);
    assert(XorTestDimension({{0, 1}, {1, 2}}, {0, 1}, 3) == 2);
    assert(XorTestDimension({{0, 1, 3, 5}, {1, 3, 5}}, {1, 1}, 6) == 16);
    assert(XorTestDimension({{0, 1, 3, 5, 8}, {2, 3}, {0, 1, 2, 5, 8}}, {1, 0, 1}, 10) == 256);
    assert(XorTestDimension({{0, 1, 3, 5, 8}, {2, 3}, {0, 1, 2, 5, 8}}, {0, 0, 1}, 10) == 0);
}
