#pragma once
#include <vector>

struct GLTex{
  /// only supports 1 or 4 channel images because
  /// hardware likes 4 bytes.
  unsigned _numChan = 4;
  /// width in number of bytes, not number of pixels.
  unsigned _width = 1, _height = 1;
  // image can be shown at different scales.
  unsigned _drawWidth = 1, _drawHeight = 1;
  std::vector<uint8_t> _buf;
  bool HasImage() const;
  void UpdateTextureData();
  unsigned int _texId = 0;
  bool _needUpdate = false;
  // true if the texture data in GPU needs to be reallocated
  bool _needAlloc = false;
  float renderScale_ = 1.0f;

  int SetImageData(const unsigned char* data, unsigned width, unsigned height,
                    int numChannels);
  //an 8x8 white image.
  void MakeDefaultTex();

  unsigned int TexWrapParam = 0;
};