#include "core/Constraint.h"
#include "core/Solver.h"

#include <vector>

namespace Glucose {

// x is one of {domain[0], ..., domain[n - 1]}
// lits[i] <=> (x >= domain[i + 1])
struct LinearTerm {
    // Make `coef` 1 by modifying `lits` and `domain`.
    void normalize();

    std::vector<Lit> lits;
    std::vector<int> domain;
    int coef;
};

class OrderEncodingLinear : public Constraint {
public:
    // sum(terms) + constant >= 0
    OrderEncodingLinear(std::vector<LinearTerm>&& terms, int constant);
    virtual ~OrderEncodingLinear() = default;

    bool initialize(Solver& solver) override;
    bool propagate(Solver& solver, Lit p) override;
    void calcReason(Solver& solver, Lit p, Lit extra, vec<Lit>& out_reason) override;
    void undo(Solver& solver, Lit p) override;

private:
    std::vector<LinearTerm> terms_;
    std::vector<std::tuple<Lit, int, int>> lits_;
    std::vector<int> ub_index_;
    std::vector<std::pair<int, int>> undo_list_;
    std::vector<Lit> active_lits_;
    int constant_, total_ub_;
};

}
