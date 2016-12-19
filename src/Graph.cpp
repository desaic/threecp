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

void saveGraph(std::string filename, const Graph & g)
{
  FileUtilOut out(filename);
  int dim = 3;
  out.out << g.V.size() << " " << g.E.size() << "\n";
  for (size_t i = 0; i < g.V.size(); i++) {
    out.out << "0 " << i;
    for (int j = 0; j < dim; j++) {
      out.out <<" "<< g.V[i][j];
    }
    out.out << "\n";
  }
  for (size_t i = 0; i < g.E.size(); i++) {
    out.out << g.E[i][0] << " " << g.E[i][1] << "\n";
  }
  out.close();
}

void loadGraphParam(std::string filename, Graph & g)
{
  //list of vertex positions followed by beam parameters.
  FileUtilIn in(filename);
  if (!in.good()) {
    return;
  }
  int idx = 0;
  std::vector<std::vector<double> > params;
  in.readArr2d(params);
  in.close();
  if (params.size() == 0) {
    return;
  }
  int nParam = (int)params[0].size();
  int dim = 3;
  for (size_t i = 0; i < g.V.size(); i++) {
    for (int j = 0; j < dim; j++) {
      if (idx >= (int)params[0].size()) {
        break;
      }
      //scale graph to 2x.
      //graph param is scaled back to [0 0.5] in input.
      g.V[i][j] = 2 * params[0][idx];
      idx++;
    }
  }
}
