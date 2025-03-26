#include "ObjLoader.h"
#include "TriangulateContour.h"
#include "Vec2.h"
#include "Vec3.h"
#include <fstream>
#include <iostream>
#include <sstream>

/// @brief 
/// @param v 
/// @param polygon 
/// @return list of triplets of indices into polygon array
/// for triangles.
std::vector<unsigned> SplitIntoTriangles(const std::vector<Vec3f>& v) {
  if (v.size() <= 9) { return {0,1,2}; }
  unsigned numVerts = v.size();
  unsigned numTrigs = numVerts - 2;
  std::vector<unsigned> trigs(3 * numTrigs);

  // compute normal;
  // C: a random center vertex on the same plane.
  Vec3f C = v[0];
  for (size_t vi = 0; vi < v.size(); vi ++) {    
    C += v[vi];
  }
  C = (1.0f / numVerts) * C;
  Vec3f N(0, 0, 0);
  for (size_t vi = 0; vi < v.size(); vi++) {
    Vec3f vert0 = v[vi];
    Vec3f vert1;
    if (vi > v.size() - 2) {
      vert1 = v[0];
    }
    else {
      vert1 = v[vi + 1];
    }
    N += (vert0 - C).cross(vert1 - C);
  }
  float norm = N.norm();
  if (norm > 0) {
    N = (1.0f / norm) * N;

    std::vector<Vec2f> verts;
    
    //pick two random axis perp to the normal vector;
    Vec3f xAxis(1, 0, 0);
    if (std::abs(xAxis.dot(N)) > 0.99) {
      xAxis = Vec3f(0, 1, 0);
    }
    Vec3f yAxis = N.cross(xAxis);
    yAxis.normalize();
    xAxis = yAxis.cross(N);

    //project to 2d points in the plane
    for (size_t vi = 0; vi < v.size(); vi++) {
      Vec3f local = v[vi] - C;
      verts[vi] = Vec2f(local.dot(xAxis), local.dot(yAxis));
    }
    TrigMesh patch = TriangulateLoop((const float*)verts.data(), verts.size());
    return patch.t;
  }
  else {
    //whatever
    for (unsigned i = 0; i < numTrigs; i++) {
      trigs.push_back(0);
      trigs.push_back(i + 1);
      trigs.push_back(i + 2);
    }
    return trigs;
  }
  return trigs;
}

std::vector<Vec3f> CopyVertices(const std::vector<float>& meshv,
  const std::vector<unsigned>& indices) {
  std::vector<Vec3f> v(indices.size()); 
  for (size_t i = 0; i < indices.size(); i++) {
    size_t vi = 3 * indices[i];
    v[i] = Vec3f(meshv[vi], meshv[vi + 1], meshv[vi + 2]);
  }
  return v;
}

int LoadOBJFile(std::string filename, std::vector<uint32_t>& t,
                std::vector<float>& v, std::vector<Vec2f>& uv) {
  std::ifstream file(filename);

  if (!file.is_open()) {
    return -1;
  }
  t.clear();
  v.clear();
  uv.clear();
  // vertex list in uv plane.
  std::vector<Vec2f> uv_verts;
  // uv vertex index for each triangle
  std::vector<int> uv_t;
  std::string curline;

  while (std::getline(file, curline)) {
    auto iss = std::istringstream(curline);
    std::string first_token;
    iss >> first_token;
    if (first_token == "v") {
      for (int i = 0; i < 3; i++) {
        float vf;
        iss >> vf;
        v.push_back(vf);
      }
    } else if (first_token == "f") {
      std::vector<unsigned> indices;
      std::vector<unsigned> uvi;
      std::string idxs;
      while (iss >> idxs) {
        std::istringstream idxss(idxs);

        std::string idx;
        std::getline(idxss, idx, '/');
        indices.push_back(std::stoi(idx) - 1);

        // uv indices
        idx.clear();
        std::getline(idxss, idx, '/');
        if (!idx.empty()) {
          uvi.push_back(std::stoi(idx) - 1);
        }
      }
      if (indices.size() < 3) {
        continue;
      }
      if (indices.size() == 3) {
        t.insert(t.end(), indices.begin(), indices.end());
        uv_t.insert(uv_t.end(), uvi.begin(), uvi.end());
      }
      else {
        std::vector<Vec3f> vCopy = CopyVertices(v, indices);
        std::vector<unsigned>localIndices = SplitIntoTriangles(vCopy);
        std::vector<unsigned> newIndices(localIndices.size());
        for (size_t i = 0; i < localIndices.size(); i++) {
          newIndices[i] = indices[localIndices[i]];
        }
        t.insert(t.end(), newIndices.begin(), newIndices.end());

        if (!uvi.empty()) {
          std::vector<unsigned> uvCopy;
          for (size_t i = 0; i < localIndices.size(); i++) {
            uvCopy.push_back(uvi[localIndices[i]]);
          }
          uv_t.insert(uv_t.end(), uvCopy.begin(), uvCopy.end());
        }
      }

    } else if (first_token == "vt") {
      Vec2f vt;
      iss >> vt[0] >> vt[1];
      uv_verts.push_back(vt);
    }
  }

  // order uvs properly
  for (int uv_idx : uv_t) {
    uv.push_back(uv_verts[uv_idx]);
  }
  return 0;
}