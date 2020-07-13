#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

constexpr int EMPTY_CHAR = -1000;

StreamReassembler::StreamReassembler(const size_t capacity) : 
    _output(capacity), _capacity(capacity), _buffer(capacity, EMPTY_CHAR) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    // DUMMY_CODE(data, index, eof);

    if (_eof_index != std::numeric_limits<size_t>::max() 
            && index + data.size() > _eof_index) {
        return;
    }

    for (size_t i = 0; i < data.size(); i++) {
        if (index + i >= _start_index && index + i < _start_index + _capacity) {
            if (_buffer[index + i - _start_index] == EMPTY_CHAR) {
                _buffer_size++;
	    }
            _buffer[index + i - _start_index] = static_cast<int>(data[i]);
	}
    }

    if (eof) {
        _eof_index = index + data.size();
    }

    std::string pop_str;
    size_t i;
    bool written = false;
    for (i = 0; i < _buffer.size(); i++) {
        if (_buffer[i] != EMPTY_CHAR) {
            pop_str += static_cast<char>(_buffer[i]);
	}
	else {
            break;
	}
    }

    if (!written)
        _output.write(pop_str);

    if (i + _start_index >= _eof_index) {
        _output.end_input();
    }


    size_t num_bytes_popped = i;
    for (i = 0; i < num_bytes_popped; i++) {
        _buffer.pop_front();
	_buffer.push_back(EMPTY_CHAR);
	_start_index++;
	_buffer_size--;
    }
}

size_t StreamReassembler::unassembled_bytes() const { return _buffer_size; }

bool StreamReassembler::empty() const { return unassembled_bytes() == 0; }
