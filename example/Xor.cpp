#include "example/Xor.h"

#include <algorithm>
#include <set>

namespace Glucose {

Xor::Xor(const std::vector<Lit>& lits, int parity) {
    std::set<Var> vars_norm;
    for (Lit l : lits) {
        if (sign(l)) parity ^= 1;
        Var v = var(l);
        if (vars_norm.count(v)) vars_norm.erase(v);
        else vars_norm.insert(v);
    }

    vars_ = std::vector<Var>(vars_norm.begin(), vars_norm.end());
    value_ = std::vector<int>(vars_.size(), -1);
    parity_ = parity;
    n_undecided_ = vars_.size();
}

bool Xor::initialize(Solver& solver, vec<Lit>& out_watchers) {
    std::vector<Lit> propagate_lits;

    for (int i = 0; i < vars_.size(); ++i) {
        out_watchers.push(mkLit(vars_[i]));
        out_watchers.push(mkLit(vars_[i], true));
    }
    for (int i = 0; i < vars_.size(); ++i) {
        lbool val = solver.value(vars_[i]);
        if (val == l_True) {
            if (!propagate(solver, mkLit(vars_[i]))) return false;
        } else if (val == l_False) {
            if (!propagate(solver, mkLit(vars_[i], true))) return false;
        }
    }

    if (vars_.size() == 0 && parity_ != 0) return false;
    return true;
}

bool Xor::propagate(Solver& solver, Lit p) {
    int s = sign(p) ? 0 : 1;
    Var v = var(p);
    solver.registerUndo(v, this);

    int idx = varIndex(v);
    assert(value_[idx] == -1);
    value_[idx] = s;
    parity_ ^= s;
    --n_undecided_;

    assert(n_undecided_ >= 0);
    if (n_undecided_ == 0) {
        if (parity_ != 0) return false;
    } else if (n_undecided_ == 1) {
        for (int i = 0; i < vars_.size(); ++i) {
            if (value_[i] == -1) {
                if (!solver.enqueue(mkLit(vars_[i], parity_ == 0), this)) {
                    return false;
                }
            }
        }
    }

    return true;
}

void Xor::calcReason(Solver& solver, Lit p, Lit extra, vec<Lit>& out_reason) {
    for (int i = 0; i < vars_.size(); ++i) {
        if (value_[i] != -1) {
            out_reason.push(mkLit(vars_[i], value_[i] == 0));
        } else if (p == lit_Undef && extra != lit_Undef) {
            out_reason.push(mkLit(vars_[i], parity_ == 1));
        }
    }
}

void Xor::undo(Solver& solver, Lit p) {
    Var v = var(p);

    int idx = varIndex(v);
    assert(value_[idx] == (sign(p) ? 0 : 1));
    parity_ ^= value_[idx];
    value_[idx] = -1;
    ++n_undecided_;
}

int Xor::varIndex(Var v) const {
    int i = std::distance(vars_.begin(), std::lower_bound(vars_.begin(), vars_.end(), v));
    assert(i < vars_.size());
    return i;
}

}
