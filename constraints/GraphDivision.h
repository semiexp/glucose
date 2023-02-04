#pragma once

#include "core/Constraint.h"
#include "core/Solver.h"

#include <vector>
#include <algorithm>

namespace Glucose {

// A struct for representing integer variables in order encoding.
// If the value is present,
// - lits.size() == values.size() + 1,
// - The domain of the variable is `values`, and
// - lits[i] iff (the value) >= values[i+1].
// If the value is absent, values.size() == 0. This does not mean that the value has
// the empty domain (in this case, the CSP is just unsatisfiable);
// instead, this means that there is no constraint on this value.
struct OptionalOrderEncoding {
    std::vector<Lit> lits;
    std::vector<int> values;

    bool is_absent() const { return values.size() == 0; }
};

class GraphDivision : public Constraint {
public:
    GraphDivision(const std::vector<OptionalOrderEncoding>& vertices, const std::vector<std::pair<int, int>>& edges, const std::vector<Lit>& edge_lits);
    virtual ~GraphDivision() = default;

    bool initialize(Solver& solver) override;
    bool propagate(Solver& solver, Lit p) override;
    void calcReason(Solver& solver, Lit p, Lit extra, vec<Lit>& out_reason) override;
    void undo(Solver& solver, Lit p) override;

private:
    enum EdgeState {
        kUndecided, kConnected, kDisconnected
    };
    const int kUnvisited = -1;

    std::optional<std::vector<Lit>> run_check(Solver& solver);
    int compute_regions(std::vector<std::vector<int>>& regions, std::vector<int>& region_id, bool is_potential);
    void compute_regions_dfs(int p, int id, std::vector<int>& region, std::vector<int>& region_id, bool is_potential);

    // Compute the reason why `src` and `dest` are in the same decided region
    std::vector<Lit> reason_decided_connecting_path(int src, int dest);

    // Compute the reason why all vertices in the decided region `id` are all connected
    std::vector<Lit> reason_decided_region(int id);

    // Compute the reason why the potential region `id` cannot be further extended
    std::vector<Lit> reason_potential_region(int id);

    std::vector<OptionalOrderEncoding> vertices_;
    std::vector<std::vector<std::pair<int, int>>> adj_;  // (dest, edge id)
    std::vector<Lit> edge_lits_;

    std::vector<EdgeState> edge_state_;
    std::vector<int> size_lb_, size_ub_;

    std::vector<std::vector<int>> decided_regions_;
    std::vector<int> decided_region_id_;
    std::vector<std::vector<int>> potential_regions_;
    std::vector<int> potential_region_id_;

    std::vector<std::vector<Lit>> reasons_;
};

}
