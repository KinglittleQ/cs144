#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

bool TCPReceiver::segment_received(const TCPSegment &seg) {
    const TCPHeader &hdr = seg.header();

    // bool not_start = (!_start);
    if (!_start) {
        if (hdr.syn) {
            _isn = hdr.seqno;
            _start = true;
            _end = hdr.fin;
        } else if (hdr.fin) {
            return true;
        } else {
            return false;
        }

        _reassembler.push_substring(seg.payload().copy(), 0, hdr.fin);

        return true;
    } else {
        uint64_t checkpoint = _reassembler.stream_out().bytes_written();
        uint64_t index = unwrap(hdr.seqno, _isn, checkpoint);
        if (!hdr.syn) {
            index = index - 1;
        }

        uint64_t left_bound, right_bound;
        uint64_t left_seg, right_seg;
        uint64_t win_size = window_size();
        uint64_t length = seg.payload().size();
        win_size = (win_size == 0 ? 1 : win_size);
        length = (length == 0 ? 1 : length);

        left_bound = unwrap(ackno().value(), _isn, checkpoint);

        right_bound = left_bound + win_size;
        left_seg = unwrap(hdr.seqno, _isn, checkpoint);
        right_seg = left_seg + length;

        bool inbound = (left_bound < right_seg && left_seg < right_bound);
        _reassembler.push_substring(seg.payload().copy(), index, hdr.fin);

        if (!_end && hdr.fin) {
            _end = true;
        }
        return inbound;
    }
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (_start) {
        // fin is received but may not be assembled
        uint64_t end = static_cast<uint64_t>(_end && _reassembler.unassembled_bytes() == 0);
        uint64_t n = 1 + _reassembler.stream_out().bytes_written() + end;
        return wrap(n, _isn);
    } else {
        return nullopt;
    }
}

size_t TCPReceiver::window_size() const { return _reassembler.stream_out().remaining_capacity(); }
