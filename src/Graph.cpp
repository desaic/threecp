#include "Graph.hpp"
#include "FileUtil.hpp"

void loadGraph(std::string filename, Graph & g)
{
  int nV, nE;
  FileUtilIn in(filename);
  if (!in.good()) {
    return;
  }
  in.in >> nV >> nE;
  g.V.resize(nV);
  g.E.resize(nE);
  for (int i = 0; i < nV; i++){
    int vi = 0;
    int ei = 0;
    in.in >> ei >> vi;
    for (int j = 0; j < 3; j++){
      in.in >> g.V[vi][j];
    }
    g.V[vi] = g.V[vi];
  }
  for (int i = 0; i < nE; i++){
    g.E[i].resize(2);
    in.in >> g.E[i][0] >> g.E[i][1];
  }
  in.close();
}