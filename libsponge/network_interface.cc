#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

#include <iostream>

// Dummy implementation of a network interface
// Translates from {IP datagram, next hop address} to link-layer frame, and from link-layer frame to IP datagram

// For Lab 5, please replace with a real implementation that passes the
// automated checks run by `make check_lab5`.

// You will need to add private members to the class declaration in `network_interface.hh`

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


void NetworkInterface::ARP_request(uint32_t next_hop_ip) {
    if(_ethernet_map.count(next_hop_ip) && _ethernet_map[next_hop_ip].has_mapped) return;

    if(!_ethernet_map.count(next_hop_ip)) {
        EthernetInfo neinfo;
        neinfo.has_mapped = 0;
        neinfo._time_wait_to_respond = 5000; // 初始值设置为5s,因为只有据上一次发送超过5s才会再发
        _ethernet_map.insert({next_hop_ip, neinfo});
    }
    EthernetInfo &einfo = _ethernet_map[next_hop_ip];
    if(einfo._time_wait_to_respond >= 5000) {
        einfo._time_wait_to_respond = 0;
        EthernetFrame ef;
        ef.header().src = _ethernet_address;
        ef.header().dst = ETHERNET_BROADCAST;
        ef.header().type = EthernetHeader::TYPE_ARP;

        ARPMessage arpmg;
        arpmg.opcode = ARPMessage::OPCODE_REQUEST;
        arpmg.sender_ethernet_address = _ethernet_address;
        arpmg.sender_ip_address = _ip_address.ipv4_numeric();
        arpmg.target_ethernet_address = {0, 0, 0, 0, 0, 0};
        arpmg.target_ip_address = next_hop_ip;

        ef.payload() = arpmg.serialize();
        _frames_out.push(ef);
    }
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but may also be another host if directly connected to the same network as the destination)
//! (Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) with the Address::ipv4_numeric() method.)
void NetworkInterface::send_datagram(const InternetDatagram &dgram, const Address &next_hop) {
    // convert IP address of next hop to raw 32-bit representation (used in ARP header)
    const uint32_t next_hop_ip = next_hop.ipv4_numeric();

    //DUMMY_CODE(dgram, next_hop, next_hop_ip);

    if(_ethernet_map.count(next_hop_ip) && _ethernet_map[next_hop_ip].has_mapped) {
        EthernetFrame ef;
        ef.header().src = _ethernet_address;
        ef.header().dst = _ethernet_map[next_hop_ip]._ethernet_address;
        ef.header().type = EthernetHeader::TYPE_IPv4;
        ef.payload() = dgram.serialize();
        _frames_out.push(ef);
    } else {
        ARP_request(next_hop_ip);
        _ipdatagram_out.insert({next_hop_ip, dgram});
    }
}

//! \param[in] frame the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame(const EthernetFrame &frame) {
    // DUMMY_CODE(frame);

    if(frame.header().dst != ETHERNET_BROADCAST && frame.header().dst != _ethernet_address) {
        return nullopt;
    }
    if(frame.header().type == EthernetHeader::TYPE_IPv4) {
        InternetDatagram ipdg;
        if(ipdg.parse(frame.payload()) == ParseResult::NoError) {
            return ipdg;
        }
    } else if(frame.header().type == EthernetHeader::TYPE_ARP){
        ARPMessage arpmg;
        if(arpmg.parse(frame.payload()) == ParseResult::NoError) {
            _ethernet_map[arpmg.sender_ip_address]._ethernet_address = arpmg.sender_ethernet_address;
            _ethernet_map[arpmg.sender_ip_address].has_mapped = 1;
            _ethernet_map[arpmg.sender_ip_address]._time_to_live = 0;
            if(arpmg.opcode == ARPMessage::OPCODE_REPLY && arpmg.target_ip_address == _ip_address.ipv4_numeric()) {
                auto beg = _ipdatagram_out.lower_bound(arpmg.sender_ip_address);
                auto end = _ipdatagram_out.upper_bound(arpmg.sender_ip_address);
                for(;beg != end; ++beg) {
                    EthernetFrame ef;
                    ef.header().src = _ethernet_address;
                    ef.header().dst = arpmg.sender_ethernet_address;
                    ef.header().type = EthernetHeader::TYPE_IPv4;
                    ef.payload() = beg->second.serialize();
                    _frames_out.push(ef);
                }
                _ipdatagram_out.erase(arpmg.sender_ip_address);
            } else if(arpmg.opcode == ARPMessage::OPCODE_REQUEST && arpmg.target_ip_address == _ip_address.ipv4_numeric()) {
                EthernetFrame ef;
                ef.header().src = _ethernet_address;
                ef.header().dst = arpmg.sender_ethernet_address;
                ef.header().type = EthernetHeader::TYPE_ARP;

                ARPMessage rp_arpmg;
                rp_arpmg.opcode = ARPMessage::OPCODE_REPLY;
                rp_arpmg.sender_ethernet_address = _ethernet_address;
                rp_arpmg.sender_ip_address = _ip_address.ipv4_numeric();
                rp_arpmg.target_ethernet_address = arpmg.sender_ethernet_address;
                rp_arpmg.target_ip_address = arpmg.sender_ip_address;

                ef.payload() = rp_arpmg.serialize();
                _frames_out.push(ef);
            }
        }
    }

    return nullopt;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick(const size_t ms_since_last_tick) { //DUMMY_CODE(ms_since_last_tick); 
    vector<uint32_t> time_out_map; // 记录超过30s的ip,要删除
    for(auto& it: _ethernet_map) {
        if(it.second.has_mapped) {
            it.second._time_to_live += ms_since_last_tick;
            if(it.second._time_to_live >= 30000) time_out_map.push_back(it.first);
        } else {
            it.second._time_wait_to_respond += ms_since_last_tick;
            if(it.second._time_wait_to_respond >= 5000) ARP_request(it.first);
        }
    }
    for(const auto& it: time_out_map) {
        _ethernet_map.erase(it);
    }
}
