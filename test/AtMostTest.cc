// Enable assert() even on release build
#undef NDEBUG

#include <cassert>

#include "test/Test.h"
#include "test/TestUtil.h"
#include "constraints/AtMost.h"

using namespace Glucose;

int AtMostTestCount(int n, int k) {
    Solver S;

    std::vector<Var> vars;
    for (int i = 0; i < n; ++i) {
        vars.push_back(S.newVar());
    }

    std::vector<Lit> lits;
    for (int i = 0; i < n; ++i) lits.push_back(mkLit(vars[i]));
    S.addConstraint(std::make_unique<AtMost>(std::move(lits), k));

    int res = CountNumAssignment(S, vars);
    return res;
}

int CountNumAtMostPatterns(int n, const std::vector<std::vector<int>>& values, const std::vector<int>& thresholds) {
    int ret = 0;
    for (int bits = 0; bits < (1 << n); ++bits) {
        bool satisfied = true;
        for (int i = 0; i < values.size(); ++i) {
            int count = 0;
            for (int j = 0; j < values[i].size(); ++j) {
                int v = values[i][j];
                if (v >= 0) count += (bits >> v) & 1;
                else count += 1 - ((bits >> ~v) & 1);
            }

            if (count > thresholds[i]) satisfied = false;
        }

        if (satisfied) ++ret;
    }

    return ret;
}

void AtMostTestPattern(int n, const std::vector<std::vector<int>>& values, const std::vector<int>& thresholds) {
    Solver solver;

    std::vector<Var> vars;
    for (int i = 0; i < n; ++i) {
        vars.push_back(solver.newVar());
    }

    for (int i = 0; i < values.size(); ++i) {
        std::vector<Lit> lits;
        for (int j = 0; j < values[i].size(); ++j) {
            int v = values[i][j];
            if (v >= 0) lits.push_back(mkLit(vars[v]));
            else lits.push_back(mkLit(vars[~v], true));
        }

        solver.addConstraint(std::make_unique<AtMost>(std::move(lits), thresholds[i]));
    }

    int n_assignment = CountNumAssignment(solver, vars);
    int n_assignment_naive = CountNumAtMostPatterns(n, values, thresholds);
    assert(n_assignment == n_assignment_naive);
}

DEFINE_TEST(atmost_count) {
    const int MAXN = 10;
    int binomial[MAXN + 1][MAXN + 1];

    for (int i = 1; i <= MAXN; ++i) binomial[0][i] = 0;
    binomial[0][0] = 1;

    for (int i = 1; i <= MAXN; ++i) {
        binomial[i][0] = 1;
        for (int j = 1; j <= MAXN; ++j) {
            binomial[i][j] = binomial[i - 1][j - 1] + binomial[i - 1][j];
        }
    }

    for (int n = 1; n <= MAXN; ++n) {
        int binomial_sum = 0;
        for (int k = 0; k <= n; ++k) {
            binomial_sum += binomial[n][k];
            assert(AtMostTestCount(n, k) == binomial_sum);
        }
    }
}

DEFINE_TEST(atmost_propagation_on_init) {
    Solver S;

    for (int k = 0; k <= 1; ++k) {
        std::vector<Var> vars;
        for (int i = 0; i < 10; ++i) {
            vars.push_back(S.newVar());
        }

        std::vector<Lit> lits;
        for (int i = 0; i < vars.size(); ++i) {
            lits.push_back(mkLit(vars[i]));
        }

        if (k == 1) {
            S.addClause(mkLit(vars[1]));
        }
        S.addConstraint(std::make_unique<AtMost>(std::move(lits), k));
        assert(S.value(vars[0]) == l_False);
        assert(S.value(vars[9]) == l_False);
    }
}

DEFINE_TEST(atmost_complex) {
    AtMostTestPattern(10, {
        {0, 1, 2, 3, 4},
        {2, ~3, 4, 5, 6, 7},
        {~2, 7, 8, 9},
    }, {3, 3, 2});
}
