#ifndef VOX_CALLBACK_H
#define VOX_CALLBACK_H

struct VoxCallback {
  /// Called when voxel xyz intersects triangle
  virtual void operator()(unsigned x, unsigned y, unsigned z,
                          unsigned long long trigIdx) {}
  /// can be used by callbacks that stores pre-triangle data
  virtual void BeginTrig(unsigned long long trigIdx) {}
  /// free any triangle specific data.
  virtual void EndTrig(unsigned long long trigIdx) {}
};

#endif