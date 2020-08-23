#ifndef SPONGE_LIBSPONGE_TCP_SENDER_HH
#define SPONGE_LIBSPONGE_TCP_SENDER_HH

#include "byte_stream.hh"
#include "tcp_config.hh"
#include "tcp_segment.hh"
#include "wrapping_integers.hh"

#include <functional>
#include <queue>

//! \brief The "sender" part of a TCP implementation.

//! Timer for retransmission
class Timer {
  private:
    bool _running{false};

    size_t _start_time{0};

    size_t _now_time{0};

    unsigned int _rto{0};

  public:
    Timer(unsigned int rto) : _rto(rto) {}

    void update_time(size_t timestamp) { _now_time = timestamp; }

    bool timeout(void) const { return (running() && (_now_time - _start_time >= _rto)); }

    void double_rto(void) { _rto *= 2u; }

    void start(size_t timestamp) {
        _start_time = _now_time = timestamp;
        _running = true;
    }

    bool running(void) const { return _running; }

    void reset_rto(unsigned int rto) {
        _rto = rto;
        _running = false;
    }
};

//! Accepts a ByteStream, divides it up into segments and sends the
//! segments, keeps track of which segments are still in-flight,
//! maintains the Retransmission Timer, and retransmits in-flight
//! segments if the retransmission timer expires.
class TCPSender {
  private:
    //! our initial sequence number, the number for our SYN.
    WrappingInt32 _isn;

    //! outbound queue of segments that the TCPSender wants sent
    std::queue<TCPSegment> _segments_out{};

    //! retransmission timer for the connection
    unsigned int _initial_retransmission_timeout;

    //! outgoing stream of bytes that have not yet been sent
    ByteStream _stream;

    //! the (absolute) sequence number for the next byte to be sent
    // uint64_t _next_seqno{0};

    //! the retansmission timer
    Timer _timer;

    //! consecutive retransmissions
    unsigned int _consecutive_retrans{0};

    //! unacknowlege segments
    std::queue<TCPSegment> _segments_unack{};

    //! timestamp
    size_t _timestamp{0};

    //! bytes sent (absolute sequence number of the next byte)
    uint64_t _bytes_sent{0};

    //! bytes already received by TCPReceiver
    uint64_t _bytes_received{0};

    //! windows size
    uint16_t _window_size{1};

    //! eof
    bool _eof{false};

    //! Get next relative sequence number
    // WrappingInt32 _next_seqno(void) const;

    //! Get ackno
    WrappingInt32 _ackno(void) const;

  public:
    //! Initialize a TCPSender
    TCPSender(const size_t capacity = TCPConfig::DEFAULT_CAPACITY,
              const uint16_t retx_timeout = TCPConfig::TIMEOUT_DFLT,
              const std::optional<WrappingInt32> fixed_isn = {});

    //! \name "Input" interface for the writer
    //!@{
    ByteStream &stream_in() { return _stream; }
    const ByteStream &stream_in() const { return _stream; }
    //!@}

    //! \name Methods that can cause the TCPSender to send a segment
    //!@{

    //! \brief A new acknowledgment was received
    bool ack_received(const WrappingInt32 ackno, const uint16_t window_size);

    //! \brief Generate an empty-payload segment (useful for creating empty ACK segments)
    void send_empty_segment();

    //! \brief create and send segments to fill as much of the window as possible
    void fill_window();

    //! \brief Notifies the TCPSender of the passage of time
    void tick(const size_t ms_since_last_tick);
    //!@}

    //! \name Accessors
    //!@{

    //! \brief How many sequence numbers are occupied by segments sent but not yet acknowledged?
    //! \note count is in "sequence space," i.e. SYN and FIN each count for one byte
    //! (see TCPSegment::length_in_sequence_space())
    size_t bytes_in_flight() const;

    //! \brief Number of consecutive retransmissions that have occurred in a row
    unsigned int consecutive_retransmissions() const;

    //! \brief TCPSegments that the TCPSender has enqueued for transmission.
    //! \note These must be dequeued and sent by the TCPConnection,
    //! which will need to fill in the fields that are set by the TCPReceiver
    //! (ackno and window size) before sending.
    std::queue<TCPSegment> &segments_out() { return _segments_out; }
    //!@}

    //! \name What is the next sequence number? (used for testing)
    //!@{

    //! \brief absolute seqno for the next byte to be sent
    uint64_t next_seqno_absolute() const { return _bytes_sent; }

    //! \brief relative seqno for the next byte to be sent
    WrappingInt32 next_seqno() const { return wrap(_bytes_sent, _isn); }
    //!@}
};

#endif  // SPONGE_LIBSPONGE_TCP_SENDER_HH
