// Enable assert() even on release build
#undef NDEBUG

#include <algorithm>
#include <cassert>
#include <queue>

#include "test/Test.h"
#include "test/TestUtil.h"
#include "constraints/GraphDivision.h"

using namespace Glucose;

namespace {

int EnumerateGraphDivisionBySAT(int n, const std::vector<std::pair<int, int>>& graph, const std::vector<std::vector<int>>& size_cands = {}) {
    Solver S;

    std::vector<OptionalOrderEncoding> vertices(n);
    if (!size_cands.empty()) {
        for (int i = 0; i < n; ++i) {
            if (size_cands[i].empty()) continue;

            std::vector<Lit> lits;
            for (int j = 1; j < size_cands[i].size(); ++j) {
                assert(size_cands[i][j - 1] < size_cands[i][j]);
                lits.push_back(mkLit(S.newVar()));
            }
            vertices[i].lits = lits;
            vertices[i].values = size_cands[i];
        }
    }

    std::vector<Var> vars;
    for (int i = 0; i < graph.size(); ++i) {
        vars.push_back(S.newVar());
    }

    std::vector<Lit> lits;
    for (int i = 0; i < graph.size(); ++i) {
        lits.push_back(mkLit(vars[i]));
    }
    S.addConstraint(std::make_unique<GraphDivision>(vertices, graph, lits));
    return CountNumAssignment(S, vars);
}

int EnumerateGraphDivisionNaive(int n, const std::vector<std::pair<int, int>>& graph, const std::vector<std::vector<int>>& size_cands) {
    std::vector<std::vector<std::pair<int, int>>> adj(n);
    for (int i = 0; i < graph.size(); ++i) {
        auto [u, v] = graph[i];
        adj[u].push_back({v, i});
        adj[v].push_back({u, i});
    }
    int ret = 0;

    for (int bits = 0; bits < (1 << graph.size()); ++bits) {
        std::vector<int> region_id(n, -1);

        for (int i = 0; i < n; ++i) {
            if (region_id[i] >= 0) continue;

            std::vector<int> region;
            std::queue<int> qu;
            qu.push(i);
            region.push_back(i);
            region_id[i] = i;

            while (!qu.empty()) {
                int p = qu.front(); qu.pop();

                for (auto [q, e] : adj[p]) {
                    if ((bits >> e) & 1) continue;
                    if (region_id[q] >= 0) continue;

                    qu.push(q);
                    region.push_back(q);
                    region_id[q] = i;
                }
            }

            for (int p : region) {
                for (auto [q, e] : adj[p]) {
                    if (((bits >> e) & 1) && region_id[q] == i) {
                        goto fail;
                    }
                }
            }
            if (!size_cands.empty()) {
                int size = region.size();
                for (int p : region) {
                    if (size_cands[p].empty()) continue;
                    if (std::find(size_cands[p].begin(), size_cands[p].end(), size) == size_cands[p].end()) {
                        goto fail;
                    }
                }
            }
        }
        ret += 1;

    fail:
        continue;
    }

    return ret;
}

void GraphDivisionTestPath(int n) {
    std::vector<std::pair<int, int>> graph;
    for (int i = 0; i < n - 1; ++i) {
        graph.push_back({i, i + 1});
    }
    int expected = 1 << (n - 1);

    assert(EnumerateGraphDivisionBySAT(n, graph) == expected);
}

void GraphDivisionTestCycle(int n) {
    std::vector<std::pair<int, int>> graph;
    for (int i = 0; i < n - 1; ++i) {
        graph.push_back({i, i + 1});
    }
    graph.push_back({0, n - 1});
    int expected = (1 << n) - n;

    assert(EnumerateGraphDivisionBySAT(n, graph) == expected);
}

void GraphDivisionTestComplex(int n, const std::vector<std::pair<int, int>>& graph, const std::vector<std::vector<int>>& size_cands = {}) {
    int expected = EnumerateGraphDivisionNaive(n, graph, size_cands);
    int actual = EnumerateGraphDivisionBySAT(n, graph, size_cands);

    assert(expected == actual);
}

}

DEFINE_TEST(graph_division_path) {
    GraphDivisionTestPath(1);
    GraphDivisionTestPath(2);
    GraphDivisionTestPath(5);
    GraphDivisionTestPath(10);
}

DEFINE_TEST(graph_division_cycle) {
    GraphDivisionTestCycle(3);
    GraphDivisionTestCycle(5);
    GraphDivisionTestCycle(10);
}

DEFINE_TEST(graph_division_complex) {
    GraphDivisionTestComplex(9, {
        {0, 1},
        {1, 2},
        {3, 4},
        {4, 5},
        {6, 7},
        {7, 8},
        {0, 3},
        {1, 4},
        {2, 5},
        {3, 6},
        {4, 7},
        {5, 8},
    });

    GraphDivisionTestComplex(9, {
        {0, 1},
        {1, 2},
        {3, 4},
        {4, 5},
        {6, 7},
        {7, 8},
        {0, 3},
        {1, 4},
        {2, 5},
        {3, 6},
        {4, 7},
        {5, 8},
    }, {
        {2, 3},
        {},
        {},
        {},
        {1, 3},
        {},
        {},
        {},
        {4, 5},
    });

    GraphDivisionTestComplex(12, {
        {0, 1},
        {1, 2},
        {2, 3},
        {4, 5},
        {5, 6},
        {6, 7},
        {8, 9},
        {9, 10},
        {10, 11},
        {0, 4},
        {1, 5},
        {2, 6},
        {3, 7},
        {4, 8},
        {5, 9},
        {6, 10},
        {7, 11},
    }, {
        {4},
        {5},
        {3},
        {},
        {},
        {},
        {},
        {},
        {},
        {},
        {},
        {},
    });
}
