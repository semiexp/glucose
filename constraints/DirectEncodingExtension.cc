#include "constraints/DirectEncodingExtension.h"

#include <algorithm>
#include <set>

namespace Glucose {

DirectEncodingExtensionSupports::DirectEncodingExtensionSupports(std::vector<std::vector<Lit>>&& vars, std::vector<std::vector<int>>&& supports) : vars_(std::move(vars)), supports_(std::move(supports)), known_values_(vars_.size(), -1) {
    for (int i = 0; i < vars_.size(); ++i) {
        for (int j = 0; j < vars_[i].size(); ++j) {
            lits_.push_back({vars_[i][j], i, j});
        }
    }
    std::sort(lits_.begin(), lits_.end());
}

bool DirectEncodingExtensionSupports::initialize(Solver& solver) {
    std::set<Lit> watchers_set;
    for (int i = 0; i < lits_.size(); ++i) {
        watchers_set.insert(std::get<0>(lits_[i]));
    }
    for (Lit l : watchers_set) {
        solver.addWatch(l, this);
    }
    for (Lit l : watchers_set) {
        lbool val = solver.value(l);
        if (val == l_True) {
            if (!propagate(solver, l)) return false;
        }
    }
    if (supports_.size() == 0 && vars_.size() > 0) return false;
    return true;
}

bool DirectEncodingExtensionSupports::propagate(Solver& solver, Lit p) {
    solver.registerUndo(var(p), this);

    active_lits_.push_back(p);
    undo_list_.push_back(-1);

    for (auto it = std::lower_bound(lits_.begin(), lits_.end(), std::make_tuple(p, -1, -1)); it != lits_.end() && std::get<0>(*it) == p; ++it) {
        int i = std::get<1>(*it), j = std::get<2>(*it);

        if (known_values_[i] != -1) {
            if (known_values_[i] == j) continue;
            else return false;
        }

        undo_list_.push_back(i);
        known_values_[i] = j;
    }

    std::vector<std::vector<int>> occurrence;
    int n_match = 0;
    for (int i = 0; i < vars_.size(); ++i) {
        occurrence.push_back(std::vector<int>(vars_[i].size() + 1, 0));
    }

    // TODO: use faster algorithm
    for (int i = 0; i < supports_.size(); ++i) {
        bool match = true;
        for (int j = 0; j < vars_.size(); ++j) {
            int x = supports_[i][j];
            int y = known_values_[j];
            if (x != y && x != -1 && y != -1) {
                match = false;
                break;
            }
        }
        if (!match) continue;

        ++n_match;
        for (int j = 0; j < vars_.size(); ++j) {
            occurrence[j][supports_[i][j] + 1] = 1;
        }
    }

    if (n_match == 0) {
        return false;
    }

    for (int i = 0; i < vars_.size(); ++i) {
        if (occurrence[i][0] != 0) continue;
        
        for (int j = 0; j < vars_[i].size(); ++j) {
            if (occurrence[i][j + 1] == 0) {
                if (!solver.enqueue(~vars_[i][j], this)) return false;
            }
        }
    }

    return true;
}

void DirectEncodingExtensionSupports::calcReason(Solver& solver, Lit p, Lit extra, vec<Lit>& out_reason) {
    for (Lit l : active_lits_) {
        out_reason.push(l);
    }
    if (extra != lit_Undef) out_reason.push(extra);
}

void DirectEncodingExtensionSupports::undo(Solver& solver, Lit p) {
    for (;;) {
        int i = undo_list_.back();
        undo_list_.pop_back();
        if (i < 0) break;

        known_values_[i] = -1;
    }
    active_lits_.pop_back();
}

}
