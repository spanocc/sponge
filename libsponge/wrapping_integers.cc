#include "wrapping_integers.hh"

// Dummy implementation of a 32-bit wrapping integer

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
//! \param n The input absolute 64-bit sequence number
//! \param isn The initial sequence number
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {
    // DUMMY_CODE(n, isn);
    // return WrappingInt32{0};
    return WrappingInt32((static_cast<uint64_t>(isn.raw_value()) + (n % (1ul << 32))) % (1ul << 32));
}

//! Transform a WrappingInt32 into an "absolute" 64-bit sequence number (zero-indexed)
//! \param n The relative sequence number
//! \param isn The initial sequence number
//! \param checkpoint A recent absolute 64-bit sequence number
//! \returns the 64-bit sequence number that wraps to `n` and is closest to `checkpoint`
//!
//! \note Each of the two streams of the TCP connection has its own ISN. One stream
//! runs from the local TCPSender to the remote TCPReceiver and has one ISN,
//! and the other stream runs from the remote TCPSender to the local TCPReceiver and
//! has a different ISN.
uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {   
    // DUMMY_CODE(n, isn, checkpoint);
    // return {};
    // 思路：有2^32个长度为2^32的线段，看n实际上在哪个线段上,在checkpoint所在线段以及他之前和之后的两个线段里寻找

    // 为什么是前后两个线段？
    // 如果只找前后一个线段，实际上找到的是（n + absolute seqno）的值最靠近checkpoint的绝对序列号，而我们想要找的是（absolute seqno）的值最靠近
    // checkpoint的绝对系列号，他们之间相差一个最大长度不超过mod的n

    /*uint64_t mod = 1ul << 32;
    uint64_t seqst = (checkpoint / mod) * mod; // 所在线段的起始位置
    uint64_t ret = seqst + n.raw_value();

    uint32_t difn = n.raw_value() - isn.raw_value();

    if(ret < isn.raw_value()) { // 确保n在isn之后
        if(seqst >= (1ul << 63)) throw runtime_error("unwrap: seq overflow");
        ret += mod;
    }
    ret -= isn.raw_value();

    // 在所在线段的前两个线段里找
    if(seqst > 0) {
        uint64_t ret1 = seqst - mod + n.raw_value();
        if(ret1 < isn.raw_value()) ret1 += mod;  // 确保n在isn之后
        ret1 -= isn.raw_value();
        if((max(checkpoint, ret1) - min(checkpoint, ret1)) < (max(checkpoint, ret) - min(checkpoint, ret))) ret = ret1;

    }
    if(seqst > mod) {
        uint64_t ret1 = seqst - mod  - mod + n.raw_value();
        if(ret1 < isn.raw_value()) ret1 += mod;
        ret1 -= isn.raw_value();
        if((max(checkpoint, ret1) - min(checkpoint, ret1)) < (max(checkpoint, ret) - min(checkpoint, ret))) ret = ret1;

    }

    // 在所在线段的后两个线段里找
    if(seqst < ((UINT64_MAX - mod) + 1)) {
        uint64_t ret2 = seqst + mod + n.raw_value();
        ret2 -= isn.raw_value();
        if((max(checkpoint, ret2) - min(checkpoint, ret2)) < (max(checkpoint, ret) - min(checkpoint, ret))) ret = ret2;
    }
    if(seqst < ((UINT64_MAX - mod - mod) + 1)) {
        uint64_t ret2 = seqst + mod + mod + n.raw_value();
        ret2 -= isn.raw_value();
        if((max(checkpoint, ret2) - min(checkpoint, ret2)) < (max(checkpoint, ret) - min(checkpoint, ret))) ret = ret2;
    }

    return ret;*/

    // 上面那个麻烦了，直接找出isn到n需要走几步，算出absolute seqno，然后在checkpoint周围的三个线段里找哪个离checkpoint最近
    uint64_t mod = 1ul << 32;
    uint64_t seqst = checkpoint & 0xffffffff00000000; // 所在线段的起始位置
    uint32_t difn = n.raw_value() - isn.raw_value();

    uint64_t ret = difn + seqst;
     if(seqst > 0) {
        uint64_t ret1 = seqst - mod + difn;
        if(ret1 < isn.raw_value()) ret1 += mod;  // 确保absolute seqno在isn之后
        if((max(checkpoint, ret1) - min(checkpoint, ret1)) < (max(checkpoint, ret) - min(checkpoint, ret))) ret = ret1;

    }
    if(seqst < ((UINT64_MAX - mod) + 1)) {
        uint64_t ret2 = seqst + mod + difn;
        if((max(checkpoint, ret2) - min(checkpoint, ret2)) < (max(checkpoint, ret) - min(checkpoint, ret))) ret = ret2;
    }
    return ret;
}
