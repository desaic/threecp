/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                           License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2000-2008, 2018, Intel Corporation, all rights reserved.
// Copyright (C) 2009, Willow Garage Inc., all rights reserved.
// Copyright (C) 2014-2015, Itseez Inc., all rights reserved.
// Third party copyrights are property of their respective owners.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//   * The name of the copyright holders may not be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is" and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are disclaimed.
// In no event shall the Intel Corporation or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/
#include "MedianBlur.h"
// for min max
#include <assert.h>
#include <emmintrin.h>
#include <immintrin.h>

#include <algorithm>
#include <execution>

/// stolen from smooth.cpp from
/// https://github.com/opencv/opencv/blob/fc5d412ba738bf53c136c3a6c0a90ac66dc6634c/modules/imgproc/src/median_blur.simd.hpp
///
/****************************************************************************************\
                                      Median Filter
\****************************************************************************************/

#if _MSC_VER >= 1200
#pragma warning(disable : 4244)
#endif

typedef unsigned short ushort;
typedef unsigned short HT;
typedef uint8_t uchar;
/**
 * This structure represents a two-tier histogram. The first tier (known as the
 * "coarse" level) is 4 bit wide and the second tier (known as the "fine" level)
 * is 8 bit wide. Pixels inserted in the fine level also get inserted into the
 * coarse bucket designated by the 4 MSBs of the fine bucket value.
 *
 * The structure is aligned on 16 bits, which is a prerequisite for SIMD
 * instructions. Each bucket is 16 bit wide, which means that extra care must be
 * taken to prevent overflow.
 */
typedef struct {
  HT coarse[16];
  HT fine[16][16];
} Histogram;

#define CV_DECL_ALIGNED(x) __declspec(align(x))
#ifndef MIN
#define MIN(a, b) ((a) > (b) ? (b) : (a))
#endif

#ifndef MAX
#define MAX(a, b) ((a) < (b) ? (b) : (a))
#endif
#define MEDIAN_HAVE_SIMD 1

/*This small inline function aligns the pointer by the certian number of bytes
   by shifting it forward by 0 or a positive offset.*/
template <typename _Tp>
static inline _Tp* alignPtr(_Tp* ptr, int n = (int)sizeof(_Tp)) {
  return (_Tp*)(((size_t)ptr + n - 1) & -n);
}

static inline void histogram_add_simd(const HT x[16], HT y[16]) {
  const __m128i* rx = (const __m128i*)x;
  __m128i* ry = (__m128i*)y;
  __m128i r0 = _mm_add_epi16(_mm_load_si128(ry + 0), _mm_load_si128(rx + 0));
  __m128i r1 = _mm_add_epi16(_mm_load_si128(ry + 1), _mm_load_si128(rx + 1));
  _mm_store_si128(ry + 0, r0);
  _mm_store_si128(ry + 1, r1);
}

static inline void histogram_sub_simd(const HT x[16], HT y[16]) {
  const __m128i* rx = (const __m128i*)x;
  __m128i* ry = (__m128i*)y;
  __m128i r0 = _mm_sub_epi16(_mm_load_si128(ry + 0), _mm_load_si128(rx + 0));
  __m128i r1 = _mm_sub_epi16(_mm_load_si128(ry + 1), _mm_load_si128(rx + 1));
  _mm_store_si128(ry + 0, r0);
  _mm_store_si128(ry + 1, r1);
}

static inline void histogram_add(const HT x[16], HT y[16]) {
  int i;
  for (i = 0; i < 16; ++i) y[i] = (HT)(y[i] + x[i]);
}

static inline void histogram_sub(const HT x[16], HT y[16]) {
  int i;
  for (i = 0; i < 16; ++i) y[i] = (HT)(y[i] - x[i]);
}

static inline void histogram_muladd(int a, const HT x[16], HT y[16]) {
  for (int i = 0; i < 16; ++i) y[i] = (HT)(y[i] + a * x[i]);
}

void medianBlur_8u_O1(const Array2D8u& _src, Array2D8u& _dst, unsigned rad) {
/**
 * HOP is short for Histogram OPeration. This macro makes an operation \a op on
 * histogram \a h for pixel value \a x. It takes care of handling both levels.
 */
#define HOP(h, x, op) h.coarse[x >> 4] op, *((HT*)h.fine + x) op

#define COP(j, x, op) \
  h_coarse[16 * j + (x >> 4)] op, h_fine[16 * (n * ((x >> 4)) + j) + (x & 0xF)] op

  Vec2u size = _src.GetSize();
  if (size[0] == 0 || size[1] == 0) {
    return;
  }
  _dst.Allocate(size[0], size[1]);
  unsigned m = size[1], r = rad;
  size_t sstep = size[0], dstep = size[0];
  Histogram CV_DECL_ALIGNED(16) H;
  HT CV_DECL_ALIGNED(16) luc[16];

  unsigned STRIPE_SIZE = std::min(size[0], 512u);

  std::vector<HT> _h_coarse(1 * 16 * (STRIPE_SIZE + 2 * r) + 16);
  std::vector<HT> _h_fine(16 * 16 * (STRIPE_SIZE + 2 * r) + 16);
  HT* h_coarse = alignPtr(&_h_coarse[0], 16);
  HT* h_fine = alignPtr(&_h_fine[0], 16);
  for (unsigned x = 0; x < size[0]; x += STRIPE_SIZE) {
    unsigned i, j, k, n = std::min(size[0] - x, STRIPE_SIZE) + r * 2;
    const uint8_t* src = _src.DataPtr() + x;
    // dst points outside of the image as intended.
    uint8_t* dst = _dst.DataPtr() + x - r;

    memset(h_coarse, 0, 16 * n * sizeof(h_coarse[0]));
    memset(h_fine, 0, 16 * 16 * n * sizeof(h_fine[0]));

    // First row initialization

    for (j = 0; j < n; j++) COP(j, src[j], += r + 2);

    for (i = 1; i < r; i++) {
      const uint8_t* p = src + sstep * std::min(i, m - 1);
      for (j = 0; j < n; j++) COP(j, p[j], ++);
    }

    for (i = 0; i < m; i++) {
      const uchar* p0 = src + sstep * std::max(0, int(i) - int(r + 1));
      const uchar* p1 = src + sstep * std::min(m - 1, i + r);

      memset(&H, 0, sizeof(H));
      memset(luc, 0, sizeof(luc));

      // Update column histograms for the entire row.
      for (j = 0; j < n; j++) {
        COP(j, p0[j], --);
        COP(j, p1[j], ++);
      }

      // First column initialization
      for (k = 0; k < 16; ++k) histogram_muladd(2 * r + 1, &h_fine[16 * n * (k)], &H.fine[k][0]);

      for (j = 0; j < 2 * r; ++j) histogram_add_simd(&h_coarse[16 * j], H.coarse);

      for (j = r; int(j) < int(n) - int(r); j++) {
        int t = 2 * r * r + 2 * r, b, sum = 0;
        HT* segment;

        histogram_add_simd(&h_coarse[16 * (std::min(j + r, n - 1))], H.coarse);

        // Find median at coarse level
        for (k = 0; k < 16; ++k) {
          sum += H.coarse[k];
          if (sum > t) {
            sum -= H.coarse[k];
            break;
          }
        }
        assert(k < 16);

        /* Update corresponding histogram segment */
        if (luc[k] <= j - r) {
          memset(&H.fine[k], 0, 16 * sizeof(HT));
          for (luc[k] = j - r; luc[k] < MIN(j + r + 1, n); ++luc[k])
            histogram_add_simd(&h_fine[16 * (n * k + luc[k])], H.fine[k]);

          if (luc[k] < j + r + 1) {
            histogram_muladd(j + r + 1 - n, &h_fine[16 * (n * (k) + (n - 1))], &H.fine[k][0]);
            luc[k] = (HT)(j + r + 1);
          }
        } else {
          for (; luc[k] < j + r + 1; ++luc[k]) {
            histogram_sub_simd(&h_fine[16 * (n * (k) + MAX(luc[k] - 2 * int(r) - 1, 0))],
                               H.fine[k]);
            histogram_add_simd(&h_fine[16 * (n * (k) + MIN(luc[k], n - 1))], H.fine[k]);
          }
        }

        histogram_sub_simd(&h_coarse[16 * (MAX(j - r, 0))], H.coarse);

        /* Find median in segment */
        segment = H.fine[k];
        for (b = 0; b < 16; b++) {
          sum += segment[b];
          if (sum > t) {
            dst[dstep * i + j] = (uchar)(16 * k + b);
            break;
          }
        }
        assert(b < 16);
      }
    }
  }
#undef HOP
#undef COP
}

const uchar icvSaturate8u_cv[] = {
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   2,   3,   4,   5,   6,   7,   8,   9,
    10,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,
    29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
    48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,  64,  65,  66,
    67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,  80,  81,  82,  83,  84,  85,
    86,  87,  88,  89,  90,  91,  92,  93,  94,  95,  96,  97,  98,  99,  100, 101, 102, 103, 104,
    105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123,
    124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142,
    143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161,
    162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180,
    181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199,
    200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218,
    219, 220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237,
    238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
    255, 255, 255, 255, 255, 255, 255, 255, 255};

#define CV_FAST_CAST_8U(t) ((-256 <= (t) && (t) <= 512) ? icvSaturate8u_cv[(t) + 256] : 0)

struct MinMax8u {
  typedef uchar value_type;
  typedef int arg_type;
  void operator()(arg_type& a, arg_type& b) const {
    int t = CV_FAST_CAST_8U(a - b);
    b += t;
    a -= t;
  }
};

typedef int64_t int64;

inline int _v_cvtsi256_si32(const __m256i& a) {
  return _mm_cvtsi128_si32(_mm256_castsi256_si128(a));
}

struct v_uint8x32 {
  typedef uchar lane_type;
  enum { nlanes = 32 };
  __m256i val;

  explicit v_uint8x32(__m256i v) : val(v) {}
  /* coverity[uninit_ctor]: suppress warning */
  v_uint8x32() {}

  uchar get0() const { return (uchar)_v_cvtsi256_si32(val); }
};

namespace hal {

enum StoreMode { STORE_UNALIGNED = 0, STORE_ALIGNED = 1, STORE_ALIGNED_NOCACHE = 2 };

}

inline __m256i _v256_combine(const __m128i& lo, const __m128i& hi) {
  return _mm256_inserti128_si256(_mm256_castsi128_si256(lo), hi, 1);
}

inline __m256 _v256_combine(const __m128& lo, const __m128& hi) {
  return _mm256_insertf128_ps(_mm256_castps128_ps256(lo), hi, 1);
}

inline __m256d _v256_combine(const __m128d& lo, const __m128d& hi) {
  return _mm256_insertf128_pd(_mm256_castpd128_pd256(lo), hi, 1);
}
inline __m128i _v256_extract_high(const __m256i& v) { return _mm256_extracti128_si256(v, 1); }

inline __m128 _v256_extract_high(const __m256& v) { return _mm256_extractf128_ps(v, 1); }

inline __m128d _v256_extract_high(const __m256d& v) { return _mm256_extractf128_pd(v, 1); }

inline __m128i _v256_extract_low(const __m256i& v) { return _mm256_castsi256_si128(v); }

inline __m128 _v256_extract_low(const __m256& v) { return _mm256_castps256_ps128(v); }

inline __m128d _v256_extract_low(const __m256d& v) { return _mm256_castpd256_pd128(v); }

#define OPENCV_HAL_IMPL_AVX_LOADSTORE(_Tpvec, _Tp)                                              \
  inline _Tpvec v256_load(const _Tp* ptr) {                                                     \
    return _Tpvec(_mm256_loadu_si256((const __m256i*)ptr));                                     \
  }                                                                                             \
  inline _Tpvec v256_load_aligned(const _Tp* ptr) {                                             \
    return _Tpvec(_mm256_load_si256((const __m256i*)ptr));                                      \
  }                                                                                             \
  inline _Tpvec v256_load_low(const _Tp* ptr) {                                                 \
    __m128i v128 = _mm_loadu_si128((const __m128i*)ptr);                                        \
    return _Tpvec(_mm256_castsi128_si256(v128));                                                \
  }                                                                                             \
  inline _Tpvec v256_load_halves(const _Tp* ptr0, const _Tp* ptr1) {                            \
    __m128i vlo = _mm_loadu_si128((const __m128i*)ptr0);                                        \
    __m128i vhi = _mm_loadu_si128((const __m128i*)ptr1);                                        \
    return _Tpvec(_v256_combine(vlo, vhi));                                                     \
  }                                                                                             \
  inline void v_store(_Tp* ptr, const _Tpvec& a) { _mm256_storeu_si256((__m256i*)ptr, a.val); } \
  inline void v_store_aligned(_Tp* ptr, const _Tpvec& a) {                                      \
    _mm256_store_si256((__m256i*)ptr, a.val);                                                   \
  }                                                                                             \
  inline void v_store_aligned_nocache(_Tp* ptr, const _Tpvec& a) {                              \
    _mm256_stream_si256((__m256i*)ptr, a.val);                                                  \
  }                                                                                             \
  inline void v_store(_Tp* ptr, const _Tpvec& a, hal::StoreMode mode) {                         \
    if (mode == hal::STORE_UNALIGNED)                                                           \
      _mm256_storeu_si256((__m256i*)ptr, a.val);                                                \
    else if (mode == hal::STORE_ALIGNED_NOCACHE)                                                \
      _mm256_stream_si256((__m256i*)ptr, a.val);                                                \
    else                                                                                        \
      _mm256_store_si256((__m256i*)ptr, a.val);                                                 \
  }                                                                                             \
  inline void v_store_low(_Tp* ptr, const _Tpvec& a) {                                          \
    _mm_storeu_si128((__m128i*)ptr, _v256_extract_low(a.val));                                  \
  }                                                                                             \
  inline void v_store_high(_Tp* ptr, const _Tpvec& a) {                                         \
    _mm_storeu_si128((__m128i*)ptr, _v256_extract_high(a.val));                                 \
  }

OPENCV_HAL_IMPL_AVX_LOADSTORE(v_uint8x32, uchar)

typedef v_uint8x32 v_uint8;
inline v_uint8 vx_load(const uchar* ptr) { return v256_load(ptr); }

#define OPENCV_HAL_IMPL_AVX_BIN_FUNC(func, _Tpvec, intrin) \
  inline _Tpvec func(const _Tpvec& a, const _Tpvec& b) { return _Tpvec(intrin(a.val, b.val)); }

OPENCV_HAL_IMPL_AVX_BIN_FUNC(v_min, v_uint8x32, _mm256_min_epu8)
OPENCV_HAL_IMPL_AVX_BIN_FUNC(v_max, v_uint8x32, _mm256_max_epu8)

struct MinMaxVec8u {
  typedef uchar value_type;
  typedef v_uint8 arg_type;
  arg_type load(const uchar* ptr) { return vx_load(ptr); }
  void store(uchar* ptr, const arg_type& val) { v_store(ptr, val); }
  void operator()(arg_type& a, arg_type& b) const {
    arg_type t = a;
    a = v_min(a, b);
    b = v_max(b, t);
  }
};

// MinMax8u MinMaxVec8u
// template<class Op, class VecOp>
void medianBlur_SortNet(const Array2D8u& _src, Array2D8u& _dst, int m) {
  typedef uchar T;
  typedef int WT;
  typedef v_uint8 VT;

  Vec2u size = _src.GetSize();
  _dst.Allocate(size[0], size[1]);
  const T* src = _src.DataPtr();
  T* dst = _dst.DataPtr();
  int sstep = (int)(size[0] / sizeof(T));
  int dstep = (int)(size[0] / sizeof(T));
  int i, j, k;
  const int cn = 1;
  MinMax8u op;
  MinMaxVec8u vop;

  if (m == 3) {
    if (size[0] == 1 || size[1] == 1) {
      int len = size[0] + size[1] - 1;
      int sdelta = size[1] == 1 ? cn : sstep;
      int sdelta0 = size[1] == 1 ? 0 : sstep - cn;
      int ddelta = size[1] == 1 ? cn : dstep;

      for (i = 0; i < len; i++, src += sdelta0, dst += ddelta) {
        for (j = 0; j < cn; j++, src++) {
          WT p0 = src[i > 0 ? -sdelta : 0];
          WT p1 = src[0];
          WT p2 = src[i < len - 1 ? sdelta : 0];

          op(p0, p1);
          op(p1, p2);
          op(p0, p1);
          dst[j] = (T)p1;
        }
      }
      return;
    }

    for (i = 0; i < size[1]; i++, dst += dstep) {
      const T* row0 = src + std::max(i - 1, 0) * sstep;
      const T* row1 = src + i * sstep;
      const T* row2 = src + std::min(i + 1, int(size[1]) - 1) * sstep;
      int limit = cn;

      for (j = 0;;) {
        for (; j < limit; j++) {
          int j0 = j >= cn ? j - cn : j;
          int j2 = j < size[0] - cn ? j + cn : j;
          WT p0 = row0[j0], p1 = row0[j], p2 = row0[j2];
          WT p3 = row1[j0], p4 = row1[j], p5 = row1[j2];
          WT p6 = row2[j0], p7 = row2[j], p8 = row2[j2];

          op(p1, p2);
          op(p4, p5);
          op(p7, p8);
          op(p0, p1);
          op(p3, p4);
          op(p6, p7);
          op(p1, p2);
          op(p4, p5);
          op(p7, p8);
          op(p0, p3);
          op(p5, p8);
          op(p4, p7);
          op(p3, p6);
          op(p1, p4);
          op(p2, p5);
          op(p4, p7);
          op(p4, p2);
          op(p6, p4);
          op(p4, p2);
          dst[j] = (T)p4;
        }

        if (limit == size[0]) break;

        int nlanes = v_uint8::nlanes;

        for (; j <= size[0] - nlanes - cn; j += nlanes) {
          VT p0 = vop.load(row0 + j - cn), p1 = vop.load(row0 + j), p2 = vop.load(row0 + j + cn);
          VT p3 = vop.load(row1 + j - cn), p4 = vop.load(row1 + j), p5 = vop.load(row1 + j + cn);
          VT p6 = vop.load(row2 + j - cn), p7 = vop.load(row2 + j), p8 = vop.load(row2 + j + cn);

          vop(p1, p2);
          vop(p4, p5);
          vop(p7, p8);
          vop(p0, p1);
          vop(p3, p4);
          vop(p6, p7);
          vop(p1, p2);
          vop(p4, p5);
          vop(p7, p8);
          vop(p0, p3);
          vop(p5, p8);
          vop(p4, p7);
          vop(p3, p6);
          vop(p1, p4);
          vop(p2, p5);
          vop(p4, p7);
          vop(p4, p2);
          vop(p6, p4);
          vop(p4, p2);
          vop.store(dst + j, p4);
        }

        limit = size[0];
      }
    }
  } else if (m == 5) {
    if (size[0] == 1 || size[1] == 1) {
      int len = size[0] + size[1] - 1;
      int sdelta = size[1] == 1 ? cn : sstep;
      int sdelta0 = size[1] == 1 ? 0 : sstep - cn;
      int ddelta = size[1] == 1 ? cn : dstep;

      for (i = 0; i < len; i++, src += sdelta0, dst += ddelta)
        for (j = 0; j < cn; j++, src++) {
          int i1 = i > 0 ? -sdelta : 0;
          int i0 = i > 1 ? -sdelta * 2 : i1;
          int i3 = i < len - 1 ? sdelta : 0;
          int i4 = i < len - 2 ? sdelta * 2 : i3;
          WT p0 = src[i0], p1 = src[i1], p2 = src[0], p3 = src[i3], p4 = src[i4];

          op(p0, p1);
          op(p3, p4);
          op(p2, p3);
          op(p3, p4);
          op(p0, p2);
          op(p2, p4);
          op(p1, p3);
          op(p1, p2);
          dst[j] = (T)p2;
        }
      return;
    }

    for (i = 0; i < size[1]; i++, dst += dstep) {
      const T* row[5];
      row[0] = src + std::max(i - 2, 0) * sstep;
      row[1] = src + std::max(i - 1, 0) * sstep;
      row[2] = src + i * sstep;
      row[3] = src + std::min(i + 1, int(size[1]) - 1) * sstep;
      row[4] = src + std::min(i + 2, int(size[1]) - 1) * sstep;
      int limit = cn * 2;

      for (j = 0;;) {
        for (; j < limit; j++) {
          WT p[25];
          int j1 = j >= cn ? j - cn : j;
          int j0 = j >= cn * 2 ? j - cn * 2 : j1;
          int j3 = j < size[0] - cn ? j + cn : j;
          int j4 = j < size[0] - cn * 2 ? j + cn * 2 : j3;
          for (k = 0; k < 5; k++) {
            const T* rowk = row[k];
            p[k * 5] = rowk[j0];
            p[k * 5 + 1] = rowk[j1];
            p[k * 5 + 2] = rowk[j];
            p[k * 5 + 3] = rowk[j3];
            p[k * 5 + 4] = rowk[j4];
          }

          op(p[1], p[2]);
          op(p[0], p[1]);
          op(p[1], p[2]);
          op(p[4], p[5]);
          op(p[3], p[4]);
          op(p[4], p[5]);
          op(p[0], p[3]);
          op(p[2], p[5]);
          op(p[2], p[3]);
          op(p[1], p[4]);
          op(p[1], p[2]);
          op(p[3], p[4]);
          op(p[7], p[8]);
          op(p[6], p[7]);
          op(p[7], p[8]);
          op(p[10], p[11]);
          op(p[9], p[10]);
          op(p[10], p[11]);
          op(p[6], p[9]);
          op(p[8], p[11]);
          op(p[8], p[9]);
          op(p[7], p[10]);
          op(p[7], p[8]);
          op(p[9], p[10]);
          op(p[0], p[6]);
          op(p[4], p[10]);
          op(p[4], p[6]);
          op(p[2], p[8]);
          op(p[2], p[4]);
          op(p[6], p[8]);
          op(p[1], p[7]);
          op(p[5], p[11]);
          op(p[5], p[7]);
          op(p[3], p[9]);
          op(p[3], p[5]);
          op(p[7], p[9]);
          op(p[1], p[2]);
          op(p[3], p[4]);
          op(p[5], p[6]);
          op(p[7], p[8]);
          op(p[9], p[10]);
          op(p[13], p[14]);
          op(p[12], p[13]);
          op(p[13], p[14]);
          op(p[16], p[17]);
          op(p[15], p[16]);
          op(p[16], p[17]);
          op(p[12], p[15]);
          op(p[14], p[17]);
          op(p[14], p[15]);
          op(p[13], p[16]);
          op(p[13], p[14]);
          op(p[15], p[16]);
          op(p[19], p[20]);
          op(p[18], p[19]);
          op(p[19], p[20]);
          op(p[21], p[22]);
          op(p[23], p[24]);
          op(p[21], p[23]);
          op(p[22], p[24]);
          op(p[22], p[23]);
          op(p[18], p[21]);
          op(p[20], p[23]);
          op(p[20], p[21]);
          op(p[19], p[22]);
          op(p[22], p[24]);
          op(p[19], p[20]);
          op(p[21], p[22]);
          op(p[23], p[24]);
          op(p[12], p[18]);
          op(p[16], p[22]);
          op(p[16], p[18]);
          op(p[14], p[20]);
          op(p[20], p[24]);
          op(p[14], p[16]);
          op(p[18], p[20]);
          op(p[22], p[24]);
          op(p[13], p[19]);
          op(p[17], p[23]);
          op(p[17], p[19]);
          op(p[15], p[21]);
          op(p[15], p[17]);
          op(p[19], p[21]);
          op(p[13], p[14]);
          op(p[15], p[16]);
          op(p[17], p[18]);
          op(p[19], p[20]);
          op(p[21], p[22]);
          op(p[23], p[24]);
          op(p[0], p[12]);
          op(p[8], p[20]);
          op(p[8], p[12]);
          op(p[4], p[16]);
          op(p[16], p[24]);
          op(p[12], p[16]);
          op(p[2], p[14]);
          op(p[10], p[22]);
          op(p[10], p[14]);
          op(p[6], p[18]);
          op(p[6], p[10]);
          op(p[10], p[12]);
          op(p[1], p[13]);
          op(p[9], p[21]);
          op(p[9], p[13]);
          op(p[5], p[17]);
          op(p[13], p[17]);
          op(p[3], p[15]);
          op(p[11], p[23]);
          op(p[11], p[15]);
          op(p[7], p[19]);
          op(p[7], p[11]);
          op(p[11], p[13]);
          op(p[11], p[12]);
          dst[j] = (T)p[12];
        }

        if (limit == size[0]) break;

        int nlanes = v_uint8::nlanes;

        for (; j <= size[0] - nlanes - cn * 2; j += nlanes) {
          VT p0 = vop.load(row[0] + j - cn * 2), p5 = vop.load(row[1] + j - cn * 2),
             p10 = vop.load(row[2] + j - cn * 2), p15 = vop.load(row[3] + j - cn * 2),
             p20 = vop.load(row[4] + j - cn * 2);
          VT p1 = vop.load(row[0] + j - cn * 1), p6 = vop.load(row[1] + j - cn * 1),
             p11 = vop.load(row[2] + j - cn * 1), p16 = vop.load(row[3] + j - cn * 1),
             p21 = vop.load(row[4] + j - cn * 1);
          VT p2 = vop.load(row[0] + j - cn * 0), p7 = vop.load(row[1] + j - cn * 0),
             p12 = vop.load(row[2] + j - cn * 0), p17 = vop.load(row[3] + j - cn * 0),
             p22 = vop.load(row[4] + j - cn * 0);
          VT p3 = vop.load(row[0] + j + cn * 1), p8 = vop.load(row[1] + j + cn * 1),
             p13 = vop.load(row[2] + j + cn * 1), p18 = vop.load(row[3] + j + cn * 1),
             p23 = vop.load(row[4] + j + cn * 1);
          VT p4 = vop.load(row[0] + j + cn * 2), p9 = vop.load(row[1] + j + cn * 2),
             p14 = vop.load(row[2] + j + cn * 2), p19 = vop.load(row[3] + j + cn * 2),
             p24 = vop.load(row[4] + j + cn * 2);

          vop(p1, p2);
          vop(p0, p1);
          vop(p1, p2);
          vop(p4, p5);
          vop(p3, p4);
          vop(p4, p5);
          vop(p0, p3);
          vop(p2, p5);
          vop(p2, p3);
          vop(p1, p4);
          vop(p1, p2);
          vop(p3, p4);
          vop(p7, p8);
          vop(p6, p7);
          vop(p7, p8);
          vop(p10, p11);
          vop(p9, p10);
          vop(p10, p11);
          vop(p6, p9);
          vop(p8, p11);
          vop(p8, p9);
          vop(p7, p10);
          vop(p7, p8);
          vop(p9, p10);
          vop(p0, p6);
          vop(p4, p10);
          vop(p4, p6);
          vop(p2, p8);
          vop(p2, p4);
          vop(p6, p8);
          vop(p1, p7);
          vop(p5, p11);
          vop(p5, p7);
          vop(p3, p9);
          vop(p3, p5);
          vop(p7, p9);
          vop(p1, p2);
          vop(p3, p4);
          vop(p5, p6);
          vop(p7, p8);
          vop(p9, p10);
          vop(p13, p14);
          vop(p12, p13);
          vop(p13, p14);
          vop(p16, p17);
          vop(p15, p16);
          vop(p16, p17);
          vop(p12, p15);
          vop(p14, p17);
          vop(p14, p15);
          vop(p13, p16);
          vop(p13, p14);
          vop(p15, p16);
          vop(p19, p20);
          vop(p18, p19);
          vop(p19, p20);
          vop(p21, p22);
          vop(p23, p24);
          vop(p21, p23);
          vop(p22, p24);
          vop(p22, p23);
          vop(p18, p21);
          vop(p20, p23);
          vop(p20, p21);
          vop(p19, p22);
          vop(p22, p24);
          vop(p19, p20);
          vop(p21, p22);
          vop(p23, p24);
          vop(p12, p18);
          vop(p16, p22);
          vop(p16, p18);
          vop(p14, p20);
          vop(p20, p24);
          vop(p14, p16);
          vop(p18, p20);
          vop(p22, p24);
          vop(p13, p19);
          vop(p17, p23);
          vop(p17, p19);
          vop(p15, p21);
          vop(p15, p17);
          vop(p19, p21);
          vop(p13, p14);
          vop(p15, p16);
          vop(p17, p18);
          vop(p19, p20);
          vop(p21, p22);
          vop(p23, p24);
          vop(p0, p12);
          vop(p8, p20);
          vop(p8, p12);
          vop(p4, p16);
          vop(p16, p24);
          vop(p12, p16);
          vop(p2, p14);
          vop(p10, p22);
          vop(p10, p14);
          vop(p6, p18);
          vop(p6, p10);
          vop(p10, p12);
          vop(p1, p13);
          vop(p9, p21);
          vop(p9, p13);
          vop(p5, p17);
          vop(p13, p17);
          vop(p3, p15);
          vop(p11, p23);
          vop(p11, p15);
          vop(p7, p19);
          vop(p7, p11);
          vop(p11, p13);
          vop(p11, p12);
          vop.store(dst + j, p12);
        }

        limit = size[0];
      }
    }
  }
}

void MedianBlur(const Array2D8u& src, Array2D8u& dst, unsigned rad) {
  if (rad == 0) {
    dst = src;
    return;
  }
  if (rad == 1) {
    medianBlur_SortNet(src, dst, 3);
  } else if (rad == 2) {
    medianBlur_SortNet(src, dst, 5);
  } else {
    medianBlur_8u_O1(src, dst, rad);
  }
}