#include <csignal>
#include <unistd.h>

#include "policy/policy.hpp"
#include "lib/logger.hpp"
#include "policy/reachability.hpp"
#include "policy/reply-reachability.hpp"
#include "policy/stateful-reachability.hpp"
#include "policy/waypoint.hpp"

Policy::Policy(const std::shared_ptr<cpptoml::table>& config)
{
    static int next_id = 1;
    id = next_id++;

    auto pkt_dst_str = config->get_as<std::string>("pkt_dst");

    if (!pkt_dst_str) {
        Logger::get().err("Missing packet destination");
    }

    std::string dst_str = *pkt_dst_str;
    if (dst_str.find('/') == std::string::npos) {
        dst_str += "/32";
    }
    pkt_dst = IPRange<IPv4Address>(dst_str);
}

int Policy::get_id() const
{
    return id;
}

const IPRange<IPv4Address>& Policy::get_pkt_dst() const
{
    return pkt_dst;
}

const EqClasses& Policy::get_pre_ecs() const
{
    static const EqClasses null_pre_ECs = EqClasses(nullptr);
    return null_pre_ECs;
}

const EqClasses& Policy::get_ecs() const
{
    static const EqClasses null_ECs = EqClasses(nullptr);
    return null_ECs;
}

size_t Policy::num_ecs() const
{
    return 0;
}

void Policy::report(State *state) const
{
    if (state->network_state[state->itr_ec].violated) {
        Logger::get().info("Policy violated!");
        kill(getppid(), SIGUSR1);
    } else {
        Logger::get().info("Policy holds!");
    }
}

Policies::Policies(const std::shared_ptr<cpptoml::table_array>& configs,
                   const Network& network)
{
    if (configs) {
        for (auto config : *configs) {
            Policy *policy = nullptr;
            auto type = config->get_as<std::string>("type");

            if (!type) {
                Logger::get().err("Missing policy type");
            }

            if (*type == "reachability") {
                policy = new ReachabilityPolicy(config, network);
            } else if (*type == "waypoint") {
                policy = new WaypointPolicy(config, network);
            } else if (*type == "reply-reachability") {
                policy = new ReplyReachabilityPolicy(config, network);
            } else if (*type == "stateful-reachability") {
                policy = new StatefulReachabilityPolicy(config, network);
            } else {
                Logger::get().err("Unknown policy type: " + *type);
            }

            policies.push_back(policy);
        }
    }
    if (policies.size() == 1) {
        Logger::get().info("Loaded 1 policy");
    } else {
        Logger::get().info("Loaded " + std::to_string(policies.size())
                           + " policies");
    }
    compute_ecs(network);
}

Policies::~Policies()
{
    for (Policy *policy : policies) {
        delete policy;
    }
}

void Policies::compute_ecs(const Network& network) const
{
    EqClasses all_ECs;

    for (const auto& node : network.get_nodes()) {
        for (const auto& intf : node.second->get_intfs_l3()) {
            all_ECs.add_ec(intf.first);
        }
        for (const Route& route : node.second->get_rib()) {
            all_ECs.add_ec(route.get_network());
        }
    }

    for (Policy *policy : policies) {
        policy->compute_ecs(all_ECs);
    }
}

Policies::iterator Policies::begin()
{
    return policies.begin();
}

Policies::const_iterator Policies::begin() const
{
    return policies.begin();
}

Policies::iterator Policies::end()
{
    return policies.end();
}

Policies::const_iterator Policies::end() const
{
    return policies.end();
}

Policies::reverse_iterator Policies::rbegin()
{
    return policies.rbegin();
}

Policies::const_reverse_iterator Policies::rbegin() const
{
    return policies.rbegin();
}

Policies::reverse_iterator Policies::rend()
{
    return policies.rend();
}

Policies::const_reverse_iterator Policies::rend() const
{
    return policies.rend();
}
