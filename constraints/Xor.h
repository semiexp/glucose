#include "core/Constraint.h"
#include "core/Solver.h"

#include <vector>

namespace Glucose {

class Xor : public Constraint {
public:
    Xor(const std::vector<Lit>& lits, int parity);
    virtual ~Xor() = default;

    bool initialize(Solver& solver, vec<Lit>& out_watchers) override;
    bool propagate(Solver& solver, Lit p) override;
    void calcReason(Solver& solver, Lit p, Lit extra, vec<Lit>& out_reason) override;
    void undo(Solver& solver, Lit p) override;

private:
    int varIndex(Var v) const;

    std::vector<int> value_;
    std::vector<Var> vars_;
    int parity_, n_undecided_;
};

}
