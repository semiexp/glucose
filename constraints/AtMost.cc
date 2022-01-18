#include "constraints/AtMost.h"

#include <algorithm>
#include <cassert>

namespace Glucose {

AtMost::AtMost(std::vector<Lit>&& lits, int threshold) : lits_(std::move(lits)), threshold_(threshold), n_true_(0) {
    std::sort(lits_.begin(), lits_.end());
    for (size_t i = 1; i < lits_.size(); ++i) {
        assert(lits_[i - 1] != lits_[i]);
    }
}

bool AtMost::initialize(Solver& solver) {
    for (size_t i = 0; i < lits_.size(); ++i) {
        solver.addWatch(lits_[i], this);
    }

    for (size_t i = 0; i < lits_.size(); ++i) {
        lbool val = solver.value(lits_[i]);
        if (val == l_True) {
            if (!propagate(solver, lits_[i])) return false;
        }
    }

    if (n_true_ > threshold_) return false;
    if (n_true_ == threshold_) {
        std::vector<Lit> to_push;
        for (size_t i = 0; i < lits_.size(); ++i) {
            if (solver.value(lits_[i]) == l_Undef) {
                to_push.push_back(~lits_[i]);
            }
        }
        for (Lit lit : to_push) {
            if (!solver.enqueue(lit, this)) return false;
        }
    }

    return true;
}

bool AtMost::propagate(Solver& solver, Lit p) {
    ++n_true_;
    solver.registerUndo(var(p), this);

    if (n_true_ > threshold_) return false;
    else if (n_true_ == threshold_) {
        for (size_t i = 0; i < lits_.size(); ++i) {
            lbool val = solver.value(lits_[i]);

            if (val == l_Undef) {
                if (!solver.enqueue(~lits_[i], this)) {
                    return false;
                }
            }
        }
    }

    return true;
}

void AtMost::calcReason(Solver& solver, Lit p, Lit extra, vec<Lit>& out_reason) {
    if (extra != lit_Undef) {
        out_reason.push(extra);
    }
    for (size_t i = 0; i < lits_.size(); ++i) {
        if (solver.value(lits_[i]) == l_True && p != lits_[i]) {
            out_reason.push(lits_[i]);
        }
    }
}

void AtMost::undo(Solver&, Lit) {
    --n_true_;
}

}
