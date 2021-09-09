#include "constraints/OrderEncodingLinear.h"

#include <algorithm>
#include <set>

namespace Glucose {

void LinearTerm::negate_coef() {
    coef *= -1;

    std::reverse(lits.begin(), lits.end());
    for (auto& lit : lits) lit = ~lit;

    std::reverse(domain.begin(), domain.end());
    for (auto& d : domain) d *= -1;
}

OrderEncodingLinear::OrderEncodingLinear(std::vector<LinearTerm>&& terms, int constant) {
    terms_ = std::move(terms);
    for (auto& term : terms_) {
        if (term.coef == 0) {
            abort();
        }
        if (term.coef < 0) {
            term.negate_coef();
        }
    }
    constant_ = constant;
    total_ub_ = constant;

    for (int i = 0; i < terms_.size(); ++i) {
        ub_index_.push_back(terms_[i].lits.size());
        total_ub_ += terms_[i].coef * terms_[i].domain.back();
        for (int j = 0; j < terms_[i].lits.size(); ++j) {
            lits_.push_back({terms_[i].lits[j], i, j});
        }
    }
    std::sort(lits_.begin(), lits_.end());
}

bool OrderEncodingLinear::initialize(Solver& solver, vec<Lit>& out_watchers) {
    std::set<Lit> watchers_set;
    for (int i = 0; i < lits_.size(); ++i) {
        watchers_set.insert(~std::get<0>(lits_[i]));
    }
    for (Lit l : watchers_set) {
        out_watchers.push(l);
    }
    for (Lit l : watchers_set) {
        lbool val = solver.value(l);
        if (val == l_True) {
            if (!propagate(solver, l)) return false;
        }
    }
    if (total_ub_ < 0) return false;
    return true;
}

bool OrderEncodingLinear::propagate(Solver& solver, Lit p) {
    solver.registerUndo(var(p), this);

    active_lits_.push_back(p);
    undo_list_.push_back({-1, -1});

    for (auto it = std::lower_bound(lits_.begin(), lits_.end(), std::make_tuple(~p, -1, -1)); it != lits_.end() && std::get<0>(*it) == ~p; ++it) {
        int i = std::get<1>(*it), j = std::get<2>(*it);

        if (ub_index_[i] <= j) continue;

        undo_list_.push_back({i, ub_index_[i]});
        total_ub_ -= (terms_[i].domain[ub_index_[i]] - terms_[i].domain[j]) * terms_[i].coef;
        ub_index_[i] = j;

        if (total_ub_ < 0) return false;
    }

    for (int i = 0; i < terms_.size(); ++i) {
        int ubi = ub_index_[i];
        if (ubi == 0) continue;
        int coef = terms_[i].coef;
        if (total_ub_ - (terms_[i].domain[ubi] - terms_[i].domain[0]) * coef >= 0) continue;

        // largest j s.t. total_ub_ - (terms_[i].domain[ubi] - terms_[i].domain[j]) * coef < 0
        int left = 0, right = ubi - 1;
        while (left < right) {
            int mid = (left + right + 1) / 2;
            if (total_ub_ - (terms_[i].domain[ubi] - terms_[i].domain[mid]) * coef < 0) {
                left = mid;
            } else {
                right = mid - 1;
            }
        }

        // TODO: do not enqueue known facts again
        if (!solver.enqueue(terms_[i].lits[left], this)) return false;
    }

    return true;
}

void OrderEncodingLinear::calcReason(Solver& solver, Lit p, Lit extra, vec<Lit>& out_reason) {
    for (Lit l : active_lits_) out_reason.push(l);
    if (extra != lit_Undef) out_reason.push(extra);
}

void OrderEncodingLinear::undo(Solver& solver, Lit p) {
    for (;;) {
        int i = undo_list_.back().first, j = undo_list_.back().second;
        undo_list_.pop_back();

        if (i < 0) break;

        total_ub_ += (terms_[i].domain[j] - terms_[i].domain[ub_index_[i]]) * terms_[i].coef;
        ub_index_[i] = j;
    }
    active_lits_.pop_back();
}

}