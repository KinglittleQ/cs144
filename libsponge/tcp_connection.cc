#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPConnection::_dump_receiver_info(TCPSegment &seg) {
    if (_receiver.ackno().has_value()) {
	seg.header().ack = true;
        seg.header().ackno = _receiver.ackno().value();
	seg.header().win = _receiver.window_size();
    }
}

void TCPConnection::_move_segment_to_outbound(void) {
    if (_sender.segments_out().empty()) {
        return;
    }
    TCPSegment &seg = _sender.segments_out().front();
    _dump_receiver_info(seg);
    _segments_out.push(seg);
    _sender.segments_out().pop();
}

void TCPConnection::_send_rst_segment(void) {
    if (_sender.segments_out().empty()) {
        _sender.send_empty_segment();
    }
    TCPSegment &seg = _sender.segments_out().front();
    _dump_receiver_info(seg);
    seg.header().rst = true;  // set RST flag
    _segments_out.push(seg);
    _sender.segments_out().pop();
}


size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity(); }

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return _timestamp - _time_last_segment; }

void TCPConnection::segment_received(const TCPSegment &seg) { 
    _time_last_segment = _timestamp;

    bool empty_seg = (seg.length_in_sequence_space() == 0);
    bool ackno_valid = true, payload_valid = true;
    const TCPHeader &hdr = seg.header();

    // RST flag is set, the connection is dead.
    if (hdr.rst) {
        _reset = true;
	_sender.stream_in().set_error();
	_receiver.stream_out().set_error();
	return;
    }

    // LISTEN or SYN_SENT, ignore empty ACK
    if (empty_seg && !_receiver.ackno().has_value() /* && _sender.next_seqno_absolute() == 0 */) {
        return;
    }

    payload_valid = _receiver.segment_received(seg);
    if ( _receiver.stream_out().input_ended() && !_sender.fin_sent() ) {
        _linger_after_streams_finish = false;
    }

    if (hdr.ack) {
        ackno_valid = _sender.ack_received(hdr.ackno, hdr.win);
    } else {
        _sender.fill_window();
    }

    // do not need to send ACK
    if ( empty_seg && ackno_valid && payload_valid )  {
        return;
    }

    if (_sender.segments_out().empty()) {
        _sender.send_empty_segment();
    }
    _move_segment_to_outbound();

}

bool TCPConnection::active() const {
    if ( _reset ) {
        return false;
    }

    // 1. FIN is received from peer and all bytes are assembled
    if ( !_receiver.stream_out().input_ended() ) {
        return true;
    }

    // 2. FIN is sent to peer
    if ( !_sender.fin_sent() ) {
        return true;
    }

    // 3. FIN is acknowledged by peer
    if ( !_sender.fin_acked() ) {
        return true;
    }

    if ( _linger_after_streams_finish ) {
        if ( time_since_last_segment_received() >= 10 * _cfg.rt_timeout ) {
            return false; // connection closed
	}
	else {
            return true;
	}
    } else {
        return false;  // connection closed
    }

}

size_t TCPConnection::write(const string &data) {
    // DUMMY_CODE(data);
    // return {};
    size_t size = _sender.stream_in().write(data);
    _sender.fill_window();
    _move_segment_to_outbound();
    return size;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) { 
    _sender.tick(ms_since_last_tick);
    _move_segment_to_outbound();
    _timestamp += ms_since_last_tick;

    if ( _sender.consecutive_retransmissions() > _cfg.MAX_RETX_ATTEMPTS ) {
        _reset = true;
	_sender.stream_in().set_error();
	_receiver.stream_out().set_error();
        _send_rst_segment();
    }
}

void TCPConnection::end_input_stream() { 
    _sender.stream_in().end_input();
    _sender.fill_window();
    _move_segment_to_outbound();
}

void TCPConnection::connect() {
    // send SYN with ACK
    _sender.fill_window();
    _move_segment_to_outbound();
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            // Your code here: need to send a RST segment to the peer
	    _send_rst_segment();
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
