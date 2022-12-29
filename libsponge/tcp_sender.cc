#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity) , _rt(retx_timeout), _win(1), _prev_seqno(0), _bytes_in_flight(0), state(CLOSE), _win_zero_used(0) {}

uint64_t TCPSender::bytes_in_flight() const { return _bytes_in_flight; }

void TCPSender::retransmission() { // cout<<"eeeee";
    if(_rt._outstanding_seg.empty()) return; 
    _segments_out.push(_rt._outstanding_seg.begin()->second);   // cout<<"rrrrr";
}

void TCPSender::fill_window() {   

    bool restart_timer = false; //是否需要重启计时器

    size_t wz;
    // _bytes_in_flight是发送但还没确认的，也就是旧的窗口大小
    if(_win >= _bytes_in_flight) wz = _win - _bytes_in_flight;    
    else wz = 0;  // 如果新窗口大小比旧窗口大小小，那么就没有可发的数据
                                                
                                                             
    if(_win == 0 && !_win_zero_used) {  // _win == 0 只传一次(每收到一次ACK就可以传一次) , 也会进入outstanding集合中被重传
        wz = 1;
        _win_zero_used = 1;
    }
    else if(_win == 0 && _win_zero_used) return;
    while(wz) {                  

        // if(state == FIN_SENT || state == FIN_ACKED) break; //以及发过FIN段了，不需要再发了

        size_t sz = _stream.buffer_size();

        // 段的序列号总大小不超过 TCPConfig::MAX_PAYLOAD_SIZE      no
        // wz = min(wz, TCPConfig::MAX_PAYLOAD_SIZE);
        // 段的有效载荷部分不超过 TCPConfig::MAX_PAYLOAD_SIZE      yes
        sz = min(sz, TCPConfig::MAX_PAYLOAD_SIZE);
       
        TCPSegment tcpseg;

        if(state == CLOSE && wz > 0) {
            tcpseg.header().syn = true;
            state = SYN_SENT;
            wz -= 1;
        }

        sz = min(sz, wz);
        tcpseg.payload() = _stream.read(sz);
        wz -= sz;

        // FIN 要在数据全部发送完但不必全部确认完之后再发送
        if(state == SYN_ACKED && _stream.eof() && wz > 0) {
            tcpseg.header().fin = true;
            state = FIN_SENT;
            wz -= 1;
        }

        tcpseg.header().seqno = wrap(_next_seqno, _isn);
        // 没有要发送的段，就直接退出
        if(tcpseg.length_in_sequence_space() == 0) break; 

        restart_timer = true; // 有sequnence space length非0的段要发送，重启计时器
         _rt._outstanding_seg.insert({_next_seqno, tcpseg});
         _next_seqno += tcpseg.length_in_sequence_space();
         _bytes_in_flight += tcpseg.length_in_sequence_space();


        _segments_out.push(tcpseg);

    }

    // 如果计时器是关闭状态，则开启计时器
    if(_rt._timer_switch == false && restart_timer) {
        _rt._timer_switch = true;
        _rt._has_passed = 0;
    }

    //cout<<"222: "<<_bytes_in_flight<<endl;
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) { //DUMMY_CODE(ackno, window_size); 
    
    _win = window_size;

    uint64_t nseqno = unwrap(ackno, _isn, _prev_seqno);
    if(nseqno > _prev_seqno) {

        if(nseqno > _next_seqno) return; // Impossible ackno (beyond next seqno) is ignored

        auto it = _rt._outstanding_seg.lower_bound(nseqno);
        if(it == _rt._outstanding_seg.begin()) return; // 有前面的if,应该不会出现收到的ackno的绝对序列号比所有的都小
        it--;
        if(it->first + it->second.length_in_sequence_space() <= nseqno) it++; // 只需要判断前一个段的整体是不是都在nseqno前面即可，防止出现nseqno把一个段给分割的情况
        if(it == _rt._outstanding_seg.begin()) return; // 再判断一次
        // if(it->first != nseqno) return; 
        _rt._outstanding_seg.erase(_rt._outstanding_seg.begin(), it); 
        _prev_seqno = nseqno;

        _bytes_in_flight = _next_seqno - nseqno;    //cout<<"111: "<<_bytes_in_flight<<endl;
        
        // 如果在SYN_SENT状态下收到ACK，一定会转为SYN_ACKED状态
        if(state == SYN_SENT) state = SYN_ACKED;
        // 要在fill_window发送之前判断
        // if(_stream.eof() && _next_seqno < _stream.bytes_written() + 2) state = SYN_ACKED;
        if(_stream.eof() && _next_seqno == _stream.bytes_written() + 2) {
            state = FIN_ACKED;
            // 所有字节确认完毕后关闭计数器
            _rt._timer_switch = false;
            _rt._has_passed = 0;
        }

        _win_zero_used = 0; // 每收到一次ACK,就可以发送一个win=0的报文段
        fill_window();                               //cout<<"333: "<<_bytes_in_flight<<endl;


        _rt._rto = _initial_retransmission_timeout;
        if(!_rt._outstanding_seg.empty()) {
            _rt._timer_switch = true;
            _rt._has_passed = 0;
        }
        _rt._nrtsm = 0;

    }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) { //DUMMY_CODE(ms_since_last_tick);
    _rt._has_passed += ms_since_last_tick;                 //cout<<"qqqqq"<<_rt._rto<<" "<<_rt._has_passed<<" ";
    if(_rt._timer_switch && _rt._has_passed >= _rt._rto) {       //cout<<"wwwww";
    // 必须当有数据可以重传，才会重传并扩大rto
        if(_win && !_rt._outstanding_seg.empty()) {
            _rt._nrtsm++;
            _rt._rto *= 2;
        }
        retransmission();
        _rt._has_passed = 0;
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return _rt._nrtsm; }

void TCPSender::send_empty_segment() {
    TCPSegment tcpseg;
    tcpseg.header().seqno = wrap(_next_seqno, _isn);
    tcpseg.payload() = std::string();
    _segments_out.push(tcpseg);
}
