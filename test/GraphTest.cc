// Enable assert() even on release build
#undef NDEBUG

#include <cassert>

#include "test/Test.h"
#include "test/TestUtil.h"
#include "constraints/Graph.h"

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
    return CountNumAssignment(S, vars);
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

DEFINE_TEST(graph_path) {
    ConnectedSubgraphTestPath(1);
    ConnectedSubgraphTestPath(2);
    ConnectedSubgraphTestPath(5);
    ConnectedSubgraphTestPath(50);
}

DEFINE_TEST(graph_cycle) {
    ConnectedSubgraphTestCycle(1);
    ConnectedSubgraphTestCycle(2);
    ConnectedSubgraphTestCycle(5);
    ConnectedSubgraphTestCycle(50);
}

DEFINE_TEST(graph_propagation_on_init) {
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
