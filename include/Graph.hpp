#ifndef GRRAPH_HPP
#define GRAPH_HPP

#include "Eigen/Dense"
#include <vector>

struct Graph{
  std::vector<Eigen::Vector3f> V;
  std::vector<std::vector<int> > E;
};

void loadGraph(std::string filename, Graph & g);
#endif