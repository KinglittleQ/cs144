#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

#include <iostream>

// Dummy implementation of a network interface
// Translates from {IP datagram, next hop address} to link-layer frame, and from link-layer frame to IP datagram

// For Lab 5, please replace with a real implementation that passes the
// automated checks run by `make check_lab5`.

// You will need to add private members to the class declaration in `network_interface.hh`

const EthernetAddress BROADCAST_ADDRESS{0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface(const EthernetAddress &ethernet_address, const Address &ip_address)
    : _ethernet_address(ethernet_address), _ip_address(ip_address) {
    cerr << "DEBUG: Network interface has Ethernet address " << to_string(_ethernet_address) << " and IP address "
         << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but may also be another host if directly connected to the same network as the destination)
//! (Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) with the Address::ipv4_numeric() method.)
void NetworkInterface::send_datagram(const InternetDatagram &dgram, const Address &next_hop) {
    // convert IP address of next hop to raw 32-bit representation (used in ARP header)
    const uint32_t next_hop_ip = next_hop.ipv4_numeric();

    EthernetFrame frame;
    frame.header().src = _ethernet_address;

    const auto iter = map_.find(next_hop_ip);
    if (iter == map_.cend() || timestamp_ - iter->second.time_updated > 30000) {
        // Not found or expire

        unsent_datagrams_.emplace_back(dgram, next_hop_ip);

        auto list_iter = unreplied_arp_requests_.begin();
        while (list_iter != unreplied_arp_requests_.end() && list_iter->ip_addr != next_hop_ip) {
            list_iter++;
        }

        // Test if already sent a request in 5 seconds
        if (list_iter != unreplied_arp_requests_.end()) {
            uint64_t duration = timestamp_ - list_iter->time_sent;
            if (duration <= 5000) {
                return;
            }
        }

        ARPMessage arp_msg;
        arp_msg.opcode = ARPMessage::OPCODE_REQUEST;
        arp_msg.sender_ethernet_address = _ethernet_address;
        arp_msg.sender_ip_address = _ip_address.ipv4_numeric();
        arp_msg.target_ip_address = next_hop_ip;

        frame.payload() = arp_msg.serialize();
        frame.header().dst = BROADCAST_ADDRESS;
        frame.header().type = EthernetHeader::TYPE_ARP;

        ARPRequest request;
        request.ip_addr = next_hop_ip;
        request.time_sent = timestamp_;
        unreplied_arp_requests_.push_back(request);

    } else {
        // Found

        EthernetAddress dst = iter->second.eth_addr;
        frame.payload() = dgram.serialize();
        frame.header().dst = dst;
        frame.header().type = EthernetHeader::TYPE_IPv4;
    }

    _frames_out.push(frame);
}

//! \param[in] frame the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame(const EthernetFrame &frame) {
    const auto &header = frame.header();

    if (header.dst != _ethernet_address && header.dst != BROADCAST_ADDRESS) {
        return std::nullopt;
    }

    if (header.type == EthernetHeader::TYPE_IPv4) {
        InternetDatagram dgram;
        if (dgram.parse(frame.payload()) == ParseResult::NoError) {
            return dgram;
        }
        return std::nullopt;
    }

    if (header.type == EthernetHeader::TYPE_ARP) {
        ARPMessage arp_msg;
        if (arp_msg.parse(frame.payload()) != ParseResult::NoError) {
            return std::nullopt;
        }

        uint32_t ip_addr = arp_msg.sender_ip_address;
        EthernetAddress eth_addr = arp_msg.sender_ethernet_address;
        EthernetAdressInfo addr_info;
        addr_info.eth_addr = eth_addr;
        addr_info.time_updated = timestamp_;
        map_[ip_addr] = addr_info;

        if (arp_msg.opcode == ARPMessage::OPCODE_REQUEST) {
            if (arp_msg.target_ip_address == _ip_address.ipv4_numeric()) {
                // send an ARP reply
                ARPMessage reply_arp;
                reply_arp.opcode = ARPMessage::OPCODE_REPLY;
                reply_arp.sender_ip_address = _ip_address.ipv4_numeric();
                reply_arp.sender_ethernet_address = _ethernet_address;
                reply_arp.target_ip_address = arp_msg.sender_ip_address;
                reply_arp.target_ethernet_address = arp_msg.sender_ethernet_address;

                EthernetFrame reply_frame;
                reply_frame.payload() = reply_arp.serialize();

                reply_frame.header().dst = header.src;
                reply_frame.header().src = _ethernet_address;
                reply_frame.header().type = EthernetHeader::TYPE_ARP;

                _frames_out.push(reply_frame);
            }
        } else if (arp_msg.opcode == ARPMessage::OPCODE_REPLY) {
            auto iter1 = unreplied_arp_requests_.begin();
            while (iter1 != unreplied_arp_requests_.end()) {
                if (iter1->ip_addr == arp_msg.sender_ip_address) {
                    iter1 = unreplied_arp_requests_.erase(iter1);
                } else {
                    iter1++;
                }
            }

            auto iter2 = unsent_datagrams_.begin();
            while (iter2 != unsent_datagrams_.end()) {
                if (iter2->second == arp_msg.sender_ip_address) {
                    EthernetFrame f;
                    f.header().src = _ethernet_address;
                    f.header().dst = arp_msg.sender_ethernet_address;
                    f.payload() = iter2->first.serialize();
                    f.header().type = EthernetHeader::TYPE_IPv4;
                    _frames_out.push(f);

                    // send_datagram(iter2->first, iter2->second);
                    iter2 = unsent_datagrams_.erase(iter2);
                } else {
                    iter2++;
                }
            }
        } else {
            return std::nullopt;
        }
    }

    return std::nullopt;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick(const size_t ms_since_last_tick) { timestamp_ += ms_since_last_tick; }
