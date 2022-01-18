#pragma once

#include "mtl/Vec.h"
#include "core/SolverTypes.h"

#include <cstdio>

namespace Glucose {

class Solver;

class Constraint {
public:
    Constraint() : num_pending_propagation_(0) {}
    virtual ~Constraint() {}

    virtual bool initialize(Solver& solver) = 0;
    virtual bool propagate(Solver& solver, Lit p) = 0;
    virtual void calcReason(Solver& solver, Lit p, Lit extra, vec<Lit>& out_reason) = 0;
    virtual void undo(Solver& solver, Lit p) {}

    int num_pending_propagation() const { return num_pending_propagation_; }

private:
    friend class Solver;

    int num_pending_propagation_;
};

}
