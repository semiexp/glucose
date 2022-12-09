// Enable assert() even on release build
#undef NDEBUG

#include <cassert>

#include "test/Test.h"
#include "test/TestUtil.h"
#include "constraints/DirectEncodingExtension.h"

using namespace Glucose;

namespace {

std::vector<Lit> MakeDirectEncodingVars(Solver& solver, int dom_size, std::vector<Var>& all_vars) {
    std::vector<Lit> ret;
    for (int i = 0; i < dom_size; ++i) {
        Var v = solver.newVar();
        all_vars.push_back(v);
        ret.push_back(mkLit(v));
    }

    vec<Lit> lits;
    for (int i = 0; i < ret.size(); ++i) {
        lits.push(ret[i]);
    }
    solver.addClause(lits);

    for (int i = 0; i < ret.size(); ++i) {
        for (int j = i + 1; j < ret.size(); ++j) {
            solver.addClause(~ret[i], ~ret[j]);
        }
    }

    return ret;
}

int CountNumValidAssignmentsSub(const std::vector<int>& dom_sizes, const std::vector<std::pair<std::vector<int>, std::vector<std::vector<int>>>>& supports_descs, std::vector<int>& vals) {
    if (vals.size() == dom_sizes.size()) {
        // check validity
        for (auto& [var_ids, supports] : supports_descs) {
            bool isok = false;
            for (auto& support : supports) {
                bool match = true;
                for (int i = 0; i < var_ids.size(); ++i) {
                    int x = support[i];
                    int y = vals[var_ids[i]];
                    if (x != y && x != -1 && y != -1) {
                        match = false;
                        break;
                    }
                }
                if (match) {
                    isok = true;
                    break;
                }
            }
            if (!isok) {
                return 0;
            }
        }
        return 1;
    }
    int ret = 0;
    int sz = dom_sizes[vals.size()];
    for (int i = 0; i < sz; ++i) {
        vals.push_back(i);
        ret += CountNumValidAssignmentsSub(dom_sizes, supports_descs, vals);
        vals.pop_back();
    }
    return ret;
}

int CountNumValidAssignments(const std::vector<int>& dom_sizes, const std::vector<std::pair<std::vector<int>, std::vector<std::vector<int>>>>& supports_descs) {
    std::vector<int> vals;
    return CountNumValidAssignmentsSub(dom_sizes, supports_descs, vals);
}

void DirectEncodingExtensionSupportsTestCount(const std::vector<int>& dom_sizes, const std::vector<std::pair<std::vector<int>, std::vector<std::vector<int>>>>& supports_descs) {
    Solver solver;

    int n_int_vars = dom_sizes.size();
    std::vector<Var> all_vars;
    std::vector<std::vector<Lit>> var_descs;
    for (int i = 0; i < n_int_vars; ++i) {
        var_descs.push_back(MakeDirectEncodingVars(solver, dom_sizes[i], all_vars));
    }

    for (int i = 0; i < supports_descs.size(); ++i) {
        auto& [var_ids, supports] = supports_descs[i];
        std::vector<std::vector<Lit>> lits;
        for (int j = 0; j < var_ids.size(); ++j) {
            assert(0 <= var_ids[j] && var_ids[j] < n_int_vars);
            lits.push_back(var_descs[var_ids[j]]);
        }
        for (int j = 0; j < supports.size(); ++j) {
            assert(supports[j].size() == var_ids.size());
            for (int k = 0; k < var_ids.size(); ++k) {
                assert(-1 <= supports[j][k] && supports[j][k] < dom_sizes[var_ids[k]]);
            }
        }
        auto s = supports;
        solver.addConstraint(std::make_unique<DirectEncodingExtensionSupports>(std::move(lits), std::move(s)));
    }

    int n_assignment = CountNumAssignment(solver, all_vars);
    int n_assignment_naive = CountNumValidAssignments(dom_sizes, supports_descs);
    assert(n_assignment == n_assignment_naive);
}

}

DEFINE_TEST(direct_encoding_extension_supports_count) {
    DirectEncodingExtensionSupportsTestCount({3, 4, 5}, {
        {
            {0, 1, 2},
            {
                {0, 0, 0},
                {0, 0, 2},
                {0, 0, 3},
                {0, 1, 4},
                {1, 2, 1},
                {1, 3, 3},
                {2, 0, 1},
                {2, 0, 2},
                {2, 1, 3},
                {2, 2, 0},
            }
        }
    });

    DirectEncodingExtensionSupportsTestCount({3, 4, 5}, {
        {
            {0, 1, 2},
            {
                {0, -1, 0},
                {0, 0, 2},
                {0, 0, 3},
                {0, 1, 4},
                {1, 2, -1},
                {1, 3, 3},
                {2, 0, 1},
                {-1, -1, 3},
            }
        }
    });

    DirectEncodingExtensionSupportsTestCount({4, 4, 5, 5}, {
        {
            {0, 1, 2},
            {
                {0, -1, 0},
                {0, 0, 2},
                {0, 0, 3},
                {0, 1, 4},
                {1, 2, -1},
                {1, 3, 3},
                {2, 0, 1},
                {-1, -1, 3},
            }
        },
        {
            {1, 2, 3},
            {
                {0, -1, 0},
                {0, 0, 2},
                {0, 0, 3},
                {0, 1, 4},
                {1, 2, -1},
                {1, 3, 3},
                {2, 0, 1},
                {-1, -1, 3},
            }
        },
    });

    DirectEncodingExtensionSupportsTestCount({3, 3}, {
        {
            {0, 1, 0},
            {
                {0, 0, 0},
                {0, 0, 1},
                {-1, 1, 2},
                {2, -1, -1},
            }
        }
    });

    DirectEncodingExtensionSupportsTestCount({4, 4, 4, 4, 4}, {
        {
            {0, 1, 2},
            {
                {0, -1, -1},
                {-1, 0, -1},
                {-1, -1, 0},
            }
        },
        {
            {1, 2, 4},
            {
                {1, -1, -1},
                {-1, 2, -1},
                {-1, -1, 3},
            }
        },
        {
            {0, 2, 3},
            {
                {2, -1, -1},
                {-1, 2, -1},
                {-1, -1, 3},
            }
        },
    });
}
