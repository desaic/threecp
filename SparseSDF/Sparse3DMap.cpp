#include "Sparse3DMap.h"

template <typename TIndex>
void Sparse3DMap<TIndex>::Allocate(unsigned x, unsigned y, unsigned z) {
  grid_.Allocate(x, y, z);
  for (SparseNode4<TIndex>& node : grid_.GetData()) {
    node.Clear();
  }
  count_ = 1;
}

template <typename TIndex>
TIndex Sparse3DMap<TIndex>::AddDense(unsigned x, unsigned y, unsigned z) {
  SparseNode4<TIndex>& node = GetSparseNode4(x, y, z);
  if (!node.HasChildren()) {
    node.AddChildrenDense(0);
  }
  Vec3u fineIdx(x & 3, y & 3, z & 3);
  TIndex* cellIdxPtr = node.AddChildDense(fineIdx[0], fineIdx[1], fineIdx[2]);
  TIndex cellIdx = *cellIdxPtr;
  // in dense node, 0 means unintialized cell
  if (cellIdx == 0) {
    cellIdx = count_;
    *cellIdxPtr = count_;
    count_++;
  }
  return cellIdx;
}

template <typename TIndex>
void Sparse3DMap<TIndex>::Compress() {
  auto& data = grid_.GetData();
  for (auto& node : data) {
    node.Compress();
  }
}

template <typename TIndex>
bool Sparse3DMap<TIndex>::HasDense(unsigned x, unsigned y, unsigned z) const {
  const SparseNode4<TIndex>& node = GetSparseNode4(x, y, z);

  if (!node.HasChildren()) {
    return false;
  }

  Vec3u fineIdx(x & 3, y & 3, z & 3);
  const TIndex* cellIdxPtr =
      node.GetChildDense(fineIdx[0], fineIdx[1], fineIdx[2]);
  TIndex cellIdx = *cellIdxPtr;
  return cellIdx > 0;
}

template <typename TIndex>
bool Sparse3DMap<TIndex>::HasSparse(unsigned x, unsigned y, unsigned z) const {
  const SparseNode4<TIndex>& node = GetSparseNode4(x, y, z);

  if (!node.HasChildren()) {
    return false;
  }
  return node.HasChild(x & 3, y & 3, z & 3);
}

template <typename TIndex>
SparseNode4<TIndex>& Sparse3DMap<TIndex>::GetSparseNode4(unsigned x, unsigned y,
                                                         unsigned z) {
  return grid_(x / 4, y / 4, z / 4);
}

template <typename TIndex>
const SparseNode4<TIndex>& Sparse3DMap<TIndex>::GetSparseNode4(
    unsigned x, unsigned y, unsigned z) const {
  return grid_(x / 4, y / 4, z / 4);
}

template class Sparse3DMap<unsigned>;
template class Sparse3DMap<size_t>;