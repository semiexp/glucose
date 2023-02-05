#include "constraints/GraphDivision.h"

#include <set>
#include <algorithm>
#include <vector>
#include <queue>

namespace Glucose {

GraphDivision::GraphDivision(const std::vector<OptionalOrderEncoding>& vertices, const std::vector<std::pair<int, int>>& edges, const std::vector<Lit>& edge_lits) :
    vertices_(vertices),
    adj_(vertices_.size()),
    edge_lits_(edge_lits),
    edge_state_(edge_lits_.size(), EdgeState::kUndecided),
    size_lb_(vertices_.size(), -1),
    size_ub_(vertices_.size(), -1),
    size_lb_reason_(vertices_.size(), std::nullopt),
    size_ub_reason_(vertices_.size(), std::nullopt),
    decided_regions_(vertices_.size()),
    decided_region_id_(vertices_.size()),
    potential_regions_(vertices_.size()),
    potential_region_id_(vertices_.size())
{
    for (int i = 0; i < edges.size(); ++i) {
        auto [s, t] = edges[i];
        assert(s != t);
        adj_[s].push_back({t, i});
        adj_[t].push_back({s, i});
    }
}

bool GraphDivision::initialize(Solver& solver) {
    std::set<Lit> lits_unique;

    for (int i = 0; i < vertices_.size(); ++i) {
        for (int j = 0; j < vertices_[i].lits.size(); ++j) {
            Lit l = vertices_[i].lits[j];
            lits_unique.insert(l);
            lits_unique.insert(~l);
        }
    }
    for (Lit l : edge_lits_) {
        lits_unique.insert(l);
        lits_unique.insert(~l);
    }
    for (Lit l : lits_unique) {
        solver.addWatch(l, this);
    }

    if (run_check(solver).has_value()) {
        return false;
    }

    return true;
}

bool GraphDivision::propagate(Solver& solver, Lit p) {
    solver.registerUndo(var(p), this);

    if (num_pending_propagation() > 0) {
        // lazy propagation
        reasons_.push_back({});
        return true;
    }

    auto res = run_check(solver);

    if (res.has_value()) {
        reasons_.push_back(*res);
        auto& r = reasons_.back();
        std::sort(r.begin(), r.end());
        r.erase(std::unique(r.begin(), r.end()), r.end());

        return false;
    }

    reasons_.push_back({});
    return true;
}

void GraphDivision::calcReason(Solver& solver, Lit p, Lit extra, vec<Lit>& out_reason) {
    assert(!reasons_.empty());

    for (auto l : reasons_.back()) {
        out_reason.push(l);
    }
    if (extra != lit_Undef) {
        out_reason.push(extra);
    }
}

void GraphDivision::undo(Solver& solver, Lit p) {
    assert(!reasons_.empty());
    reasons_.pop_back();
}

std::optional<std::vector<Lit>> GraphDivision::run_check(Solver& solver) {
    for (int i = 0; i < edge_lits_.size(); ++i) {
        lbool v = solver.value(edge_lits_[i]);

        // edge_lits_[i] corresponds to the presence of a "border"
        if (v == l_True) {
            edge_state_[i] = EdgeState::kDisconnected;
        } else if (v == l_False) {
            edge_state_[i] = EdgeState::kConnected;
        } else {
            edge_state_[i] = EdgeState::kUndecided;
        }
    }

    for (int i = 0; i < vertices_.size(); ++i) {
        auto& v = vertices_[i];
        if (v.is_absent()) continue;

        {
            int left = 0;
            int right = v.values.size() - 1;
            while (left < right) {
                int mid = (left + right + 1) / 2;
                if (solver.value(v.lits[mid - 1]) == l_True) {
                    left = mid;
                } else {
                    right = mid - 1;
                }
            }
            size_lb_[i] = v.values[left];
            if (left == 0) {
                size_lb_reason_[i] = std::nullopt;
            } else {
                size_lb_reason_[i] = v.lits[left - 1];
            }
        }
        {
            int left = 0;
            int right = v.values.size() - 1;
            while (left < right) {
                int mid = (left + right) / 2;
                if (solver.value(v.lits[mid]) == l_False) {
                    right = mid;
                } else {
                    left = mid + 1;
                }
            }
            size_ub_[i] = v.values[left];
            if (left == v.values.size() - 1) {
                size_ub_reason_[i] = std::nullopt;
            } else {
                size_ub_reason_[i] = ~v.lits[left];
            }
        }
    }

    int n_decided_regions = compute_regions(decided_regions_, decided_region_id_, false);
    int n_potential_regions = compute_regions(potential_regions_, potential_region_id_, true);

    // 1. There must not exist borders whose both sides are in the same region
    {
        for (int u = 0; u < vertices_.size(); ++u) {
            for (auto& [v, e] : adj_[u]) {
                if (decided_region_id_[u] == decided_region_id_[v] && edge_state_[e] == EdgeState::kDisconnected) {
                    std::vector<Lit> ret = reason_decided_connecting_path(u, v);
                    ret.push_back(edge_lits_[e]);
                    return ret;
                }
            }
        }
    }

    // 2. Within a decided region,
    //   - Size variables of its members must be at least the size of the region
    //   - All the values of size variables must be identical
    {
        for (int r = 0; r < n_decided_regions; ++r) {
            int r_size = decided_regions_[r].size();

            int least_ub = vertices_.size();
            int least_ub_pos = -1;
            for (int p : decided_regions_[r]) {
                if (vertices_[p].is_absent()) continue;

                if (least_ub > size_ub_[p]) {
                    least_ub = size_ub_[p];
                    least_ub_pos = p;
                }
                if (size_ub_[p] < r_size) {
                    std::vector<Lit> ret = reason_decided_region(r);
                    if (size_ub_reason_[p].has_value()) {
                        ret.push_back(*size_ub_reason_[p]);
                    }
                    return ret;
                }
            }

            for (int p : decided_regions_[r]) {
                if (vertices_[p].is_absent()) continue;

                if (least_ub < size_lb_[p]) {
                    assert(least_ub != -1);

                    std::vector<Lit> ret = reason_decided_connecting_path(least_ub_pos, p);
                    if (size_ub_reason_[least_ub_pos].has_value()) {
                        ret.push_back(*size_ub_reason_[least_ub_pos]);
                    }
                    if (size_lb_reason_[p].has_value()) {
                        ret.push_back(*size_lb_reason_[p]);
                    }

                    return ret;
                }
            }
        }
    }

    // 3. Within a potential region, size variables of its members are at most the size of the region
    {
        for (int r = 0; r < n_potential_regions; ++r) {
            int r_size = potential_regions_[r].size();

            for (int p : potential_regions_[r]) {
                if (vertices_[p].is_absent()) continue;

                if (size_lb_[p] > r_size) {
                    std::vector<Lit> ret = reason_potential_region(r);
                    if (size_lb_reason_[p].has_value()) {
                        ret.push_back(*size_lb_reason_[p]);
                    }

                    return ret;
                }
            }
        }
    }
    return std::nullopt;
}

int GraphDivision::compute_regions(std::vector<std::vector<int>>& regions, std::vector<int>& region_id, bool is_potential) {
    int n_regions = 0;

    std::fill(region_id.begin(), region_id.end(), -1);
    for (int i = 0; i < vertices_.size(); ++i) {
        if (region_id[i] >= 0) {
            continue;
        }
        regions[n_regions].clear();
        compute_regions_dfs(i, n_regions, regions[n_regions], region_id, is_potential);
        ++n_regions;
    }

    return n_regions;
}

void GraphDivision::compute_regions_dfs(int p, int id, std::vector<int>& region, std::vector<int>& region_id, bool is_potential) {
    if (region_id[p] >= 0) {
        return;
    }
    region.push_back(p);
    region_id[p] = id;

    for (auto [q, e] : adj_[p]) {
        EdgeState state = edge_state_[e];
        if (state == EdgeState::kConnected || (is_potential && state == EdgeState::kUndecided)) {
            compute_regions_dfs(q, id, region, region_id, is_potential);
        }
    }
}

std::vector<Lit> GraphDivision::reason_decided_connecting_path(int src, int dest) {
    if (src == dest) {
        return {};
    }

    std::vector<std::pair<int, int>> origin(vertices_.size(), {-2, -2});
    std::queue<int> qu;

    origin[src] = {-1, -1};
    qu.push(src);

    while (!qu.empty()) {
        int u = qu.front(); qu.pop();

        for (auto [v, e] : adj_[u]) {
            if (edge_state_[e] != EdgeState::kConnected) {
                continue;
            }
            if (origin[v].first != -2) {
                continue;
            }
            origin[v] = {u, e};
            if (v == dest) {
                break;
            }
            qu.push(v);
        }
    }

    assert(origin[dest].first != -2);

    std::vector<Lit> ret;
    int p = dest;
    while (p != src) {
        ret.push_back(~edge_lits_[origin[p].second]);
        p = origin[p].first;
    }

    return ret;
}

std::vector<Lit> GraphDivision::reason_decided_region(int id) {
    std::vector<Lit> ret;

    for (int p : decided_regions_[id]) {
        for (auto [q, e] : adj_[p]) {
            if (edge_state_[e] == EdgeState::kConnected) {
                assert(decided_region_id_[q] == id);
                if (p < q) {
                    ret.push_back(~edge_lits_[e]);
                }
            }
        }
    }

    return ret;
}

std::vector<Lit> GraphDivision::reason_potential_region(int id) {
    std::vector<Lit> ret;

    for (int p : potential_regions_[id]) {
        for (auto [q, e] : adj_[p]) {
            if (potential_region_id_[q] != id) {
                assert(edge_state_[e] == EdgeState::kDisconnected);
                ret.push_back(edge_lits_[e]);
            }
        }
    }

    return ret;
}

}
