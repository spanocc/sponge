#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    // DUMMY_CODE(seg);

    if(seg.header().syn) {
        //if(state != EMPTY) throw runtime_error("TCPReceiver: repeated syn");
        isn = seg.header().seqno;
        state = SYN_RECV;
    }

    if(state == LISTEN) return;

    bool eof = seg.header().fin;
    uint64_t checkpoint = 0;
    if(_reassembler.next_index()) checkpoint = _reassembler.next_index() - 1;
    size_t index = unwrap(seg.header().seqno, isn, checkpoint);
    if(index == 0 && !seg.header().syn) return; // index == 0 的唯一可能是syn段
    if(index > 0) index -= 1;                                        //不管第一个序号是不是syn都行
    //if(seg.payload().str().size()) {
    // 带fin的TCP段可能是无有效载荷的，但是也要给reassembler一个eof信息，就push一个空的字符串进去
    //if(seg.header().syn) index = unwrap(seg.header().seqno + 1, isn, checkpoint);
    //else index = unwrap(seg.header().seqno, isn, checkpoint);
    _reassembler.push_substring(seg.payload().copy(), index, eof);
    //}

    if(eof) state = FIN_RECV;

   
}

optional<WrappingInt32> TCPReceiver::ackno() const { 
    if(state == LISTEN) return nullopt;

    uint64_t absolute_seqno = _reassembler.next_index();
    //if(state == EXIST) absolute_seqno += 1;
    if(state == FIN_RECV && _reassembler.unassembled_bytes() == 0) absolute_seqno += 2;
    else if(state == SYN_RECV || state == FIN_RECV) absolute_seqno += 1;
    return wrap(absolute_seqno, isn);

}

size_t TCPReceiver::window_size() const {   // 不用间unressemble_bytes,因为这段数据在ackno之后，也算在window里
    return _capacity - (_reassembler.stream_out().buffer_size()); 
}
