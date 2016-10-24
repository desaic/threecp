#include <iostream>
#include "ArrayUtil.hpp"
#include "Element.hpp"
#include "ElementHex.hpp"
#include "ElementMeshUtil.hpp"
#include "ElementMesh.hpp"
#include "ElementRegGrid.hpp"
#include "FileUtil.hpp"
#include "TrigMesh.hpp"

typedef std::map<int, int> IntMap;

int HexFaces[6][4] = {
    { 0, 1, 3, 2 }, { 4, 6, 7, 5 },
    { 0, 4, 5, 1 }, { 2, 3, 7, 6 },
    { 0, 2, 6, 4 }, { 1, 5, 7, 3 } };
static const int sw[8][3] =
{ { -1, -1, -1 },
{ -1, -1, 1 },
{ -1, 1, -1 },
{ -1, 1, 1 },
{ 1, -1, -1 },
{ 1, -1, 1 },
{ 1, 1, -1 },
{ 1, 1, 1 }
};
struct IntGrid{

  //what indices in each cell.
  std::vector < std::vector<int> > g;

  //points added to the grid.
  std::vector<Eigen::Vector3d> pts;
  
  //grid size.
  Eigen::Vector3i s;
  
  Eigen::Vector3d origin;
  
  //grid size.
  Eigen::Vector3d dx;

  void allocate(Eigen::Vector3i size){
    s = size;
    int nCell = size[0] * size[1] * size[2];
    g.resize(nCell);
  }

  int grid2Linear(const Eigen::Vector3i & i){
    return i[0] * s[1] * s[2] + i[1] * s[2] + i[2];
  }

  Eigen::Vector3i linear2Grid(int i){
    std::vector<int> idx(3);
    idx[0] = i / (s[2] * s[1]);
    i -= idx[0] * (s[2] * s[1]);
    idx[1] = i / s[2];
    i -= idx[1] * s[2];
    idx[2] = i;
  }

  Eigen::Vector3i find(const Eigen::Vector3d & x){
    Eigen::Vector3d rel = x - origin;
    Eigen::Vector3i idx;
    for (int i = 0; i < 3; i++){
      int j = (int)(rel[i] / dx[i]);
      if (j < 0){
        j = 0;
      }
      if (j >= s[i]){
        j = s[i] - 1;
      }
      idx[i] = j;
    }
    return idx;
  }

  void addPoint(const Eigen::Vector3d & x){
    Eigen::Vector3i idx = find(x);
    int ptIdx = pts.size();
    pts.push_back(x);
    int gIdx = grid2Linear(idx);
    g[gIdx].push_back(ptIdx);
  }

  bool inBound(const Eigen::Vector3i  & idx){
    for (int i = 0; i < 3; i++){
      if (idx[i] < 0 || idx[i] >= s[i]){
        return false;
      }
    }
    return true;
  }

  ///@return -1 if no point near 1-ring.
  int findNbr(const Eigen::Vector3d & x){
    
    Eigen::Vector3i centerIdx = find(x);
    int minIdx = -1;
    double minDist = 0;
    for (int i = -1; i < 2; i++){
      for (int j = -1; j < 2; j++){
        for (int k = -1; k < 2; k++){
          Eigen::Vector3i idx;
          idx[0] = centerIdx[0] + i;
          idx[1] = centerIdx[1] + j;
          idx[2] = centerIdx[2] + k;
          if (inBound(idx)){
            int nbrIdx = grid2Linear(idx);
            for (size_t l = 0; l < g[nbrIdx].size(); l++){
              int x1idx = g[nbrIdx][l];
              Eigen::Vector3d x1 = pts[x1idx];
              double squaredDist = (x1 - x).squaredNorm();
              if (minIdx < 0 || squaredDist < minDist){
                minDist = squaredDist;
                minIdx = x1idx;
              }
            }
          }
        }
      }
    }
    return minIdx;
  }
};

int grid2Linear(const Eigen::Vector3i & i, int N)
{
  return i[0]*N*N + i[1]*N + i[2];
}

Eigen::Vector3d trilinear(Eigen::Vector3d p, std::vector<Eigen::Vector3d> & x)
{
  Eigen::Vector3d v = Eigen::Vector3d::Zero();
  for (size_t i = 0; i < x.size(); i++){
    v += (1.0f / 8) * (1 + sw[i][0] * p[0])
      *(1 + sw[i][1] * p[1]) *(1 + sw[i][2] * p[2]) * x[i];
  }
  return v;
}

void subdivideHex(ElementMesh * em, int nSub)
{
  
  int nFineV = (nSub + 1) * (nSub + 1) * (nSub + 1);
  //natural coordinate of fine vertices.
  std::vector<Eigen::Vector3d> natCoord(nFineV);
  for (int i = 0; i <= nSub; i++){
    for (int j = 0; j <= nSub; j++){
      for (int k = 0; k <= nSub; k++){
        int idx = i * (nSub + 1)* (nSub + 1) + j * (nSub + 1) + k;
        natCoord[idx] = Eigen::Vector3d(i*2.0 / nSub - 1, j*2.0 / nSub - 1, k*2.0 / nSub - 1);
      }
    }
  }
  //3d index of first vertex in a cube.
  int nFineE = nSub * nSub * nSub ;
  std::vector<Eigen::Vector3i> v0idx(nFineE);
  for (int i = 0; i < nSub; i++){
    for (int j = 0; j < nSub; j++){
      for (int k = 0; k < nSub; k++){
        int idx = i * (nSub )* (nSub ) + j * (nSub ) + k;
        v0idx[idx] = Eigen::Vector3i(i, j, k);
      }
    }
  }
  int oneV[8][3] = { { 0, 0, 0 }, { 0, 0, 1 }, { 0, 1, 0 }, { 0, 1, 1 },
  { 1, 0, 0 }, { 1, 0, 1 }, { 1, 1, 0 }, { 1, 1, 1 } };
  //local vertex indices of fine elements.
  std::vector<std::vector<int> > fineVi(nFineE);
  //loop over fine elements
  for (int i = 0; i < nFineE; i++){
    fineVi[i].resize(8);
    //loop over 8 vertices of each fine element.
    for (int j = 0; j < 8; j++){
      Eigen::Vector3i vlocalIdx;
      for (int k = 0; k < 3; k++){
        vlocalIdx[k] = oneV[j][k];
      }
      vlocalIdx += v0idx[i];
      fineVi[i][j] = grid2Linear(vlocalIdx, nSub+1);
    }
  }

  std::vector<Element * > e_new;
  IntGrid grid;
  Eigen::Vector3d mn, mx;
  BBox(em->X, mn, mx);
  grid.origin = mn;
  double cubeSize = (1.0/nSub) * (em->X[0] - em->X[1]).norm();
  double eps = cubeSize * 0.1;
  for (int i = 0; i < 3; i++){
    grid.s[i] = (int)((mx[i] - mn[i]) / cubeSize )+ 1;
    grid.dx[i] = cubeSize;
  }
  grid.allocate(grid.s);

  //make new vertices with duplicates
  for (size_t i = 0; i < em->e.size(); i++){
    std::vector<int> newvidx(natCoord.size(), -1);
    Element * ele = em->e[i];
    std::vector<Eigen::Vector3d> x(8);
    for (int j = 0; j < 8; j++){
      x[j] = em->X[ele->at(j)];
    }
    //loop over fine vertices
    for (int j = 0; j < (int)natCoord.size(); j++){
      Eigen::Vector3d vfine = Eigen::Vector3d::Zero();
      vfine = trilinear(natCoord[j], x);
      int ni = grid.findNbr(vfine);
      int vj = grid.pts.size();
      if (ni >= 0){
        Eigen::Vector3d vnear = grid.pts[ni];
        double dist = (vnear - vfine).norm();
        if (dist > eps){
          grid.addPoint(vfine);
        }
        else{
          vj = ni;
        }
      }
      else{
        grid.addPoint(vfine);
      }

      newvidx[j] = vj;
    }
    //loop over fine elements
    for (int j = 0; j < (int)fineVi.size(); j++){
      ElementHex * ele = new ElementHex();
      for (int k = 0; k < fineVi[j].size(); k++){
        (*ele)[k] = newvidx[fineVi[j][k]];
      }
      e_new.push_back(ele);
    }
  }
  ElementMesh * em_new = new ElementMesh();
  em_new->e = e_new;
  em_new->X = grid.pts;
  FileUtilOut out("fine.txt");
  em_new->initArrays();
  em_new->saveMesh(out.out);
  TrigMesh tm;
  hexToTrigMesh(em_new, &tm);
  tm.save_obj("fine.obj");
  out.close();
  saveAbq(em_new, "sphere2.abq");
  delete em_new;
}

void specialVertexBound(ElementMesh * m, float lb)
{
  m->topVerts.clear();
  if (m->dim == 2){
    //find top left element
    std::vector<std::vector< int> > grid;
    m->grid2D(grid);
    int nx = (int)grid.size();
    int ny = (int)grid[0].size();
    int topi = -1, topj = -1;
    for (int jj = ny - 1; jj >= 0; jj--){
      for (int ii = 0; ii<nx; ii++){
        if (grid[ii][jj] >= 0){
          topi = ii;
          topj = jj;
          break;
        }
      }
      if (topi>0){
        break;
      }
    }

    if (topi<0){
      std::cout << "Error initializing contact verts. Empty grid\n";
      return;
    }

    int eidx = grid[topi][topj];
    //top left vertex
    m->topVerts.push_back(m->e[eidx]->at(1));
    int ii = topi + 1;
    //find 1 element past right most element
    for (; ii<nx; ii++){
      if (grid[ii][topj]<0){
        break;
      }
    }
    //top right vertex
    eidx = grid[ii - 1][topj];
    m->topVerts.push_back(m->e[eidx]->at(3));

    int jj = topj - 1;
    //find downward neighbor of top left element until break.
    for (; jj >= 0; jj--){
      if (grid[topi][jj]<0){
        break;
      }
    }

    eidx = grid[topi][jj + 1];

    m->specialVert = m->e[eidx]->at(0);
    std::cout << "special vert " << m->specialVert << "\n";
    std::cout << lb << "\n";
    m->lb[m->specialVert][1] = lb;
  }
  else if (m->dim == 3){
    //find top left front element
    std::vector<std::vector< std::vector<int> > > grid;
    m->grid3D(grid);
    int nx = (int)grid.size();
    int ny = (int)grid[0].size();
    int nz = (int)grid[0][0].size();
    int topi = -1, topj = -1, topk = -1;
    for (int jj = ny - 1; jj >= 0; jj--){
      for (int ii = 0; ii<nx; ii++){
        for (int kk = 0; kk<nz; kk++){
          if (grid[ii][jj][kk] >= 0){
            topi = ii;
            topj = jj;
            topk = kk;
            break;
          }
        }
        if (topi >= 0){
          break;
        }
      }
      if (topi >= 0){
        break;
      }
    }

    if (topi<0){
      std::cout << "Error initializing contact verts. Empty grid\n";
      return;
    }

    int eidx = grid[topi][topj][topk];
    //top left vertex
    m->topVerts.push_back(m->e[eidx]->at(2));
    int ii = topi + 1;
    //find 1 element past left most element
    for (; ii<nx; ii++){
      if (grid[ii][topj][topk]<0){
        break;
      }
    }
    //find top right vertex used for measuring top angle
    eidx = grid[ii - 1][topj][topk];
    m->topVerts.push_back(m->e[eidx]->at(6));

    int jj = topj - 1;
    //find downward neighbor of top left element until break.
    for (; jj >= 0; jj--){
      if (grid[topi][jj][topk]<0){
        break;
      }
    }

    eidx = grid[topi][jj + 1][topk];
    m->specialVert = m->e[eidx]->at(0);
    std::cout << "special vert " << m->specialVert << "\n";
    std::cout << lb << "\n";
    for (int kk = topk; kk<nz; kk++){
      eidx = grid[topi][jj + 1][kk];
      if (eidx<0){
        continue;
      }
      m->lb[m->e[eidx]->at(0)][1] = lb;
      m->lb[m->e[eidx]->at(1)][1] = lb;
    }
  }
}

void updateSurfVert(const std::vector<Eigen::Vector3d> & x, TrigMesh * tm, const std::vector<int> & vidx)
{
  for (unsigned int ii = 0; ii<vidx.size(); ii++){
    tm->v[ii] = x[vidx[ii]];
  }
}

//find neighboring elements of a vertex
void elementNeighbors(ElementMesh * em,
  std::vector<std::vector< int> > & eleNeighbor)
{
  eleNeighbor.clear();
  eleNeighbor.resize(em->X.size());
  for (unsigned int ii = 0; ii<em->e.size(); ii++){
    for (int jj = 0; jj<em->e[ii]->nV(); jj++){
      eleNeighbor[em->e[ii]->at(jj)].push_back(ii);
    }
  }
}

int findHexFace(const IntMap & m, int ei, ElementMesh * em){
  for (int fi = 0; fi<6; fi++){
    int cnt = 0;
    for (int fv = 0; fv<4; fv++){
      if (m.at(em->e[ei]->at(HexFaces[fi][fv])) == 2){
        cnt++;
      }
    }
    if (cnt == 4){
      return fi;
    }
  }
  return 0;
}

void hexToTrigMesh(ElementMesh * em, TrigMesh * surf)
{
  std::vector<std::vector<bool> > exterior(em->e.size());
  for (unsigned int ii = 0; ii<exterior.size(); ii++){
    unsigned int nV = em->e[ii]->nV();
    exterior[ii].resize(nV, true);
  }

  //mark exterior faces
  std::vector<std::vector<int > > eleNeighbor;
  elementNeighbors(em, eleNeighbor);
  for (unsigned int ii = 0; ii<em->X.size(); ii++){
    for (unsigned int nj = 0; nj<eleNeighbor[ii].size(); nj++){
      int jj = eleNeighbor[ii][nj];
      for (unsigned int nk = nj + 1; nk<eleNeighbor[ii].size(); nk++){
        int kk = eleNeighbor[ii][nk];
        IntMap im;
        for (int vv = 0; vv<em->e[jj]->nV(); vv++){
          im[em->e[jj]->at(vv)] = 1;
        }
        for (int vv = 0; vv<em->e[kk]->nV(); vv++){
          int key = em->e[kk]->at(vv);
          auto entry = im.find(key);
          if (entry != im.end()){
            im[key] = 2;
          }
          else{
            im[key] = 1;
          }
        }
        if (im.size() == 12){
          //share a face
          int fi = findHexFace(im, jj, em);
          exterior[jj][fi] = false;
          fi = findHexFace(im, kk, em);
          exterior[kk][fi] = false;
        }
      }
    }
  }
  //save exterior vertices and exterior faces
  std::vector<int> e2surf(em->X.size(), -1);
  int vCnt = 0;
  //save vertices
  surf->v.clear();
  surf->t.clear();
  surf->vidx.clear();
  for (unsigned int ii = 0; ii<em->e.size(); ii++){
    for (int fi = 0; fi<6; fi++){
      if (!exterior[ii][fi]){
        continue;
      }
      for (int fv = 0; fv<4; fv++){
        int vi = em->e[ii]->at(HexFaces[fi][fv]);
        if (e2surf[vi]<0){
          e2surf[vi] = vCnt;
          surf->v.push_back(em->x[vi]);
          surf->vidx.push_back(vi);
          vCnt++;
        }
      }
    }
  }
  //save faces
  for (unsigned int ii = 0; ii<em->e.size(); ii++){
    for (int fi = 0; fi<6; fi++){
      if (!exterior[ii][fi]){
        continue;
      }
      int trigs[2][3] = { { 0, 1, 2 }, { 2, 3, 0 } };
      for (int ti = 0; ti<2; ti++){
        Eigen::Vector3i newTrig;
        for (int tv = 0; tv<3; tv++){
          int vi = em->e[ii]->at(HexFaces[fi][trigs[ti][tv]]);
          newTrig[tv] = e2surf[vi];
        }
        surf->t.push_back(newTrig);
        surf->ei.push_back(ii);
        surf->fi.push_back(fi);
      }
    }
  }
}

void saveSimState(const ElementMesh * em, std::ostream & out)
{
  save_arr(em->x, out);
  save_arr(em->v, out);
  save_arr(em->fe, out);
  save_arr(em->fc, out);
}

void loadSimState(ElementMesh * em, std::istream & in, bool loadDisplacement)
{
  load_arr(em->x, in);
  if (loadDisplacement){
    for (size_t i = 0; i < em->x.size(); i++){
      em->x[i] += em->X[i];
    }
  }
  load_arr(em->v, in);
  load_arr(em->fe, in);
  em->fc.resize(em->fe.size());
  load_arr(em->fc, in);
}

void saveAbq(ElementMesh * em, std::string filename, float scale)
{
  FileUtilOut out(filename);
  out.out << "*NODE\n";
  for (unsigned int ii = 0; ii<em->X.size(); ii++){
    Eigen::Vector3d v = em->X[ii];
    v *= scale;
    out.out << (ii + 1) << ", " << v[0] << ", " << v[1] << ", " << v[2] << "\n";
  }
  out.out << "*ELEMENT, TYPE=C3D8\n";
  int abqOrder[8] = { 0, 2, 6, 4, 1, 3, 7, 5 };
  for (unsigned int ii = 0; ii<em->e.size(); ii++){
    out.out << (ii + 1);
    for (int jj = 0; jj<em->e[ii]->nV(); jj++){
      out.out << ", " << (em->e[ii]->at(abqOrder[jj]) + 1);
    }
    out.out << "\n";
  }
  out.out << "*ELSET,ELSET=EALL,GENERATE\n";
  out.out << "  1," << (em->e.size()) << "\n";
  out.close();
}

///@brief moves rest pose X to origin
///and sets x=X.
void placeAtOrigin(ElementMesh * em)
{
  Eigen::Vector3d mn, mx;
  BBox(em->X, mn, mx);
  for (size_t i = 0; i < em->x.size(); i++){
    em->X[i] -= mn;
  }
  em->x = em->X;
}

void translate(ElementMesh * em, const Eigen::Vector3d & vec)
{
  for (size_t i = 0;i< em->x.size(); i++){
    em->x[i] += vec;
  }
}

///@param O new corner (0,0,0) of bounding box.
///@param Frame. Each column is an axis in world space.
void applyFrame(ElementMesh * em, const Eigen::Vector3d & O,
  const Eigen::Matrix3d & Frame)
{
  Eigen::Vector3d mn, mx;
  BBox(em->x, mn, mx);
  for (size_t i = 0; i < em->x.size(); i++){
    em->x[i] += O - mn;
    em->x[i] = Frame * (em->x[i] - O) + O;
  }
}

bool hitWall(float wallDist, const std::vector<Eigen::Vector3d> & x, int dim, int sign)
{
  bool intersect = false;
  for (size_t i = 0; i < x.size(); i++){
    if (sign*(wallDist - x[i][dim]) < 0){
      intersect = true;
      break;
    }
  }
  return intersect;
}

void assignGridMat(const std::vector<double> & s,
  const std::vector<int> & gridSize, ElementRegGrid * grid)
{
  double thresh = 5e-2;
  grid->resize(gridSize[0], gridSize[1], gridSize[1]);
  std::vector<Element* > e;
  std::vector<double> color;
  for (size_t i = 0; i < s.size(); i++) {
    if (s[i] < thresh) {
      delete grid->e[i];
    }
    else {
      e.push_back(grid->e[i]);
      color.push_back(s[i]);
    }
  }
  grid->color = color;
  grid->e = e;
  grid->rmEmptyVert();
}
