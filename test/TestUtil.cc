#include "TestUtil.h"

int CountNumAssignment(Glucose::Solver& solver, const std::vector<Glucose::Var>& vars) {
    int n_ans = 0;
    for (;;) {
        bool has_ans = solver.solve();
        if (!has_ans) break;

        ++n_ans;
        Glucose::vec<Glucose::Lit> deny_clause;
        for (int i = 0; i < vars.size(); ++i) {
            deny_clause.push(Glucose::mkLit(vars[i], solver.modelValue(vars[i]) == l_True));
        }
        solver.addClause(deny_clause);
    }
    return n_ans;
}
