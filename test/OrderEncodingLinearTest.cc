// Enable assert() even on release build
#undef NDEBUG

#include <cassert>

#include "test/Test.h"
#include "test/TestUtil.h"
#include "constraints/OrderEncodingLinear.h"

using namespace Glucose;

std::vector<Lit> MakeOrderEncodingVars(Solver& solver, const std::vector<int>& domain, std::vector<Var>& all_vars) {
    std::vector<Lit> ret;
    for (int i = 0; i < domain.size() - 1; ++i) {
        Var v = solver.newVar();
        all_vars.push_back(v);
        ret.push_back(mkLit(v));
    }
    for (int i = 1; i < ret.size(); ++i) {
        solver.addClause(ret[i - 1], ~ret[i]);
    }
    return ret;
}

int CountNumIPAssignmentsSub(const std::vector<std::vector<int>>& domains, const std::vector<std::vector<int>>& coefs, std::vector<int>& vals) {
    if (vals.size() == domains.size()) {
        for (int i = 0; i < coefs.size(); ++i) {
            int v = coefs[i].back();
            for (int j = 0; j < domains.size(); ++j) {
                v += coefs[i][j] * vals[j];
            }
            if (v < 0) return 0;
        }
        return 1;
    }
    int ret = 0;
    for (int x : domains[vals.size()]) {
        vals.push_back(x);
        ret += CountNumIPAssignmentsSub(domains, coefs, vals);
        vals.pop_back();
    }
    return ret;
}

int CountNumIPAssignments(const std::vector<std::vector<int>>& domains, const std::vector<std::vector<int>>& coefs) {
    std::vector<int> vals;
    return CountNumIPAssignmentsSub(domains, coefs, vals);
}

void OrderEncodingLinearTestIP(const std::vector<std::vector<int>>& domains, const std::vector<std::vector<int>>& coefs) {
    Solver solver;

    std::vector<Var> all_vars;
    std::vector<std::vector<Lit>> lits;
    for (int i = 0; i < domains.size(); ++i) {
        lits.push_back(MakeOrderEncodingVars(solver, domains[i], all_vars));
    }

    for (int i = 0; i < coefs.size(); ++i) {
        assert(coefs[i].size() == domains.size() + 1);

        std::vector<LinearTerm> terms;
        for (int j = 0; j < domains.size(); ++j) {
            if (coefs[i][j] == 0) continue;
            terms.push_back({ lits[j], domains[j], coefs[i][j] });
        }
        solver.addConstraint(std::make_unique<OrderEncodingLinear>(std::move(terms), coefs[i][domains.size()]));
    }

    int n_assignment = CountNumAssignment(solver, all_vars);
    int n_assignment_naive = CountNumIPAssignments(domains, coefs);
    assert(n_assignment == n_assignment_naive);
}

void OrderEncodingLinearTestInconsistent(const std::vector<std::vector<int>>& domains, const std::vector<int>& coefs) {
    Solver solver;

    std::vector<Var> all_vars;
    std::vector<std::vector<Lit>> lits;
    for (int i = 0; i < domains.size(); ++i) {
        lits.push_back(MakeOrderEncodingVars(solver, domains[i], all_vars));
    }

    assert(coefs.size() == domains.size() + 1);

    std::vector<LinearTerm> terms;
    for (int j = 0; j < domains.size(); ++j) {
        if (coefs[j] == 0) continue;
        terms.push_back({ lits[j], domains[j], coefs[j] });
    }
    assert(!solver.addConstraint(std::make_unique<OrderEncodingLinear>(std::move(terms), coefs[domains.size()])));
}

DEFINE_TEST(order_encoding_linear_count) {
    OrderEncodingLinearTestIP({
        {1, 2, 3},
        {1, 2, 3},
    }, {
        {1, 2, -6},
    });

    OrderEncodingLinearTestIP({
        {1, 2, 3},
        {1, 2, 3},
    }, {
        {1, 2, -6},
        {1, -1, 0},
    });

    OrderEncodingLinearTestIP({
        {1, 2, 3},
        {1, 2, 3},
        {2, 3, 4, 5},
        {3, 4, 5, 6},
    }, {
        {1, 2, 0, 0, -6},
        {1, 1, 1, 1, -10},
        {2, 1, -3, 1, 2},
    });

    OrderEncodingLinearTestIP({
        {1, 2, 3, 5, 6, 8},
        {1, 2, 4, 8, 15},
        {2, 3, 7, 9, 11},
        {3, 4, 5, 6, 9},
    }, {
        {1, 2, 0, 0, -8},
        {1, 1, 1, 1, -15},
        {1, 2, -3, 1, -10},
    });

    OrderEncodingLinearTestIP({
        {0},
        {0, 1},
        {0, 1},
        {0, 1},
        {0, 1},
        {0, 1},
        {0, 1},
        {0, 1},
        {0, 1},
        {0, 1},
        {0, 1},
    }, {
        {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, -5},
    });
}

DEFINE_TEST(order_encoding_linear_unsatisfiable) {
    OrderEncodingLinearTestInconsistent({
        {1, 2, 3},
        {1, 2, 3},
    }, {
        {1, 2, -10},
    });

    OrderEncodingLinearTestInconsistent({
        {1},
        {1},
    }, {
        {1, 2, -4},
    });
}

DEFINE_TEST(order_encoding_linear_propagate_on_creation) {
    const std::vector<std::vector<int>> domains = {
        {1, 2, 3, 4},
        {2, 4, 6, 8},
    };
    const std::vector<std::vector<int>> coefs = {
        {1, 1, -7},
    };

    Solver solver;

    std::vector<Var> all_vars;
    std::vector<std::vector<Lit>> lits;
    for (int i = 0; i < domains.size(); ++i) {
        lits.push_back(MakeOrderEncodingVars(solver, domains[i], all_vars));
    }

    for (int i = 0; i < coefs.size(); ++i) {
        assert(coefs[i].size() == domains.size() + 1);

        std::vector<LinearTerm> terms;
        for (int j = 0; j < domains.size(); ++j) {
            if (coefs[i][j] == 0) continue;
            terms.push_back({ lits[j], domains[j], coefs[i][j] });
        }
        solver.addConstraint(std::make_unique<OrderEncodingLinear>(std::move(terms), coefs[i][domains.size()]));
    }
    assert(!solver.addClause(~lits[1][0]));
}
