#include "core/Constraint.h"
#include "core/Solver.h"

#include <vector>

namespace Glucose {

class DirectEncodingExtensionSupports : public Constraint {
public:
    // sum(terms) + constant >= 0
    DirectEncodingExtensionSupports(std::vector<std::vector<Lit>>&& vars, std::vector<std::vector<int>>&& supports);
    virtual ~DirectEncodingExtensionSupports() = default;

    bool initialize(Solver& solver) override;
    bool propagate(Solver& solver, Lit p) override;
    void calcReason(Solver& solver, Lit p, Lit extra, vec<Lit>& out_reason) override;
    void undo(Solver& solver, Lit p) override;

private:
    std::vector<std::vector<Lit>> vars_;
    std::vector<std::vector<int>> supports_;

    std::vector<std::tuple<Lit, int, int>> lits_;
    std::vector<int> known_values_;
    std::vector<int> undo_list_;
    std::vector<Lit> active_lits_;
};

}
