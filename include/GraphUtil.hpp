#pragma once
#ifndef GRAPH_UTIL_HPP
#define GRAPH_UTIL_HPP
#include "Graph.hpp"

/// \brief converts a voxel grid containing line structures to
/// a graph
void voxToGraph(const std::vector<double> & s, const std::vector<int> & gridSize,
  Graph & G);

/// \brief computes graph edges from incidence matrix.
/// uses std::map in implementation. Graph is undirected.
/// Opposite edges are merged.
void computeEdges(Graph & G);

void computeIncidence(Graph & G);

/// \brief contract vertices with degree 2.
/// removes the vertices.
/// \TODO handle graph cycles properly.
/// If the vertex to be removed is far away from the line connecting its two neighboring vertices,
/// do not remove it. i.e. have some quality measures based on 3d coordinates.
void contractVertDegree2(Graph & G);

void contractEdge(Graph & G, int vi, int vj);

void mergeCloseVerts(Graph & G, float eps);

void rmIsolatedVerts(Graph & G);
#endif