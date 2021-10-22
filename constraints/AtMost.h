#include "core/Constraint.h"
#include "core/Solver.h"

#include <vector>

namespace Glucose {

class AtMost : public Constraint {
public:
    AtMost(std::vector<Lit>&& lits, int threshold);
    virtual ~AtMost() = default;

    bool initialize(Solver& solver, vec<Lit>& out_watchers) override;
    bool propagate(Solver& solver, Lit p) override;
    void calcReason(Solver& solver, Lit p, Lit extra, vec<Lit>& out_reason) override;
    void undo(Solver& solver, Lit p) override;

private:
    std::vector<Lit> lits_;
    int threshold_, n_true_;
};

}
