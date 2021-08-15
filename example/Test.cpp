// Enable assert() even on release build
#undef NDEBUG

#include <cassert>
#include <vector>

#include "example/Graph.h"
#include "core/Solver.h"

using namespace Glucose;


int EnumerateConnectedSubgraphBySAT(int n, const std::vector<std::pair<int, int>>& graph) {
    Solver S;

    std::vector<Var> vars;
    for (int i = 0; i < n; ++i) {
        vars.push_back(S.newVar());
    }

    std::vector<Lit> lits;
    for (int i = 0; i < n; ++i) {
        lits.push_back(mkLit(vars[i]));
    }
    S.addConstraint(std::make_unique<ActiveVerticesConnected>(lits, graph));
    int n_ans = 0;
    for (;;) {
        bool has_ans = S.solve();
        if (!has_ans) break;

        ++n_ans;
        vec<Lit> deny_clause;
        for (int i = 0; i < n; ++i) {
            deny_clause.push(mkLit(vars[i], S.modelValue(vars[i]) == l_True));
        }
        S.addClause(deny_clause);
    }

    return n_ans;
}

void ConnectedSubgraphTestPath(int n) {
    std::vector<std::pair<int, int>> graph;
    for (int i = 0; i < n - 1; ++i) {
        graph.push_back({i, i + 1});
    }
    int expected = n * (n + 1) / 2 + 1;

    assert(EnumerateConnectedSubgraphBySAT(n, graph) == expected);
}

void ConnectedSubgraphTestCycle(int n) {
    std::vector<std::pair<int, int>> graph;
    for (int i = 0; i < n; ++i) {
        graph.push_back({i, (i + 1) % n});
    }
    int expected = n * (n - 1) + 2;

    assert(EnumerateConnectedSubgraphBySAT(n, graph) == expected);
}

void PropagationOnInit() {
    Solver S;

    std::vector<Var> vars;
    for (int i = 0; i < 5; ++i) {
        vars.push_back(S.newVar());
    }

    std::vector<std::pair<int, int>> graph;
    for (int i = 0; i < 4; ++i) {
        graph.push_back({i, i + 1});
    }

    std::vector<Lit> lits;
    for (int i = 0; i < 5; ++i) {
        lits.push_back(mkLit(vars[i]));
    }
    
    S.addClause(mkLit(vars[1]));
    S.addClause(mkLit(vars[2], true));
    S.addConstraint(std::make_unique<ActiveVerticesConnected>(lits, graph));
    assert(!S.addClause(mkLit(vars[4])));
}

int main() {
    ConnectedSubgraphTestPath(1);
    ConnectedSubgraphTestPath(2);
    ConnectedSubgraphTestPath(5);
    ConnectedSubgraphTestPath(50);
    ConnectedSubgraphTestCycle(1);
    ConnectedSubgraphTestCycle(2);
    ConnectedSubgraphTestCycle(5);
    ConnectedSubgraphTestCycle(50);

    PropagationOnInit();

    return 0;
}
