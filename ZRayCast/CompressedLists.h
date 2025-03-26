#pragma once
#include <climits>
#include <vector>

#include "FixedArray.h"
/// stores N columns of segment lists using a single list
/// to systematically get around struct size alignment issues.
/// Maximum of V.size() and N are only 4-bytes to save memory.
/// cannot modify other than reconstructing the whole thing.
template <typename T>
class CompressedLists {
 public:
  unsigned ColStart(unsigned col) const { return colIdx[col]; }
  unsigned ColEnd(unsigned col) const { return colIdx[col + 1]; }
  unsigned ColLen(unsigned col) const { return colIdx[col + 1] - colIdx[col]; }
  T& Col(unsigned col) { return V[colIdx[col]]; }
  const T& Col(unsigned col) const { return V[colIdx[col]]; }

  /// push a column after existing list of columns
  ///@return -1 if too many (> 4 billion) columns.
  ///  -2 if too many total items.
  int Compress(const std::vector<FixedArray<T> >& lists) {
    size_t numCols = lists.size();
    if (numCols >= size_t(UINT_MAX - 1)) {
      return -1;
    }
    size_t totalEle = 0;
    for (size_t i = 0; i < lists.size(); i++) {
      totalEle += lists[i].size();
    }
    if (totalEle >= size_t(UINT_MAX - 1)) {
      return -2;
    }

    colIdx.Allocate(lists.size() + 1);
    V.Allocate(totalEle);
    unsigned valIdx = 0;
    for (size_t i = 0; i < lists.size(); i++) {
      colIdx[i] = valIdx;
      for (size_t j = 0; j < lists[i].size(); j++) {
        V[valIdx] = lists[i][j];
        valIdx++;
      }
    }
    colIdx[numCols] = valIdx;
    return 0;
  }

 private:
  FixedArray<T> V;
  // col start = colIdx[is]
  // col end = colIdx[i+1];
  // colIdx.size() = N+1
  FixedArray<unsigned> colIdx;
};
