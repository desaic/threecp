#include "Image.h"

void transformImage(const ImageF32 & input, ImageF32 & output, double a, double b) {
  output.size = input.size;
  output.allocate();
  int centerx = output.size[0] / 2;
  int centery = output.size[1] / 2;
  for (int i = 0; i < (int)output.size[0]; i++) {
    for (int j = 0; j < (int)output.size[1]; j++) {
      output(i, j) = (float)(input(i, j) + a * (i - centerx) + b * (j - centery));
    }
  }
}
