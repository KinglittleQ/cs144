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

    _routing_table.emplace_back(route_prefix, prefix_length, next_hop, interface_num);
}

//! \param[in] dgram The datagram to be routed
void Router::route_one_datagram(InternetDatagram &dgram) {
    uint32_t dst_ip = dgram.header().dst;
    int max_prefix_length = -1;
    size_t matched_route;
    std::cout << Address::from_ipv4_numeric(dgram.header().dst).ip() << std::endl;
    for (size_t i = 0; i < _routing_table.size(); i++) {
        auto &entry = _routing_table[i];
        uint32_t postfix_length = 32 - entry.prefix_length;
        uint32_t prefix_shifted = entry.prefix_length > 0 ? (entry.route_prefix >> postfix_length) : 0;
        uint32_t dst_shifted = entry.prefix_length > 0 ? (dst_ip >> postfix_length) : 0;
        if (prefix_shifted == dst_shifted && max_prefix_length < entry.prefix_length) {
            max_prefix_length = entry.prefix_length;
            matched_route = i;
        }
    }

    if (max_prefix_length != -1) {
        if (dgram.header().ttl == 0 || (--dgram.header().ttl) == 0) {
            return;
        }
        const auto &matched_interface = _routing_table[matched_route];
        if (matched_interface.next_hop.has_value()) {
            Address next_hop = matched_interface.next_hop.value();
            interface(matched_interface.interface_num).send_datagram(dgram, next_hop);
        } else {
            Address next_hop = Address::from_ipv4_numeric(dst_ip);
            interface(matched_interface.interface_num).send_datagram(dgram, next_hop);
        }
    }
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
