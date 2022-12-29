#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity(); }

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return _time_since_last_segment_received; }


void TCPConnection::send_segment() {

    while(!_sender.segments_out().empty()) {    // cout<<"jjjj";
        TCPSegment &tcpseg = _sender.segments_out().front();
        if(_receiver.ackno().has_value()) {
            tcpseg.header().ack = true;
            tcpseg.header().ackno = _receiver.ackno().value();
        }
        if(_receiver.window_size() > numeric_limits<uint16_t>::max()) 
            tcpseg.header().win = numeric_limits<uint16_t>::max();
        else tcpseg.header().win = static_cast<uint16_t>(_receiver.window_size());
        _segments_out.push(tcpseg);
        _sender.segments_out().pop();
    }
}

void TCPConnection::send_reset_segment() {   // cout<<"hhhh";
    _sender.send_empty_segment();
    _sender.segments_out().back().header().rst = true;
    send_segment();

    _sender.stream_in().set_error();
    _receiver.stream_out().set_error();
    _active = false;
}

void TCPConnection::segment_received(const TCPSegment &seg) { //DUMMY_CODE(seg); 
    // 设置了RESET标志
    if(seg.header().rst) {
        _sender.stream_in().set_error();
        _receiver.stream_out().set_error();
        _active = false;
        return;
    }
    // keep-alive
    if(_receiver.ackno().has_value() && (seg.length_in_sequence_space() == 0) && seg.header().seqno == _receiver.ackno().value() - 1) {
        _sender.send_empty_segment();
    }
    // 将段给tcpreceiver
    _receiver.segment_received(seg);

    // if(_receiver.ackno().has_value() && !_receiver.stream_out().input_ended()) _active = true;

    if(_receiver.stream_out().input_ended() && !_sender.stream_in().eof()) _linger_after_streams_finish = false;

    if(_receiver.stream_out().input_ended() && _sender.stream_in().eof() 
        && _sender.next_seqno_absolute() == _sender.stream_in().bytes_written() + 2 && bytes_in_flight() == 0 
        && _linger_after_streams_finish == false)
        _active = false;

    _time_since_last_segment_received = 0;

    // 设置了ack标志
    if(seg.header().ack) {
        _sender.ack_received(seg.header().ackno, seg.header().win);
    }
    else _sender.fill_window(); // 如果没设置ack标志，说明对方发的是是初始段，那我们也应该发一个初始段，通过fill_window 

    // 如果传入的段占序列号，至少发送一个段应答
    if(seg.length_in_sequence_space() > 0 && _sender.segments_out().empty()) {
        _sender.send_empty_segment(); // 发一个带ackno和win的空段
    }

    send_segment();

}

bool TCPConnection::active() const { return _active; }

size_t TCPConnection::write(const string &data) {
    // DUMMY_CODE(data);
    size_t ret = 0;
    ret = _sender.stream_in().write(data);
    _sender.fill_window();
    send_segment();
    return ret;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) { // DUMMY_CODE(ms_since_last_tick); 
    _sender.tick(ms_since_last_tick);
    send_segment();
    _time_since_last_segment_received += ms_since_last_tick;

    if(_receiver.stream_out().input_ended() && _sender.stream_in().eof() 
        && _sender.next_seqno_absolute() == _sender.stream_in().bytes_written() + 2 && bytes_in_flight() == 0 
        && _linger_after_streams_finish == false)
        _active = false;

    if(_receiver.stream_out().input_ended() && _sender.stream_in().eof() 
        && _sender.next_seqno_absolute() == _sender.stream_in().bytes_written() + 2 && bytes_in_flight() == 0
        && _time_since_last_segment_received >= 10 * _cfg.rt_timeout)
        _active = false;

    if(_sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS) {
        send_reset_segment();
    }
}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    _sender.fill_window();
    send_segment();
}

void TCPConnection::connect() {
    // 初始时，win == 1 ，fill_window就可以发送一个syn段
    if(_sender.next_seqno_absolute() == 0) {
        _sender.fill_window();
        send_segment();
    }
    else std::cerr<<"Should not be here! TCPConnection: connect has been builded!\n";
    // _active = true;
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            // Your code here: need to send a RST segment to the peer
            send_reset_segment();
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
