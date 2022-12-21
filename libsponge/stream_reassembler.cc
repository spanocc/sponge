#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity), unassembled_str(), 
                                                            unassembled_num(0), nindex(0), _end_input(0)  {}


void StreamReassembler::reassemble_substr() {
    auto iter = unassembled_str.begin();
    for(; iter != unassembled_str.end(); ++iter) {  
        if(nindex != iter->first) break;                 
        _output.write(iter->second);
        size_t len = iter->second.size();
        unassembled_num -= len;
        nindex += len;                        //cout<<"####\n"<<len<<"####\n";
    }
    if(iter != unassembled_str.begin()) {
        unassembled_str.erase(unassembled_str.begin(), iter);
    }
    if(unassembled_num == 0 && _end_input) _output.end_input();

    //if(_end_input) cout<<"####\n"<<unassembled_num<<"\n"<<_output.buffer_size()<<"\n###\n";
}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    //DUMMY_CODE(data, index, eof);         

    //cout<<"!!!\nsize:"<<data.size()<<" index:"<<index<<" "<<nindex<<" eof::"<<eof<<" cap"<<_capacity<<"\n";

    size_t remain_size = data.size(); // 剩余字符串的长度
    // 字符串可能重叠，把data字符串一点点分隔开，再插入到map里
    size_t remain_index = index;    // 剩余字符串的起点

    if(remain_index + remain_size > nindex + _capacity - _output.buffer_size()) {  
        if(remain_index >= nindex + _capacity - _output.buffer_size()) return;
        remain_size -= (remain_index + remain_size - (nindex + _capacity - _output.buffer_size()));  // 对于超出的部分截断
    }
    if(eof && remain_size == data.size()) _end_input = true;  // 必须是没截断过的才能是eof


    vector<pair<size_t, string> > insert_str;      // The substring to insert the map

    if(nindex >= remain_index + remain_size) {   
        if(unassembled_num == 0 && _end_input) _output.end_input(); // 在返回之前要判断一遍，防止这个是eof
        return;
    }
    if(nindex > remain_index) {
        remain_size -= nindex - remain_index;
        remain_index = nindex; 
    }
    auto st = unassembled_str.lower_bound(remain_index);
    if(st != unassembled_str.begin()) st--;
    auto ed = unassembled_str.lower_bound(remain_index + remain_size);
    for(auto it = st; it != ed; ++it) {                                                   
        size_t len = it->second.size(), idx = it->first;
        if(idx <= remain_index && idx + len <= remain_index) continue;
        
        if(idx > remain_index) {
            remain_size -= idx - remain_index;
            /*assert(remain_index >= index && remain_index < index + data.size());
            assert(remain_index + remain_size <= data.size());
            assert(remain_index - index > data.size());*/
            insert_str.push_back( {remain_index, data.substr(remain_index - index, idx - remain_index)} );
            remain_index = idx;
        }

        //     -----
        //    /----/      <--map中的字符串
        //  ------------
        // /-----------/  <--data的剩余字符串
        // 可能出现这种情况的话，把他前一部分分解出来后，不能把 it+1 ， 应该再继续分解后一部分

        if(idx <= remain_index && idx + len > remain_index && idx + len < remain_index + remain_size) {
            remain_size -= (idx + len - remain_index);
            remain_index = idx + len;
        }
        else if(idx <= remain_index && idx + len >= remain_index + remain_size) {
            remain_index += remain_size;
            remain_size = 0;
        }        
    }
    if(remain_size != 0) {  //cout<<"pppppppppppppp\n"<<remain_size<<endl;
        /*assert(remain_index >= index && remain_index < index + data.size());
        assert(remain_index + remain_size <= data.size());
        assert(remain_index - index > data.size());*/
        insert_str.push_back( {remain_index, data.substr(remain_index - index, remain_size)} );
    }

    /*cout<<"$$$$\n";
    for(auto &it: unassembled_str) {
        cout<<"index: "<<it.first<<" size: "<<it.second.size()<<endl;
    }cout<<"$$$$\n";*/
        

    for(const auto &it : insert_str) {  //cout<<it.second.size()<<"# ";
        assert(it.second.size() != 0);
        assert(unassembled_str.count(it.first) == 0);
        unassembled_str.insert(it);
        unassembled_num += it.second.size();
    }

    //cout<<unassembled_num<<" "<<nindex<<" ";

    reassemble_substr();

    //cout<<unassembled_num<<" "<<nindex<<" "<<_output.buffer_size()<<"\n!!!\n";
}

size_t StreamReassembler::unassembled_bytes() const { return unassembled_num; }

bool StreamReassembler::empty() const { return unassembled_num == 0; }
