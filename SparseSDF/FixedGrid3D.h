#pragma once
// a fixed NxNxN grid.
template <unsigned TN>
struct FixedGrid3D {
  static const unsigned N = TN;
  static const unsigned LEN = N * N * N;
  // extra bytes for padding
  short val[LEN];

  short operator()(unsigned x, unsigned y, unsigned z) const { return val[x + y * N + z * N * N]; }
  short& operator()(unsigned x, unsigned y, unsigned z) { return val[x + y * N + z * N * N]; }

  void Fill(short v) {
    for (unsigned i = 0; i < LEN; i++) {
      val[i] = v;
    }
  }
};