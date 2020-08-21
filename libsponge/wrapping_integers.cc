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
    n = n % (1ULL << 32);
    n = (n + isn.raw_value()) % (1ULL << 32);
    return WrappingInt32{static_cast<uint32_t>(n)};
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
    WrappingInt32 ckpt = wrap(checkpoint, isn);
    int64_t diff;
    diff = static_cast<int64_t>(n.raw_value()) - static_cast<int64_t>(ckpt.raw_value());

    int64_t results[3];
    results[1] = checkpoint + diff;
    results[0] = results[1] - (1LL << 32);
    results[2] = results[1] + (1LL << 32);

    int64_t min_diff = 1LL << 33;
    int min_idx = 0;
    for (int i = 0; i < 3; i++) {
        if (results[i] >= 0 && std::abs(results[i] - static_cast<int64_t>(checkpoint)) < min_diff) {
            min_diff = std::abs(results[i] - static_cast<int64_t>(checkpoint));
            min_idx = i;
        }
    }

    return results[min_idx];
}
