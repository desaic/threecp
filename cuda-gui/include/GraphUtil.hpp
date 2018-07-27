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
/// If some vertex along the path to be removed is very far away from the final line segment, 
/// do not remove it.
void contractVertDegree2(Graph & G, float eps);

void contractEdge(Graph & G, int vi, int vj);

void contractPath(Graph & , float eps);

/// \brief find a path made of degree 2 vertices only.
void findPath(Graph & G, std::vector<int> & path);

void mergeCloseVerts(Graph & G, float eps);

void rmIsolatedVerts(Graph & G);

void mirrorGraphCubic(Graph & G);

void separateEdges(Graph & G);

#endif