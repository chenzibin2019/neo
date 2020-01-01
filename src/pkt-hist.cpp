#include "pkt-hist.hpp"

#include <typeinfo>

#include "middlebox.hpp"

NodePacketHistory::NodePacketHistory(Packet *p, NodePacketHistory *h)
    : last_pkt(p), past_hist(h)
{
}

std::list<Packet *> NodePacketHistory::get_packets() const
{
    std::list<Packet *> packets;

    for (const NodePacketHistory *nph = this; nph; nph = nph->past_hist) {
        packets.push_front(nph->last_pkt);
    }

    return packets;
}

bool operator==(const NodePacketHistory& a, const NodePacketHistory& b)
{
    return (a.last_pkt == b.last_pkt && a.past_hist == b.past_hist);
}

/******************************************************************************/

PacketHistory::PacketHistory(const Network& network)
{
    const std::map<std::string, Node *>& nodes = network.get_nodes();

    for (const auto& pair : nodes) {
        Node *node = pair.second;
        if (typeid(*node) == typeid(Middlebox)) {
            tbl.emplace(node, nullptr);
        }
    }
}

void PacketHistory::set_node_pkt_hist(Node *node, NodePacketHistory *nph)
{
    tbl[node] = nph;
}

NodePacketHistory *PacketHistory::get_node_pkt_hist(Node *node)
{
    return tbl.at(node);
}

bool operator==(const PacketHistory& a, const PacketHistory& b)
{
    return a.tbl == b.tbl;
}

/******************************************************************************/

size_t NodePacketHistoryHash::operator()(NodePacketHistory *const& nph) const
{
    size_t value = 0;
    std::hash<Packet *> pkt_hf;
    std::hash<NodePacketHistory *> nph_hf;
    ::hash::hash_combine(value, pkt_hf(nph->last_pkt));
    ::hash::hash_combine(value, nph_hf(nph->past_hist));
    return value;
}

bool NodePacketHistoryEq::operator()(NodePacketHistory *const& a,
                                     NodePacketHistory *const& b) const
{
    return *a == *b;
}

size_t PacketHistoryHash::operator()(PacketHistory *const& ph) const
{
    size_t value = 0;
    std::hash<Node *> node_hf;
    std::hash<NodePacketHistory *> nph_hf;
    for (const auto& entry : ph->tbl) {
        ::hash::hash_combine(value, node_hf(entry.first));
        ::hash::hash_combine(value, nph_hf(entry.second));
    }
    return value;
}

bool PacketHistoryEq::operator()(PacketHistory *const& a,
                                 PacketHistory *const& b) const
{
    return *a == *b;
}
