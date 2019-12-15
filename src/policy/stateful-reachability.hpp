#pragma once

#include <vector>
#include <unordered_set>
#include <cpptoml/cpptoml.hpp>

#include "policy/policy.hpp"
#include "node.hpp"

/*
 * For all possible packets starting from any of start_nodes, with destination
 * address within pkt_dst, the packet should eventually be accepted by one of
 * final_nodes when reachable is true. Otherwise, if reachable is false, the
 * packet should either be dropped, or be accepted by none of the final_nodes.
 *
 * The policy states that the above should be true when the prerequisite policy
 * is verified to be true. If the prerequisite policy is violated, the policy
 * holds.
 */
class StatefulReachabilityPolicy : public Policy
{
private:
    std::vector<Node *> start_nodes;
    std::unordered_set<Node *> final_nodes;
    bool reachable;
    Policy *prerequisite;
    EqClasses ECs;  // ECs to be verified

public:
    StatefulReachabilityPolicy(const std::shared_ptr<cpptoml::table>&,
                               const Network&);
    ~StatefulReachabilityPolicy() override;

    const EqClasses& get_pre_ecs() const override;
    const EqClasses& get_ecs() const override;
    size_t num_ecs() const override;
    void compute_ecs(const EqClasses&) override;
    std::string to_string() const override;
    std::string get_type() const override;
    void init(State *) override;
    void config_procs(State *, const Network&, ForwardingProcess&) const
    override;
    void check_violation(State *) override;
};
