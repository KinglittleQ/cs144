#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <iostream>
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
    , _stream(capacity)
    , _timer(retx_timeout) {}

uint64_t TCPSender::bytes_in_flight() const { return _bytes_sent - _bytes_received; }

void TCPSender::fill_window() {
    TCPSegment seg;

    if (_eof) {
        return;
    }

    // if (_stream.eof()) {
    //    seg.header().fin = true;
    //}
    if (_bytes_sent == 0) {
        seg.header().syn = true;
    }

    // size_t syn = seg.header().syn ? 1 : 0;
    // size_t fin =

    size_t label_size = seg.length_in_sequence_space();
    uint16_t window_size = _window_size > 0u ? _window_size : 1u;
    if (bytes_in_flight() + label_size > window_size || bytes_in_flight() >= window_size) {
        return;
    }
    size_t payload_length = window_size - bytes_in_flight() - label_size;
    if (payload_length > _stream.buffer_size()) {
        payload_length = _stream.buffer_size();
    }
    if (payload_length > TCPConfig::MAX_PAYLOAD_SIZE) {
        payload_length = TCPConfig::MAX_PAYLOAD_SIZE;
    }

    if (label_size + payload_length == 0u && !_stream.eof()) {
        return;
    }

    seg.payload() = Buffer(_stream.read(payload_length));
    seg.header().seqno = next_seqno();

    if (window_size > bytes_in_flight() + seg.length_in_sequence_space()) {
        _eof = _stream.eof();
        seg.header().fin = _stream.eof();
    }

    _segments_out.push(seg);
    _segments_unack.push(seg);
    _bytes_sent += seg.length_in_sequence_space();

    if (!_timer.running()) {
        _timer.reset_rto(_initial_retransmission_timeout);
        _timer.start(_timestamp);
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
//! \returns `false` if the ackno appears invalid (acknowledges something the TCPSender hasn't sent yet)
bool TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    // DUMMY_CODE(ackno, window_size);
    // return {};

    uint64_t abs_ackno = unwrap(ackno, _isn, _bytes_received);
    if (abs_ackno > _bytes_sent) {
        return false;
    }
    if (abs_ackno < _bytes_received) {
        return true;
    }

    _window_size = window_size;
    if (abs_ackno == _bytes_received) {
        fill_window();
        return true;
    }

    // new data been ack
    _timer.reset_rto(_initial_retransmission_timeout);
    //_timer.start(_timestamp);
    _consecutive_retrans = 0;
    _bytes_received = abs_ackno;

    while (!_segments_unack.empty()) {
        TCPSegment &seg = _segments_unack.front();
        uint64_t abs_seqno = unwrap(seg.header().seqno, _isn, _bytes_received);
        if (abs_seqno + seg.length_in_sequence_space() <= abs_ackno) {
            _segments_unack.pop();
        } else {
            break;
        }
    }

    if (!_segments_unack.empty()) {
        _timer.start(_timestamp);
    }
    // _consecutive_retrans = 0;
    fill_window();

    return true;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    // DUMMY_CODE(ms_since_last_tick);
    _timestamp += ms_since_last_tick;

    if (!_timer.running()) {
        return;
    }

    _timer.update_time(_timestamp);
    if (_timer.timeout()) {
        _segments_out.push(_segments_unack.front());
        _consecutive_retrans += 1;
        _timer.double_rto();
        _timer.start(_timestamp);
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retrans; }

void TCPSender::send_empty_segment() {
    TCPSegment seg;
    seg.header().seqno = next_seqno();
    _segments_out.push(seg);
}

/*
WrappingInt32 TCPSender::_next_seqno(void) const {
    return wrap(_bytes_sent, _isn);
}*/

WrappingInt32 TCPSender::_ackno(void) const { return wrap(_bytes_received, _isn); }
