#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

bool TCPReceiver::segment_received(const TCPSegment &seg) {
    // DUMMY_CODE(seg);
    // return {};

    bool not_start = (!_start);
    if (not_start) {
        if (seg.header().syn) {
            _isn = seg.header().seqno;
            _start = true;
            if (seg.header().fin) {
                _end = true;
            }
        } else if (seg.header().fin) {
            // _isn = WrappingInt32(0);
            // _start = false;
            // _end = true;
            return true;
        } else {
            return false;
        }

        _reassembler.push_substring(seg.payload().copy(), 0u, seg.header().fin);

        return true;
    } else {
        uint64_t checkpoint = _reassembler.stream_out().bytes_written();
        uint64_t index = unwrap(seg.header().seqno, _isn, checkpoint);
        if (!seg.header().syn) {
            index = index - 1u;
        }

        uint64_t left_bound, right_bound;
        uint64_t left_seg, right_seg;
        uint64_t win_size = window_size();
        uint64_t length = seg.length_in_sequence_space();
        win_size = win_size != 0u ? win_size : 1u;
        length = length != 0u ? length : 1u;

        left_bound = unwrap(ackno().value(), _isn, checkpoint);
        right_bound = left_bound + win_size;
        left_seg = unwrap(seg.header().seqno, _isn, checkpoint);
        right_seg = left_seg + length;

        bool inbound = (left_bound < right_seg && left_seg < right_bound);
        _reassembler.push_substring(seg.payload().copy(), index, seg.header().fin);

        if (!_end && seg.header().fin) {
            _end = true;
        }
        return inbound;

        // return true;
    }

    /*
    if (!start && seg.header().syn) {
        start = true;
    isn = seg.header().seqno;
    }
    if (!start && !seg.header().fin) {
        return false;
    }

    uint64_t index = 0u, checkpoint = 0u;
    if (start) {
        checkpoint = 1u + _reassembler.stream_out().bytes_written();
        index = unwrap(seg.header().seqno, isn, checkpoint);
        if (!seg.header().syn) {
            index = index - 1u;
    }
    }
    _reassembler.push_substring(seg.payload().copy(), index, seg.header().fin);

    if (!start) {
        return true;
    }

    uint64_t left_bound, left, right, right_bound;
    uint64_t win_size = window_size();
    if (win_size == 0u) {
        win_size = 1u;
    }
    uint64_t length = seg.length_in_sequence_space();
    if (length == 0u) {
        length = 1u;
    }

    left_bound = unwrap(ackno().value(), isn, checkpoint);
    left = index + 1u;
    right = left + length;
    right_bound = left_bound + win_size;

    if (left < right_bound && left_bound < right) {
        return true;
    }

    return false;
    */
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (_start) {
        uint64_t n = 1 + _reassembler.stream_out().bytes_written() + _end;
        return wrap(n, _isn);
    } else {
        return nullopt;
    }
}

size_t TCPReceiver::window_size() const { return _reassembler.stream_out().remaining_capacity(); }
