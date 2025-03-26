#ifndef GRAPH_HPP
#define GRAPH_HPP

#include "Eigen/Dense"
#include <vector>
#include <map>

typedef std::map<std::pair<int, int>, int> EdgeMap;

struct Graph{
  std::vector<int> VLabel;
  std::vector<Eigen::Vector3f> V;
  //list of edges.
  std::vector<std::vector<int> > E;
  //list of incident vertices.
  std::vector<std::vector<int> > IV;
};

void loadGraph(std::string filename, Graph & g);

///\brief writes plain text of vertex positions and
///edges.
void saveGraph(std::string filename, const Graph & g);

///\brief save a simpler format.
void saveGraphSimple(std::string filename, const Graph & g);

void loadGraphParam(std::string filename, Graph & g);

///\brief save graph as a mesh with cylinders.
///\param format 0 for obj 1 for ply
void saveGraphMesh(std::string filename, const Graph & g, int format = 0);
#endif