#include "GLTex.h"
#include "GLFW/glfw3.h"

#include <iostream>

void GLTex::UpdateTextureData() {
  unsigned width = this->_width;
  unsigned format = GL_RGBA;
  unsigned type = GL_UNSIGNED_BYTE;
  if (!_needUpdate) {
    return;
  }
  _needUpdate = false;
  if (_numChan == 1) {
    format = GL_RED;
  } else if (_numChan == 4) {
    width /= 4;
  } else {
    std::cout << "can only display gray RGB, or RGBA image.\n";
    return;
  }
  //already allocated correct GL texture buffer
  if (!_needAlloc) {
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, _height, format, type,
                    _buf.data());
    return;
  }

  if (_texId > 0) {
    glDeleteTextures(1, &_texId);
  }
  GLenum errCode;
  errCode = glGetError();
  glGenTextures(1, &_texId);
  if (_texId == 0) {
    errCode = glGetError();
    std::cout << "Failed to generate gl texture id error " << errCode
              << "\n";
    return;
  }

  glBindTexture(GL_TEXTURE_2D, _texId);

  // Setup filtering parameters for display
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  GLenum wrap = GL_CLAMP;
  if (TexWrapParam > 0) {
    wrap = TexWrapParam;
  }
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
  // instead of opengl default 4 byte alignment
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  std::cout << width << " " << _height << "\n";

  glTexImage2D(GL_TEXTURE_2D, 0, format, width, _height, 0, format, type,
               _buf.data());
  errCode = glGetError();
  if (errCode != GL_NO_ERROR) {
    std::cout << "error setting texture data " << errCode << "\n";
  }
  _needAlloc = false;
}

int GLTex::SetImageData(const unsigned char* data, unsigned width, unsigned height,
                         int numChannels) {
  _needAlloc = !(HasImage() && _height == height &&
                   _width == width && _numChan == numChannels);
  _drawHeight = height;
  if (numChannels > 0) {
    _drawWidth = width / numChannels;
  }
  size_t numBytes = width * height;
  if (numChannels == 3) {
    // convert RGB to RGBA
    _numChan = 4;
    _width = width / 3 * 4;
    _height = height;
    _buf.resize(_width * _height);
    size_t dstIdx = 0;
    for (size_t i = 0; i < numBytes; i += 3) {
      _buf[dstIdx] = data[i];
      _buf[dstIdx + 1] = data[i + 1];
      _buf[dstIdx + 2] = data[i + 2];
      _buf[dstIdx + 3] = 255;
      dstIdx += 4;
    }
  } else {
    _numChan = numChannels;
    _width = width;
    _height = height;
    _buf.resize(numBytes);
    std::memcpy(_buf.data(), data, numBytes);
  }
  _needUpdate = true;
  return 0;
}

void GLTex::MakeDefaultTex() {
  std::vector<unsigned char> bytes(8 * 8 * 4, 255);
  SetImageData(bytes.data(), 8, 8, 4);
}

bool GLTex::HasImage() const{
  return (!_buf.empty()) && _buf.size() == (_width*_height); 
}