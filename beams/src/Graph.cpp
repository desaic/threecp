#include "Graph.hpp"
#include "FileUtil.hpp"
#include "TrigMesh.hpp"

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

void saveGraphSimple(std::string filename, const Graph & g)
{
  FileUtilOut out(filename);
  int dim = 3;
  out.out << g.V.size() << " " << g.E.size() << "\n";
  for (size_t i = 0; i < g.V.size(); i++) {
    for (int j = 0; j < dim; j++) {
      out.out << g.V[i][j] << " ";
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
  int pidx = params.size() - 1;
  int nParam = (int)params[pidx].size();
  int dim = 3;
  for (size_t i = 0; i < g.V.size(); i++) {
    for (int j = 0; j < dim; j++) {
      if (idx >= (int)params[pidx].size()) {
        break;
      }
      //scale graph to 2x.
      //graph param is scaled back to [0 0.5] in input.
      g.V[i][j] = 2 * params[pidx][idx];
      idx++;
    }
  }
}

void saveGraphMesh(std::string filename, const Graph & g,
  int format)
{
  TrigMesh cyl;
  FileUtilIn cyl_in("D:\\workspace\\GitHub\\gui\\data\\cylinder.obj");
  cyl.load(cyl_in.in);
  cyl_in.close();
  TrigMesh outMesh;
  //diameter of each edge.
  float thickness = 0.04;
  for (size_t i = 0; i < g.E.size(); i++) {
    TrigMesh transformed = cyl;
    Eigen::Vector3d x0 = g.V[g.E[i][0]].cast<double>();
    Eigen::Vector3d x1 = g.V[g.E[i][1]].cast<double>();
    Eigen::Vector3d axis[3];
    double xlen = (x1 - x0).norm();
    if (xlen < 1e-6) {
      std::cout << "Short edge.\n";
      continue;
    }
    if (axis[0][2] > 0.95) {
      std::cout << "debug.";
    }
    axis[0] = (x1 - x0)/xlen;
    if (std::abs(axis[0][1]) > 0.95) {
      axis[1] = Eigen::Vector3d(0, 0, 1);
    }
    else {
      axis[1] = Eigen::Vector3d(0, 1, 0);
    }
    axis[2] = axis[0].cross(axis[1]).normalized();
    axis[1] = axis[2].cross(axis[0]).normalized();
    Eigen::Matrix3d Rot;
    for (int j = 0; j < 3; j++) {
      Rot.col(j) = axis[j];
    }
    Eigen::Matrix3d Scale = Eigen::Matrix3d::Identity();
    Scale(0, 0) = xlen;
    Scale(1, 1) = thickness;
    Scale(2, 2) = thickness;
    Rot = Rot*Scale;
    for (int j = 0; j < (int)transformed.v.size(); j++) {
      transformed.v[j] = Rot*transformed.v[j] + x0;
      //for (int k = 0; k < 3; k++) {
      //  if (isnan(transformed.v[j][k])) {
      //    std::cout << "debug\n";
      //  }
      //}
    }
    int i0 = outMesh.t.size();
    int i1 = i0 + transformed.t.size();
    outMesh.append(transformed);
    Eigen::Vector3d mid = 0.5 * (x0 + x1);
    for (int j = i0; j < i1; j++) {
      Eigen::Matrix3f tcolor;
      for (int k = 0; k < 3; k++) {
        tcolor.row(k) = (0.3 + 0.9 * (1.0-mid[1])) * Eigen::Vector3f(0.3f, 0.3f, 0.8f);
      }
      outMesh.tcolor.push_back(tcolor);
    }
  }
  if (format == 0) {
    outMesh.save_obj(filename.c_str());
  }
  else if (format == 1) {
    for (unsigned int i = 0; i < outMesh.v.size(); i++) {
      outMesh.v[i][1] -= 1;
    }
    savePly(&outMesh, filename);
  }
}
