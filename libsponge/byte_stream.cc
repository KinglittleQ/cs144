#include "byte_stream.hh"

#include <algorithm>
#include <iterator>
#include <stdexcept>

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity) {
    _capacity = capacity;
    if (capacity == 0) {
        _error = true;
    }
}

size_t ByteStream::write(const string &data) {
    size_t i = 0;
    if (!input_ended()) {
        for (i = 0; i < data.size() && _stream.size() < _capacity; i++) {
            _stream.push_back(data[i]);
        }
    }
    _bytes_written += i;
    return i;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    string bytes;
    size_t stream_size = _stream.size();
    for (size_t i = 0; i < len && i < stream_size; i++) {
        bytes.push_back(_stream[i]);
    }
    return bytes;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    size_t stream_size = _stream.size();
    size_t i = 0;
    for (i = 0; i < len && i < stream_size; i++) {
        _stream.pop_front();
    }
    _bytes_read += i;
}

void ByteStream::end_input() { _end_input = true; }

bool ByteStream::input_ended() const { return _end_input; }

size_t ByteStream::buffer_size() const { return _stream.size(); }

bool ByteStream::buffer_empty() const { return _stream.empty(); }

bool ByteStream::eof() const { return input_ended() && buffer_empty(); }

size_t ByteStream::bytes_written() const { return _bytes_written; }

size_t ByteStream::bytes_read() const { return _bytes_read; }

size_t ByteStream::remaining_capacity() const { return _capacity - buffer_size(); }
