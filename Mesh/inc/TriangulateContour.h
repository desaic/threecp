#ifndef TRIANGULATE_CONTOUR
#define TRIANGULATE_CONTOUR
#pragma once
#include "TrigMesh.h"

TrigMesh TriangulateContours(const float* verts, unsigned nV,
                             const unsigned* edges, unsigned nE);
//triangulate one simple loop
TrigMesh TriangulateLoop(const float* verts, unsigned nV);
#endif