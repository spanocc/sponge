#include "byte_stream.hh"

// 调试
#include<iostream>
// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

// 这只是一个样例
template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

// 要按类中成员变量的顺序初始化，且string的也要初始化，不然会报错
ByteStream::ByteStream(const size_t capacity) : max_capacity(capacity), used_capacity(0), total_read(0), total_write(0),
                                                byte_buffer(), _end_input(0) { /*DUMMY_CODE(capacity);*/}

size_t ByteStream::write(const string &data) {
    //DUMMY_CODE(data);
    if(_end_input) return 0;
    size_t len = min(remaining_capacity(), data.size());
    byte_buffer += data.substr(0, len);
    used_capacity += len;   
    total_write += len;
    return len;
}

// 调用peek 和 pop 两个函数保证len不小于buffer_size
//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    //DUMMY_CODE(len);
    size_t newlen = min(len, buffer_size());
    return byte_buffer.substr(0, newlen);
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) { 
    /*DUMMY_CODE(len);*/ 
    size_t newlen = min(len, buffer_size());
    byte_buffer.erase(0, newlen); 
    total_read += newlen;          
    used_capacity -= newlen;          
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    //DUMMY_CODE(len);
    size_t newlen = min(len, buffer_size());
    string ret = peek_output(newlen);
    pop_output(newlen);
    return ret;
}

void ByteStream::end_input() { _end_input = 1;}

bool ByteStream::input_ended() const { return _end_input; }

size_t ByteStream::buffer_size() const { return used_capacity; }

bool ByteStream::buffer_empty() const { return used_capacity == 0; }

bool ByteStream::eof() const { return _end_input && buffer_empty(); }

size_t ByteStream::bytes_written() const { return total_write; }

size_t ByteStream::bytes_read() const { return total_read; }

size_t ByteStream::remaining_capacity() const { return max_capacity - used_capacity; }
