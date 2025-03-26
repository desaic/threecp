#include "TriangulateContour.h"
#include "../cdt/CDT.h"
#include <vector>
TrigMesh TriangulateContours(const float* vIn, unsigned nV,
                             const unsigned* eIn, unsigned nE) {
  std::vector<CDT::V2d<double>> verts;
  std::vector<CDT::Edge> edges;
  for (size_t i = 0; i < nV; i++) {
    verts.push_back(CDT::V2d<double>::make(vIn[i * 2], vIn[i * 2 + 1]));
  }
  for (size_t i = 0; i < nE; i++) {
    edges.push_back(CDT::Edge(eIn[2 * i], eIn[2 * i+1]));
  }
  CDT::Triangulation<double> cdt;
  cdt.insertVertices(verts);
  cdt.insertEdges(edges);
  cdt.eraseOuterTrianglesAndHoles();
  std::vector<CDT::Triangle> trigs = cdt.triangles;
  TrigMesh out;
  out.t.resize(3 * trigs.size());
  out.v.resize(3 * verts.size(), 0);
  for (size_t i = 0; i < verts.size(); i++) {
    out.v[3 * i] = verts[i].x;
    out.v[3 * i + 1] = verts[i].y;
  }
  for (size_t i = 0; i < trigs.size(); i++) {
    for (unsigned j = 0; j < 3; j++) {
      out.t[3 * i + j] = trigs[i].vertices[j];
    }
  }
  return out;
}

TrigMesh TriangulateLoop(const float* verts, unsigned nV) {
  std::vector<unsigned> edges(2*nV);
  for (size_t i = 0; i < nV; i++) {
    edges[2 * i] = i;
    edges[2 * i + 1] = (i + 1) % nV;
  }
  return TriangulateContours(verts, nV, edges.data(), nV);
}