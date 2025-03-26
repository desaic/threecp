#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include "FixedArray.h"
#include "Array2D.h"
#include "Array3D.h"
#include "cpu_voxelizer.h"

#include "AdapDF.h"
#include "MarchingCubes.h"
#include "TrigMesh.h"
#include "Vec3.h"
#include "lodepng.h"
#include "meshutil.h"
#include "PointTrigDist.h"

void MirrorCubicStructure(const Array3D8u& s_in, Array3D8u& s_out);

void loadBinaryStructure(const std::string& filename, Array3D8u& s) {
  int dim = 3;
  std::ifstream in(filename, std::ios::in | std::ios::binary);
  if (!in.good()) {
    std::cout << "Cannot open input " << filename << "\n";
    in.close();
    return;
  }
  unsigned int gridSize[3];
  for (int i = 0; i < dim; i++) {
    in.read((char*)(&gridSize[i]), sizeof(int));
  }

  s.Allocate(gridSize[0], gridSize[1], gridSize[2]);

  std::vector<uint8_t>& vox = s.GetData();
  for (size_t i = 0; i < vox.size(); i += 8) {
    unsigned char aggr;
    in.read((char*)&aggr, sizeof(unsigned char));
    // 1 bit per voxel.
    size_t j = 0;
    for (unsigned char mask = 1; mask > 0; mask <<= 1, j++)
      vox[i + j] = (aggr & mask) > 0;
  }
  in.close();
}

void Upsample2x(const Array3D8u& gridIn, Array3D8u& gridOut) {
  Vec3u oldSize = gridIn.GetSize();
  gridOut.Allocate(oldSize[0] * 2, oldSize[1] * 2, oldSize[2] * 2);
  Vec3u newSize = gridOut.GetSize();
  for (unsigned i = 0; i < newSize[0]; i++) {
    for (unsigned j = 0; j < newSize[1]; j++) {
      for (unsigned k = 0; k < newSize[2]; k++) {
        gridOut(i, j, k) = gridIn(i / 2, j / 2, k / 2);
      }
    }
  }
}

void Downsample2x(const Array3D8u& gridIn, Array3D8u& gridOut) {
  Vec3u oldSize = gridIn.GetSize();
  gridOut.Allocate(oldSize[0] / 2, oldSize[1] / 2, oldSize[2] / 2);
  Vec3u newSize = gridOut.GetSize();
  for (unsigned i = 0; i < newSize[0]; i++) {
    unsigned i0 = i * 2;
    for (unsigned j = 0; j < newSize[1]; j++) {
      unsigned j0 = j * 2;
      for (unsigned k = 0; k < newSize[2]; k++) {
        unsigned k0 = k * 2;
        int sum = gridIn(i0, j0, k0);
        sum += gridIn(i0 + 1, j0, k0);
        sum += gridIn(i0, j0 + 1, k0);
        sum += gridIn(i0 + 1, j0 + 1, k0);
        sum += gridIn(i0, j0, k0 + 1);
        sum += gridIn(i0 + 1, j0, k0 + 1);
        sum += gridIn(i0, j0 + 1, k0 + 1);
        sum += gridIn(i0 + 1, j0 + 1, k0 + 1);
        sum /= 8;
        gridOut(i, j, k) = sum;
      }
    }
  }
}

void PadGrid(const Array3D8u& grid, Array3D8u& padded) {
  Vec3u oldSize = grid.GetSize();
  padded.Allocate(oldSize[0] + 2, oldSize[1] + 2, oldSize[2] + 2);
  Vec3u newSize = padded.GetSize();
  for (unsigned i = 0; i < oldSize[0]; i++) {
    for (unsigned j = 0; j < oldSize[1]; j++) {
      for (unsigned k = 0; k < oldSize[2]; k++) {
        padded(i + 1, j + 1, k + 1) = grid(i, j, k);
      }
    }
  }
  // pad x
  for (unsigned y = 0; y < newSize[1]; y++) {
    unsigned srcy = y - 1;
    if (y == 0) {
      srcy = 0;
    } else if (y == newSize[1] - 1) {
      srcy = oldSize[1] - 1;
    }

    for (unsigned z = 0; z < newSize[2]; z++) {
      unsigned srcz = z - 1;
      if (z == 0) {
        srcz = 0;
      } else if (z == newSize[2] - 1) {
        srcz = oldSize[2] - 1;
      }
      padded(0, y, z) = grid(0, srcy, srcz);
      padded(newSize[0] - 1, y, z) = grid(oldSize[0] - 1, srcy, srcz);
    }
  }

  // pad y
  for (unsigned x = 0; x < newSize[0]; x++) {
    unsigned srcx = x - 1;
    if (x == 0) {
      srcx = 0;
    } else if (x == newSize[0] - 1) {
      srcx = oldSize[0] - 1;
    }

    for (unsigned z = 0; z < newSize[2]; z++) {
      unsigned srcz = z - 1;
      if (z == 0) {
        srcz = 0;
      } else if (z == newSize[2] - 1) {
        srcz = oldSize[2] - 1;
      }
      padded(x, 0, z) = grid(srcx, 0, srcz);
      padded(x, newSize[1] - 1, z) = grid(srcx, oldSize[1] - 1, srcz);
    }
  }

  for (unsigned x = 1; x < newSize[0] - 1; x++) {
    for (unsigned y = 1; y < newSize[1] - 1; y++) {
      padded(x, y, 0) = grid(x - 1, y - 1, 0);
      padded(x, y, newSize[2] - 1) = grid(x - 1, y - 1, oldSize[2] - 1);
    }
  }
}

void PadGridConst(const Array3D8u& grid, Array3D8u& padded, uint8_t val) {
  Vec3u oldSize = grid.GetSize();
  padded.Allocate(oldSize[0] + 2, oldSize[1] + 2, oldSize[2] + 2);
  Vec3u newSize = padded.GetSize();
  padded.Fill(val);
  for (unsigned i = 0; i < oldSize[0]; i++) {
    for (unsigned j = 0; j < oldSize[1]; j++) {
      for (unsigned k = 0; k < oldSize[2]; k++) {
        padded(i + 1, j + 1, k + 1) = grid(i, j, k);
      }
    }
  }
}

void ExpandCellToVerts(const Array3D8u& grid, Array3D8u& verts) {
  Vec3u oldSize = grid.GetSize();
  verts.Allocate(oldSize[0] + 1, oldSize[1] + 1, oldSize[2] + 1);
  Vec3u newSize = verts.GetSize();
  verts.Fill(0);

  Array3D8u padded;
  PadGrid(grid, padded);

  for (unsigned i = 0; i < newSize[0]; i++) {
    for (unsigned j = 0; j < newSize[1]; j++) {
      for (unsigned k = 0; k < newSize[2]; k++) {
        uint8_t val = padded(i, j, k);
        val = padded(i, j + 1, k);
        val = padded(i + 1, j, k);
        val = padded(i + 1, j + 1, k);

        val = padded(i, j, k + 1);
        val = padded(i, j + 1, k + 1);
        val = padded(i + 1, j, k + 1);
        val = padded(i + 1, j + 1, k + 1);
        verts(i, j, k) = val;
      }
    }
  }
}

void Scale(Array3D8u& grid, float scale) {
  std::vector<uint8_t>& data = grid.GetData();
  for (size_t i = 0; i < data.size(); i++) {
    data[i] = uint8_t(scale * data[i]);
  }
}

void Thresh(Array3D8u& grid, float t) {
  std::vector<uint8_t>& data = grid.GetData();
  for (size_t i = 0; i < data.size(); i++) {
    if (data[i] > t) {
      data[i] = 1;
    } else {
      data[i] = 0;
    }
  }
}

void InvertVal(Array3D8u& grid) {
  std::vector<uint8_t>& data = grid.GetData();
  for (size_t i = 0; i < data.size(); i++) {
    data[i] = 255 - data[i];
  }
}

void Normalize(std::vector<float>& v) {
  if (v.size() == 0) {
    return;
  }
  float sum = 0;
  for (size_t i = 0; i < v.size(); i++) {
    sum += v[i];
  }
  if (sum == 0) {
    return;
  }
  float scale = 1.0f / sum;
  for (size_t i = 0; i < v.size(); i++) {
    v[i] *= scale;
  }
}

int GaussianFilter1D(float sigma, unsigned rad, std::vector<float>& filter) {
  if (rad > 100) {
    return -1;
  }
  if (sigma < 0.01f) {
    sigma = 0.01f;
  }
  unsigned filterSize = 2 * rad + 1;
  filter.resize(filterSize);
  float pi = 3.14159f;
  float factor = 1.0f / (sigma * std::sqrt(2 * pi));
  filter[rad] = factor;
  for (unsigned i = 0; i < rad; i++) {
    float x = (float(i) + 1) / sigma;
    float gauss = factor * std::exp(-0.5f * x * x);
    filter[rad + i + 1] = gauss;
    filter[rad - 1 - i] = gauss;
  }
  Normalize(filter);
  return 0;
}

// with mirror boundary condition
void FilterVec(uint8_t* v, size_t len, const std::vector<float>& kern) {
  size_t halfKern = kern.size() / 2;

  std::vector<uint8_t> padded(len + 2 * halfKern);
  for (size_t i = 0; i < halfKern; i++) {
    padded[i] = v[halfKern - i];
    padded[len - i] = v[len - halfKern + i];
  }
  for (size_t i = 0; i < len; i++) {
    padded[i + halfKern] = v[i];
  }

  for (size_t i = 0; i < len; i++) {
    float sum = 0;
    for (size_t j = 0; j < kern.size(); j++) {
      sum += kern[j] * padded[i + j];
    }
    v[i] = uint8_t(sum);
  }
}

void FilterDecomp(Array3D8u& grid, const std::vector<float>& kern) {
  Vec3u gsize = grid.GetSize();
  // z
  for (unsigned i = 0; i < gsize[0]; i++) {
    for (unsigned j = 0; j < gsize[1]; j++) {
      uint8_t* vec = &grid(i, j, 0);
      FilterVec(vec, gsize[2], kern);
    }
  }
  // y
  for (unsigned i = 0; i < gsize[0]; i++) {
    for (unsigned k = 0; k < gsize[2]; k++) {
      std::vector<uint8_t> v(gsize[1]);
      for (unsigned j = 0; j < gsize[1]; j++) {
        v[j] = grid(i, j, k);
      }
      FilterVec(v.data(), gsize[1], kern);
      for (unsigned j = 0; j < gsize[1]; j++) {
        grid(i, j, k) = v[j];
      }
    }
  }
  // x
  for (unsigned j = 0; j < gsize[1]; j++) {
    for (unsigned k = 0; k < gsize[2]; k++) {
      std::vector<uint8_t> v(gsize[0]);
      for (unsigned i = 0; i < gsize[0]; i++) {
        v[i] = grid(i, j, k);
      }
      FilterVec(v.data(), gsize[0], kern);
      for (unsigned i = 0; i < gsize[0]; i++) {
        grid(i, j, k) = v[i];
      }
    }
  }
}

int SavePng(const std::string& filename, Array2D8u& slice) {
  lodepng::State state;
  // input color type
  state.info_raw.colortype = LCT_GREY;
  state.info_raw.bitdepth = 8;
  // output color type
  state.info_png.color.colortype = LCT_GREY;
  state.info_png.color.bitdepth = 8;
  state.encoder.auto_convert = 0;
  std::vector<unsigned char> out;
  Vec2u size = slice.GetSize();
  lodepng::encode(out, slice.GetData(), size[0], size[1], state);
  std::ofstream outFile(filename, std::ofstream::out | std::ofstream::binary);
  outFile.write((const char*)out.data(), out.size());
  outFile.close();
  return 0;
}

int LoadPngGrey(const std::string& filename, Array2D8u& arr) {
  std::vector<unsigned char> buf;
  lodepng::State state;
  state.info_raw.colortype = LCT_GREY;
  state.info_raw.bitdepth = 8;
  state.info_png.color.colortype = LCT_GREY;
  state.info_png.color.bitdepth = 8;
  unsigned error = lodepng::load_file(buf, filename.c_str());
  if (error) {
    return -1;
  }
  unsigned w, h;
  arr.GetData().clear();
  error = lodepng::decode(arr.GetData(), w, h, state, buf);
  if (error) {
    return -2;
  }
  arr.SetSize(w, h);
  return 0;
}
void SaveSlices(const std::string& prefix, const Array3D8u& vol) {
  Vec3u size = vol.GetSize();
  Array2D8u slice(size[0], size[1]);
  for (unsigned z = 0; z < size[2]; z++) {
    for (unsigned y = 0; y < size[1]; y++) {
      for (unsigned x = 0; x < size[0]; x++) {
        slice(x, y) = vol(x, y, z);
      }
    }
    std::stringstream ss;
    ss << prefix;
    ss << std::setw(4) << std::setfill('0') << z << ".png";
    std::string outFile = ss.str();
    SavePng(outFile, slice);
  }
}

void LoadImageSequence(const std::string& dir, int maxIndex, Array3D8u& vol) {
  int lasti = 0;
  for (int i = 0; i <= maxIndex; i++) {
    std::string imageFile = dir + "/" + std::to_string(i) + ".png";
    std::filesystem::path imagePath(imageFile);
    if (!std::filesystem::exists(imagePath)) {
      // give up after 10 missing images.
      if (i - lasti > 10) {
        break;
      }
      continue;
    }
    lasti = i;

    Array2D8u image;
    LoadPngGrey(imageFile, image);
    if (i == 0) {
      vol.Allocate(image.GetSize()[0], image.GetSize()[1], maxIndex + 1);
    }
    Vec2u imageSize = image.GetSize();
    unsigned rows = std::min(imageSize[1], vol.GetSize()[1]);
    unsigned cols = std::min(imageSize[0], vol.GetSize()[0]);
    for (unsigned row = 0; row < rows; row++) {
      for (unsigned col = 0; col < cols; col++) {
        vol(col, row, i) = image(col, row);
      }
    }
  }
}

#include <memory>

#include "BBox.h"
#include "Stopwatch.h"
#include "cpu_voxelizer.h"
#include "FastSweep.h"
#include "SDFVoxCb.h"
#include "VoxCallback.h"
#include "AdapSDF.h"

void GetSlice8u(const Array3D<short>& sdf, Array2D8u & slice, unsigned z)
{
  Vec3u gridSize = sdf.GetSize();
  slice.Allocate(gridSize[0], gridSize[1]);
  const short* srcPtr = sdf.DataPtr() + z * gridSize[0] * gridSize[1];
  uint8_t* dstPtr = slice.DataPtr();
  size_t numPix = gridSize[0] * gridSize[1];
  for (size_t i = 0; i < numPix; i++) {
    dstPtr[i] = srcPtr[i]/10;
  }
}

void SaveSDFImages(const std::string & prefix, const Array3D<short> &sdf) {
  Vec3u gridSize = sdf.GetSize();
  for (unsigned z = 0; z < gridSize[2]; z++) {
    Array2D8u slice;
    GetSlice8u(sdf, slice, z);
    std::string filename = prefix + std::to_string(z) + ".png";
    SavePng(filename, slice);
  }
}

void GetSDFSlice(const AdapSDF& sdf, Array2D8u &slice, Vec3f sliceRes, float z) {
  Vec2u sliceSize = slice.GetSize();
  float MaxDrawDist = 1.2f;
  float distScale = 100;
  for (unsigned row = 0; row < sliceSize[1]; row++) {
    for (unsigned col = 0; col < sliceSize[0]; col++) {
      Vec3f x((col +0.5f)* sliceRes[0], (row+0.5f)*sliceRes[1], z);
      float dist = sdf.GetCoarseDist(x+sdf.origin);
      if (dist < MaxDrawDist && dist > -MaxDrawDist) {
        slice(col, row) = uint8_t(dist * distScale + 127);
      }
      if (dist >= MaxDrawDist) {
        slice(col, row) = 255;
      }
      if (dist <= -MaxDrawDist) {
        slice(col, row) = 0;
      }
      if (dist > -0.016 && dist < 0.016) {
        slice(col, row) = 255;
      }
    }
  }
}

void GetSDFFineSlice(const AdapSDF& sdf, Array2D8u& slice, Vec3f sliceRes,
                 float z) {
  Vec2u sliceSize = slice.GetSize();
  float MaxDrawDist = 1.2f;
  float distScale = 100;
  for (unsigned row = 0; row < sliceSize[1]; row++) {
    for (unsigned col = 0; col < sliceSize[0]; col++) {
      Vec3f x((col + 0.5f) * sliceRes[0], (row + 0.5f) * sliceRes[1], z);
      if (row == 255 && col == 1600) {
        std::cout << "debug\n";
      }
      float dist = sdf.GetFineDist(x + sdf.origin);
      if (dist < MaxDrawDist && dist > -MaxDrawDist) {
        slice(col, row) = uint8_t(dist * distScale + 127);
      }
      else if (dist >= MaxDrawDist) {
        slice(col, row) = 255;
      }
      else if (dist <= -MaxDrawDist) {
        slice(col, row) = 0;
      }
      if (dist > -0.016 && dist < 0.016) {
        slice(col, row) = 255;
      }
    }
  }
}

void TestPointSample() {
  std::string fileName1 = "F:/dolphin/meshes/fish/salmon.stl";
  TrigMesh mesh1;
  mesh1.LoadStl(fileName1);
  std::vector<SurfacePoint> points;
  SamplePointsOneTrig(0, mesh1, points, 0.1f);
  std::ofstream out("pointSample.obj");
  Triangle trig = mesh1.GetTriangleVerts(0);
  for (size_t i = 0; i < 3; i++) {
    out << "v " << trig.v[i][0] << " " << trig.v[i][1] << " " << trig.v[i][2]
        << "\n";
  }
  for (size_t i = 0; i < points.size(); i++) {
    out << "v " << points[i].v[0] << " " << points[i].v[1] << " "
        << points[i].v[2] << "\n";
  }
  out << "f 1 2 3\n";
}

void SavePseudoNormals(const TrigMesh & mesh, const std::string & outFile ) {
  size_t numTrig = mesh.t.size() / 3;
  size_t numEdges = mesh.ne.size();
  std::ofstream out(outFile);
  std::vector<bool> visited(numEdges, false);
  size_t vertCount = 1;
  for (size_t t = 0; t < numTrig; t++) {
    Triangle trig = mesh.GetTriangleVerts(t);
    for (unsigned ei = 0; ei < 3; ei++) {
      size_t eIdx = mesh.te[3 * t + ei];
      if (visited[eIdx]) {
        continue;
      }
      Vec3f edgeCenter = 0.5f*(trig.v[ei]+trig.v[(ei+1)%3]);
      Vec3f normalTop = mesh.ne[eIdx];
      out << "v " << edgeCenter[0] << " " << edgeCenter[1] << " "
          << edgeCenter[2] << "\n";
      normalTop = normalTop + edgeCenter;
      out << "v " << normalTop[0] << " " << normalTop[1] << " " << normalTop[2]
          << "\n";
      out << "l " << vertCount << " " << (vertCount + 1) << "\n";
      vertCount += 2;
    }
  }
}

void CheckCornerCase(const AdapSDF &sdf) { Vec3u size = sdf.dist.GetSize();
  float voxSize = sdf.voxSize;
  int nbr[6][3] = {{-1, 0, 0}, {1, 0, 0},  {0, -1, 0},
                   {0, 1, 0},  {0, 0, -1}, {0, 0, 1}};
  for (unsigned z = 1; z < size[2]-1; z++) {
    for (unsigned y = 1; y < size[1] - 1; y++) {
      for (unsigned x = 1; x < size[0] - 1; x++) {
        float absDist = std::abs(sdf.dist(x, y, z)) * sdf.distUnit;
        if (absDist > 2 * voxSize || absDist < voxSize) {
          continue;
        }
        //check neighbors.
        for (unsigned n = 0; n < 6; n++) {
          int nx = int(x) + nbr[n][0];
          int ny = int(y) + nbr[n][1];
          int nz = int(z) + nbr[n][2];
          short nDist = sdf.dist(nx, ny, nz);
          float nAbs = std::abs(nDist) * sdf.distUnit;
          if (nAbs + voxSize < absDist) {
            std::cout << x<<" "<<y<<" "<<z<<" corner case ";
            std::cout << nx << " " << ny << " " << nz << " ";
            std::cout << absDist << " " << nAbs << "\n";
          }
        }
      }
    }
  }
}

void MarchingCubes(AdapDF * sdf, float level, TrigMesh* surf) {
  const Array3D<short>& grid = sdf->dist;
  Vec3u s = grid.GetSize();
  float unit = sdf->distUnit;
  const unsigned zAxis = 2;
  for (unsigned x = 0; x < s[0] - 1; x++) {
    for (unsigned y = 0; y < s[1] - 1; y++) {
      for (unsigned z = 0; z < s[2] - 1; z++) {
        MarchOneCube(x, y, z, grid, level / unit, surf, sdf->voxSize / unit);
      }
    }
  }
  for (unsigned i = 0; i < surf->v.size(); i += 3) {
    surf->v[i] = unit * surf->v[i] + sdf->origin[0];
    surf->v[i + 1] = unit * surf->v[i + 1] + sdf->origin[1];
    surf->v[i + 2] = unit * surf->v[i + 2] + sdf->origin[2];
  }
}

void MarchingCubes(Array3D8u & grid, float level, TrigMesh* surf,float unit, float voxSize) {
  Vec3u s = grid.GetSize();
  const unsigned zAxis = 2;
  for (unsigned x = 0; x < s[0] - 1; x++) {
    for (unsigned y = 0; y < s[1] - 1; y++) {
      for (unsigned z = 0; z < s[2] - 1; z++) {
        MarchOneCube(x, y, z, grid, level / unit, surf, voxSize / unit);
      }
    }
  }
  for (unsigned i = 0; i < surf->v.size(); i += 3) {
    surf->v[i] = unit * surf->v[i] ;
    surf->v[i + 1] = unit * surf->v[i + 1] ;
    surf->v[i + 2] = unit * surf->v[i + 2] ;
  }
}

void TestSDF() {
  TrigMesh mesh1;
  
  //std::string fileName = "F:/dolphin/meshes/fish/salmon.stl";
  //std::string fileName = "F:/dolphin/meshes/lattice_big/MeshLattice.stl";
  //mesh1.LoadStl(fileName);
  
  std::string fileName = "F:/dolphin/meshes/sdfTest/soleRigid1.obj";
  mesh1.LoadObj(fileName);
  mesh1.ComputePseudoNormals();
  
  //SavePseudoNormals(mesh1,"psnormal.obj");
  AdapSDF sdf;
  Utils::Stopwatch timer;
  //brute force dense fine sdf
  sdf.voxSize = 0.4;
  sdf.BuildTrigList(&mesh1);
  sdf.Compress();
  timer.Start();
  sdf.ComputeCoarseDist();
  //CheckCornerCase(sdf);
  CloseExterior(sdf.dist, sdf.MAX_DIST);
  float ms = timer.ElapsedMS();
  std::cout << "coarse dist time " << ms << "\n";
  timer.Start();
  Array3D8u frozen;
  FastSweep(sdf.dist, sdf.voxSize, sdf.distUnit, sdf.band, frozen);
  ms = timer.ElapsedMS();
  std::cout << "sweep time " << ms << "\n";

  sdf.ComputeFineDistBrute();

  Array2D8u slice;
  Vec3u sdfSize = sdf.dist.GetSize();
  slice.Allocate(20 * sdfSize[0], 20 * sdfSize[1]);
  float z = 15;
  timer.Start();
  GetSDFFineSlice(sdf, slice, Vec3f(0.02f, 0.02f, 0.02f), z);
   ms = timer.ElapsedMS();
  std::cout << "slice time " << ms << "\n";
  std::string sliceFile = "sliceFine" + std::to_string(int(z / 0.001)) + ".png";
  SavePng(sliceFile, slice);
  std::vector<uint8_t> dist(sdf.dist.GetData().size());

  TrigMesh marchMesh;
  MarchingCubes(&sdf, sdf.voxSize * sdf.distUnit, &marchMesh);
  marchMesh.SaveObj("marchCubes.obj");
}

void TestTrigDist() {
  TrigMesh mesh;
  mesh.LoadObj("F:/dolphin/meshes/cube_simp.obj");
  TrigFrame frame;
  unsigned tIdx = 0;
  size_t numTrigs = mesh.t.size() / 3;
  for (; tIdx < numTrigs; tIdx++) {
    Triangle trig = mesh.GetTriangleVerts(tIdx);
    mesh.ComputeTrigNormals();
    Vec3f n = mesh.GetTrigNormal(tIdx);
    ComputeTrigFrame((const float*)(trig.v), n, frame);
    std::cout << frame.x[0] << " " << frame.x[1] << " " << frame.x[2] << "\n";
    std::cout << frame.y[0] << " " << frame.y[1] << " " << frame.y[2] << "\n";
    std::cout << frame.z[0] << " " << frame.z[1] << " " << frame.z[2] << "\n";
    std::cout << frame.v1x << " " << frame.v2x << " " << frame.v2y << "\n";
    std::cout << "\n";   
  }

  tIdx = 1;
  Array2D8u image;
  image.Allocate(200, 200, 0);
  float dx = 0.01;
  float x0 = -0.5;
  Vec2u imageSize = image.GetSize();
  Triangle trig = mesh.GetTriangleVerts(tIdx);
  Vec3f n = mesh.GetTrigNormal(tIdx);
  ComputeTrigFrame((const float*)(trig.v),n, frame);
  for (unsigned row = 0; row < imageSize[1];row++) {
    for (unsigned col = 0; col < imageSize[0];col++) {
      float px = x0 + col*dx;
      float py = x0 + row * dx;
      
      TrigDistInfo info = PointTrigDist2D(px, py, frame.v1x, frame.v2x,
                                          frame.v2y);
      float dist2 = info.sqrDist;
      image(col,row) = uint8_t(info.bary[2] * 100);

    }
  }
  SavePng("bary2.png", image);
}

extern void VoxelConnector(int argc, char* argv[]);

using Face = FixedArray<float>;

struct DistanceGrid {
  Array3D<short>* dist = nullptr;
  float dx = 0.032f;
  float distUnit = 0.002f;

  DistanceGrid(Array3D<short>& grid, float gridRes, float unit)
      : dist(&grid), dx(gridRes), distUnit(unit) {}
};

void DrawSphere(DistanceGrid& grid, Vec3f center, float r) {
  Vec3f delta(0.05, 0.05, 0.05);
  Vec3f sphereMin = center - r - delta;
  Vec3f sphereMax = center + r + delta;
  Vec3i idxMin, idxMax;
  Vec3u gridSize = grid.dist->GetSize();
  for (int dim = 0; dim < 3; dim++) {
    idxMin[dim] = std::max(0, int(sphereMin[dim]/grid.dx));
    idxMax[dim] =
        std::min(int(gridSize[dim] - 1), int(sphereMax[dim] / grid.dx + 1));
  }

  for (int z = idxMin[2]; z <= idxMax[2]; z++) {
    for (int y = idxMin[1]; y <= idxMax[1]; y++) {
      for (int x = idxMin[2]; x <= idxMax[0]; x++) {
        Vec3f corner = grid.dx * Vec3f(x,y,z);
        float dist = (corner - center).norm() - r;
      }
    }
  }

}

void LoadHybrid() {
  std::string hybridFile = "F:/dolphin/meshes/hand/hand_0.5.HYBRID";
  std::ifstream in(hybridFile);
  unsigned nV, nF, nE;
  in >> nV >> nF >> nE;
  //each element occupies 3 lines in hybrid file.
  nE /= 3;
  std::cout << "vert face elts " << nV << " " << nF << " " << nE << "\n";
  
  float scale = 0.3f;

  std::vector<Vec3f> verts(nV);
  for (unsigned i = 0; i < nV; i++) {
    in >> verts[i][0] >> verts[i][1] >> verts[i][2];
  }

  for (Vec3f& v:verts) {
    v = scale * v;
  }

  std::vector<Face> faces(nF);
  for (unsigned i = 0; i < faces.size(); i++) {
    unsigned numFaceVert;
    in >> numFaceVert;
    faces[i].Allocate(numFaceVert);
    for (unsigned j = 0; j < numFaceVert; j++) {
      in >> faces[i][j];
    }
  }

  std::string visualFile = "vis.obj";

  std::ofstream out(visualFile);
  for (unsigned i = 0; i < verts.size(); i++) {
    out << "v " << verts[i][0] << " " << verts[i][1] << " " << verts[i][2]
        << "\n";
  }
  for (unsigned i = 0; i < faces.size(); i++) {
    out << "f ";
    for (unsigned j = 0; j < faces[i].size(); j++) {
      out << (faces[i][j]+1) << " ";
    }
    out << "\n";
  }

  Array3D<short> grid(512, 512, 512);
  grid.Fill(10000);
  float distUnit = 0.002;
  float dx = 0.032;
  DistanceGrid dg(grid, dx, distUnit);

  BBox bbox;
  ComputeBBox(verts, bbox);
  std::cout << bbox.vmin[0] << " " << bbox.vmin[1] << " " << bbox.vmin[2]
            << "\n";
  std::cout << bbox.vmax[0] << " " << bbox.vmax[1] << " " << bbox.vmax[2]
            << "\n";

  float margin = 1.0f;

  for (unsigned i = 0; i < verts.size(); i++) {
    verts[i] = verts[i] - bbox.vmin + Vec3f(1,1,1);  
  }
  bbox.vmax = bbox.vmax - bbox.vmin + 2.0f*Vec3f(margin,margin,margin);
  float thickness0 = 1.0f;
  float thickness1 = 0.5f;
  for (unsigned i = 0; i < verts.size(); i++) {
    verts[i] = verts[i] - bbox.vmin + Vec3f(1, 1, 1);
  }

  
  Vec3f delta(0.05, 0.05, 0.05);
  float zmax = bbox.vmax[2];
  for (unsigned i = 0; i < verts.size(); i++) {
    float z = verts[i][2];
    float alpha = z/zmax;
    float r = 0.5f*(alpha*thickness1 + (1-alpha)*thickness0);
    DrawSphere(dg, verts[i], r);
  }

}

struct LineSeg {
  Vec3f p0, p1;
  LineSeg() {}
  LineSeg(Vec3f a, Vec3f b):p0(a),p1(b) {}
};

float PtLineSegDist(Vec3f pt, Vec3f l1, Vec3f l2) {
  Vec3f pl1 = pt - l1;
  Vec3f l12 = l2 - l1;
  float eps = 1e-12;
  float len = l12.norm() + eps;
  float t = pl1.dot(l12) / len / len;
  if (t < 0) {
    return pl1.norm();
  }
  if (t > 1) {
    return (pt - l2).norm();
  }
  return (pl1 - t*l12).norm();
}

void PrintGrid(std::ostream& out, const Array3D8u& s) {
  Vec3u size = s.GetSize();
  out << size[0] << " " << size[1] << " " << size[2] << "\n";
  for (unsigned z = 0; z < size[2]; z++) {
    for (unsigned y = 0; y < size[1]; y++) {
      for (unsigned x = 0; x < size[0]; x++) {
        uint8_t val = s(x, y, z);
        out << int(val) << " ";
      }
      out<<"\n";
    }
  }
}

void MakeFluoriteLattice() {
  Array3D8u corner(64, 64, 64);
  corner.Fill(0);
  Vec3u size = corner.GetSize();
  Vec3f cellCenter(size[0] / 2, size[1] / 2, size[2] / 2);
  float beamRad = 5;
  std::vector<LineSeg> lines(2);
  lines[0] = LineSeg(Vec3f(0, 0, 0), cellCenter);
  lines[1] = LineSeg(Vec3f(size[0], size[1], 0), cellCenter);
  for (unsigned z = 0; z < size[2]; z++) {
    for (unsigned y = 0; y < size[1]; y++) {
      for (unsigned x = 0; x < size[0]; x++) {
        Vec3f pt(x + 0.5f, y + 0.5f, z + 0.5f);
        float minDist = 10 * size[0];
        for (const auto& l : lines) {
          minDist = std::min(minDist, PtLineSegDist(pt, l.p0,l.p1));
        }
        if (minDist < beamRad) {
          corner(x, y, z) = 1;
        }
      }
    }
  }
  Scale(corner, 255);
  Array3D8u s;
  MirrorCubicStructure(corner, s);

  std::vector<float> kern;
  GaussianFilter1D(1, 3, kern);

  Array3D8u s2, s4;
  Upsample2x(s, s2);

  FilterDecomp(s2, kern);
  Downsample2x(s2, s);

  std::ofstream out("fl_grid.txt");
  PrintGrid(out, s);
  for (size_t i = 0; i < s.GetData().size(); i++) {
    if (s.GetData()[i] > 100) {
      s.GetData()[i] = 2;
    } else {
      s.GetData()[i] = 1;    
    }
  }
  std::string objFile = "fl_smooth.obj";
  Vec3f voxRes(1, 1, 1);
  Vec3f origin(0,0,0);
  SaveVolAsObjMesh(objFile, s, (float*)(&voxRes), (float*)(&origin), 2);
}

float gyroid(float x, float y, float z) {
  return std::cos(x) * std::sin(y) + std::cos(y) * std::sin(z) +
         std::cos(z) * std::sin(x);
}

void MakeGyroidCell() {
  Array3D8u dist(125,125,125);
  dist.Fill(1);
  Vec3u size = dist.GetSize();
  float thickness = 10;
  float gyroidScale = (2 * 3.1415926)/size[0];
  for (unsigned z = 0; z < size[2]; z++) {
    for (unsigned y = 0; y < size[1]; y++) {
      for (unsigned x = 0; x < size[0]; x++) {
        Vec3f coord(x + 0.5f, y + 0.5f, z + 0.5f);
        coord = gyroidScale * coord;
        float f = gyroid(coord[0], coord[1], coord[2]);
        if (std::abs(f) < 0.5f) {
          dist(x, y, z) = 0;
        }
      }
    }
  }
  std::ofstream out("gyroid_grid.txt");
  PrintGrid(out, dist);
  std::string objFile = "gyroidCell.obj";
  Vec3f voxRes(1, 1, 1);
  Vec3f origin(0, 0, 0);
  SaveVolAsObjMesh(objFile, dist, (float*)(&voxRes), (float*)(&origin), 2);
}

int main(int argc, char* argv[]) {
  //VoxelConnector(argc, argv);
  //return 0;
  //TestTrigDist();
  //TestSDF();
//  MakeFluoriteLattice();
  MakeGyroidCell();
  Array3D8u eyeVol;
  std::string imageDir = "F:/dolphin/meshes/eye0531/slices/";
  int maxIndex = 760;
  LoadImageSequence(imageDir, maxIndex, eyeVol);
  for (int id = 1; id < 4; id++) {
    float voxRes[3] = {0.064, 0.0635, 0.05};
    float origin[3] = {.0f,.0f,.0f};
    SaveVolAsObjMesh("eye_" + std::to_string(id) + ".obj", eyeVol, voxRes,
                     origin, id);
  }

  int id = 5481;
  std::vector<float> kern;
  GaussianFilter1D(1, 3, kern);

  std::string structFile =
      "F:/dolphin/meshes/3DCubic64_Data/bin/" + std::to_string(id) + ".bin";
  Array3D8u s, mirrored;
  float voxRes[3] = {1, 1, 1};
  loadBinaryStructure(structFile, s);
  Array3D8u s2, s4;
  Scale(s, 255);
  SaveSlices("s_", s);

  Upsample2x(s, s2);

  // Upsample2x(s2, s4);
  FilterDecomp(s2, kern);
  Downsample2x(s2, s);

  Array3D8u v4, v8;
  ExpandCellToVerts(s, v4);

  SaveSlices("v4_", v4);
  TrigMesh surf;
  MirrorCubicStructure(v4, mirrored);
  MirrorCubicStructure(mirrored, v8);
  MirrorCubicStructure(v8, mirrored);
  PadGridConst(mirrored, v8, 0);
  InvertVal(v8);

  MarchingCubes(v8, 220, &surf,1,1);

  std::string objFile = std::to_string(id) + "_smooth.obj";
  surf.SaveObj(objFile.c_str());

  // Thresh(mirrored, 125);
  // saveObjRect(std::to_string(id) + "_smooth.obj", mirrored, voxRes, 1);

  return 0;
}

void MirrorCubicStructure(const Array3D8u& s_in, Array3D8u& s_out) {
  Array3D8u st;
  Vec3u gridSize = s_in.GetSize();
  st.Allocate(gridSize[0] * 2, gridSize[1] * 2, gridSize[2] * 2);
  gridSize = st.GetSize();
  for (unsigned i = 0; i < gridSize[0]; i++) {
    for (unsigned j = 0; j < gridSize[1]; j++) {
      for (unsigned k = 0; k < gridSize[2]; k++) {
        unsigned si = i;
        unsigned sj = j;
        unsigned sk = k;
        unsigned tmp = 0;
        if (si >= gridSize[0] / 2) {
          si = gridSize[0] - i - 1;
        }
        if (sj >= gridSize[1] / 2) {
          sj = gridSize[1] - j - 1;
        }
        if (sk >= gridSize[2] / 2) {
          sk = gridSize[2] - k - 1;
        }
        if (si < sj) {
          tmp = si;
          si = sj;
          sj = tmp;
        }
        if (si < sk) {
          tmp = sk;
          sk = si;
          si = tmp;
        }
        if (sj < sk) {
          tmp = sk;
          sk = sj;
          sj = tmp;
        }
        st(i, j, k) = s_in(si, sj, sk);
      }
    }
  }
  s_out = st;
}
