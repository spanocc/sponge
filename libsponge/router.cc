#include "router.hh"

#include <iostream>

using namespace std;

// Dummy implementation of an IP router

// Given an incoming Internet datagram, the router decides
// (1) which interface to send it out on, and
// (2) what next hop address to send it to.

// For Lab 6, please replace with a real implementation that passes the
// automated checks run by `make check_lab6`.

// You will need to add private members to the class declaration in `router.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

//! \param[in] route_prefix The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
//! \param[in] prefix_length For this route to be applicable, how many high-order (most-significant) bits of the route_prefix will need to match the corresponding bits of the datagram's destination address?
//! \param[in] next_hop The IP address of the next hop. Will be empty if the network is directly attached to the router (in which case, the next hop address should be the datagram's final destination).
//! \param[in] interface_num The index of the interface to send the datagram out on.
void Router::add_route(const uint32_t route_prefix,
                       const uint8_t prefix_length,
                       const optional<Address> next_hop,
                       const size_t interface_num) {
    cerr << "DEBUG: adding route " << Address::from_ipv4_numeric(route_prefix).ip() << "/" << int(prefix_length)
         << " => " << (next_hop.has_value() ? next_hop->ip() : "(direct)") << " on interface " << interface_num << "\n";

    // DUMMY_CODE(route_prefix, prefix_length, next_hop, interface_num);
    // Your code here.
    RouteEntry re;
    re.route_prefix = route_prefix;
    re.prefix_length = prefix_length;
    re.next_hop = next_hop;
    re.interface_num = interface_num;
    _route_table.push_back(re);
}

bool Router::route_match(const uint32_t& re, const uint32_t& ip, uint8_t plen) {
    if(plen > 32) return false;
    if(plen == 0) return true;
    uint32_t mask = ~((1 << (32 - plen)) - 1); // 前plen位
    return ((re & mask) == (ip & mask));
}

//! \param[in] dgram The datagram to be routed
void Router::route_one_datagram(InternetDatagram &dgram) {
    // DUMMY_CODE(dgram);
    // Your code here.
    if(dgram.header().ttl == 0 || --dgram.header().ttl == 0) return;
    uint32_t idx = 0, max_len = 0;
    for(size_t i = 0; i < _route_table.size(); ++i) {
        const auto& it = _route_table[i];
        if(it.prefix_length > max_len && route_match(it.route_prefix, dgram.header().dst, it.prefix_length)) {
            idx = i;
            max_len = it.prefix_length;
        }
    }
    uint32_t ip = dgram.header().dst;
    if(_route_table[idx].next_hop.has_value()) ip = _route_table[idx].next_hop.value().ipv4_numeric();
    interface(_route_table[idx].interface_num).send_datagram(dgram, Address::from_ipv4_numeric(ip));
}

void Router::route() {
    // Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
    for (auto &interface : _interfaces) {
        auto &queue = interface.datagrams_out();
        while (not queue.empty()) {
            route_one_datagram(queue.front());
            queue.pop();
        }
    }
}
