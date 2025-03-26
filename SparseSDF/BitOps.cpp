#include "BitOps.h"

#include <intrin.h>

/// Return the number of on bits in the given 64-bit value.
unsigned CountOn(unsigned long long v) {
  v = __popcnt64(v);
  // Software Implementation
  // v = v - ((v >> 1) & UINT64_C(0x5555555555555555));
  // v = (v & UINT64_C(0x3333333333333333)) + ((v >> 2) &
  // UINT64_C(0x3333333333333333)); v = (((v + (v >> 4)) &
  // UINT64_C(0xF0F0F0F0F0F0F0F)) * UINT64_C(0x101010101010101)) >> 56;
  return static_cast<unsigned>(v);
}
