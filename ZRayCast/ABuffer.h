#pragma once
#include <vector>

#include "CompressedLists.h"
#include "FixedArray.h"
#include "Hit.h"
#include "Vec2.h"

/// max grid size to prevent simple coding errors.
#define MAX_ABUFFER_SIZE 1000000
/// max uint16_t / 0.01 zRes.
/// pretty generous for the printer.
#define MAX_Z_HEIGHT_MM 655

/// <summary>
/// Alph buffer for rendering transparency. It stores all intersections for each pixel.
/// </summary>
/// <typeparam name="VAL_T"></typeparam>
template <typename VAL_T>
struct ABuffer {
  struct Segment {
    Segment() {}
    Segment(VAL_T p0, VAL_T p1) : t0(p0), t1(p1) {}
    // can't be negative
    VAL_T t0 = 0;
    VAL_T t1 = 0;
    // for std lower_bound to find the
    // closest segment after t
    friend bool operator<(const Segment& a, float t) { return a.t1 < t; }
  };

  using SegList = FixedArray<Segment>;
  class ConstSegList {
   public:
    ConstSegList(const Segment* start, unsigned len) : _start(start), _len(len) {}
    unsigned size() const { return _len; }
    const Segment& operator[](unsigned x) { return _start[x]; }
    const Segment& back() const { return _start[_len - 1]; }
    bool empty() const { return _len == 0; }
    typedef const Segment* iterator;
    iterator begin() const { return _start; }
    iterator end() const { return _start + _len; }

   private:
    const Segment* _start;
    unsigned _len = 0;
  };

  std::vector<CompressedLists<Segment>> segs;
  Vec2u size;
  float zRes = 1.0f;

  void resize(size_t x, size_t y) {
    size[0] = unsigned(x);
    size[1] = unsigned(y);
    segs.resize(y);
  }

  CompressedLists<Segment>& GetRow(size_t y) { return segs[y]; }
  const CompressedLists<Segment>& GetRow(size_t y) const { return segs[y]; }

  ConstSegList operator()(unsigned x, unsigned y) const {
    const Segment* start = &(segs[y].Col(x));
    return ConstSegList(start, segs[y].ColLen(x));
  }
};

using ABuffer16u = ABuffer<uint16_t>;
using ABufferf = ABuffer<float>;

/// converts hits to a list of line segments to store in alpha buffer.
/// merges overlapping segments or segments within zRes to
///  a single segment.
/// @param[out] out_segments list of segments, each segment consists of
/// two endpoints.
int ConvertHitsToSegs(const HitList& hits, ABufferf::SegList& out_segments, float zRes);
