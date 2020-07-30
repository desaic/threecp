#ifndef GRAPH_HPP
#define GRAPH_HPP

#include "Eigen/Dense"
#include <vector>
#include <map>

typedef std::map<std::pair<int, int>, int> EdgeMap;
struct Cuboid
{
    float x0[3];
    float x1[3];
    float r[2];
    float theta;
    bool withCaps;
    Cuboid() {
        for (int i = 0; i < 3; i++) {
            x0[i] = 0;
            x1[i] = 0;
        }
        r[0] = 0;
        r[1] = 0;
        theta = 0;
        withCaps = 1;
    }
};
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