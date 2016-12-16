#include "GraphUtil.hpp"

#include "ArrayUtil.hpp"
#include "EigenUtil.hpp"
#include <iterator>
#include <map>
#include <set>

void addNeighbors(const std::vector<double> & s, const std::vector<int> & gridSize,
  const std::map<int, int> & graphIdx, int lidx, int radius,
  Graph & G)
{
  std::vector<int> gidx;
  linearToGridIdx(lidx, gridSize, gidx);
  std::map<int, int>::const_iterator it = graphIdx.find(lidx);
  if (it == graphIdx.end()) {
    return;
  }
  int vidx = it->second;

  for (int i = -radius; i <= radius; i++) {
    for (int j = -radius; j <= radius; j++) {
      for (int k = -radius; k <= radius; k++) {
        std::vector<int> nidx = {i + gidx[0], j + gidx[1], k+gidx[2] };
        if (!inbound(nidx[0], nidx[1], nidx[2], gridSize)) {
          continue;
        }
        int nlidx = gridToLinearIdx(nidx[0], nidx[1], nidx[2], gridSize);
        it = graphIdx.find(nlidx);
        if (it == graphIdx.end()) {
          continue;
        }
        int nvidx = it->second;
        if (nvidx != vidx) {
          G.IV[vidx].push_back(nvidx);
        }
      }
    }
  }
}

void voxToGraph(const std::vector<double> & s, const std::vector<int> & gridSize,
  Graph & G)
{
  //voxels within radius of 2 are marked as neighbors.
  int nbrRadius = 2;
  float thresh = 0.5;
  //map from linear idx to graph vertex idx.
  std::map<int, int> graphIdx;
  Eigen::Vector3f dx (1.0f / gridSize[0], 1.0f / gridSize[1], 1.0f / gridSize[2]);
  int vCnt = 0;
  for (int i = 0; i < gridSize[0]; i++) {
    for (int j = 0; j < gridSize[1]; j++) {
      for (int k = 0; k < gridSize[2]; k++) {
        int lidx = gridToLinearIdx(i, j, k, gridSize);
        if (s[lidx] > thresh) {
          Eigen::Vector3f coord(i + 0.5f, j + 0.5f, k + 0.5f);
          coord = (coord.array() * dx.array());
          G.V.push_back(coord);
          graphIdx[lidx] = vCnt;
          vCnt++;
        }
      }
    }
  }
  G.IV.resize(G.V.size());
  std::map<int, int>::const_iterator it;
  for (it = graphIdx.begin(); it != graphIdx.end(); it++) {
    addNeighbors(s, gridSize, graphIdx, it->first, 1, G);
  }
  computeEdges(G);
}

float ptLineDist(Eigen::Vector3f pt, Eigen::Vector3f x0, Eigen::Vector3f x1,
  float & t)
{
  Eigen::Vector3f v0 = pt - x0;
  Eigen::Vector3f dir = (x1 - x0);
  double len = dir.norm();
  dir = (1.0 / len)*dir;
  //component of v0 parallel to the line segment.
  t = v0.dot(dir);
  Eigen::Vector3f a = t * dir;
  t = t / len;
  Eigen::Vector3f b = v0 - a;
  double dist = 0;

  if (t < 0) {
    dist = (pt - x0).norm();
  }
  else if (t > 1) {
    dist = (pt - x1).norm();
  }
  else {
    dist = b.norm();
  }

  return dist;
}

void contractVertDegree2(Graph & G, float eps)
{
  if (G.IV.size() == 0) {
    computeIncidence(G);
  }
  while (1) {
    //set of verts with degree 2
    std::set<int> deg2Set;
    for (size_t i = 0; i < G.IV.size(); i++) {
      if (G.IV[i].size() == 2) {
        deg2Set.insert(i);
      }
    }
    if (deg2Set.size() == 0) {
      break;
    }
    for(std::set<int>::iterator it = deg2Set.begin(); it!=deg2Set.end(); it++){
      int vi = (*it);
      if (G.IV[vi].size() != 2) {
        continue;
      }
      int v0 = G.IV[vi][0];
      int v1 = G.IV[vi][1];
      //float t = 0;
      //float dist = ptLineDist(G.V[vi], G.V[v0], G.V[v1], t);
      ////check new edge is not too far away from the vertex.
      //if (dist > eps) {
      //  continue;
      //}
      //make new edge connecting its 2 neighbors

      if (!contains(G.IV[v0], v1)) {
        G.IV[v0].push_back(v1);
        G.IV[v1].push_back(v0);
      }
      //delete its own two edges
      remove(G.IV[v0], vi);
      remove(G.IV[v1], vi);
      G.IV[vi].clear();
    }
  }
  //remove unused vertices.
  //remap vertex indices in edges.
  rmIsolatedVerts(G);
  computeEdges(G);
}

void contractEdge(Graph & G, int vi, int vj)
{
  Eigen::Vector3f newV = 0.5*(G.V[vi] + G.V[vj]);
  int newIdx = G.V.size();
  G.V.push_back(newV);
  G.IV.push_back(std::vector<int>(0));
  //new vertex' neighbor is the union of vi and vj's neighbors.
  std::set<int> nbr;
  std::copy(G.IV[vi].begin(), G.IV[vi].end(), std::inserter(nbr, nbr.end()));
  std::copy(G.IV[vj].begin(), G.IV[vj].end(), std::inserter(nbr, nbr.end()));
  nbr.erase(vi);
  nbr.erase(vj);
  G.IV[vi].clear();
  G.IV[vj].clear();
  std::copy(nbr.begin(), nbr.end(), std::back_inserter(G.IV[newIdx]));
  for (size_t i = 0; i < G.IV[newIdx].size(); i++) {
    int ni = G.IV[newIdx][i];
    remove(G.IV[ni], vi);
    remove(G.IV[ni], vj);
    if (!contains(G.IV[ni], newIdx)) {
      G.IV[ni].push_back(newIdx);
    }
  }
}

void contractPath(Graph & G, float eps)
{
  std::vector<int> path;
  G.VLabel.clear();
  G.VLabel.resize(G.V.size(), 0);
  while (1) {
    findPath(G, path);
    if (path.size() <= 2) {
      std::cout << "Path too short in contract Path.\n";
      break;
    }
    std::vector<int> newPath;
    int i0 = path[0];
    int i1 = path[path.size() - 1];
    Eigen::Vector3f v0 = G.V[i0];
    Eigen::Vector3f v1 = G.V[i1];
    float maxDist = 0;
    int maxIdx = -1;
    float t = 0;

    for (size_t i = 1; i < path.size() - 1; i++) {
      float dist = ptLineDist(G.V[path[i]], v0, v1, t);
      int vidx = path[i];
      if (dist > maxDist) {
        maxDist = dist;
        maxIdx = vidx;
      }
      G.IV[vidx].clear();
      if (i == 1) {
        remove(G.IV[i0], vidx);
      }
      if (i == (int)path.size() - 2) {
        remove(G.IV[i1], vidx);
      }
    }
    newPath.push_back(i0);
    if (maxDist > eps) {
      newPath.push_back(maxIdx);
    }
    newPath.push_back(i1);

    for (int i = 1; i < newPath.size(); i++) {
      int prev = newPath[i - 1];
      int vidx = newPath[i];
      addUnique(G.IV[prev],vidx);
      addUnique(G.IV[vidx], prev);
      G.VLabel[vidx] = 1;
    }
    //break;
  }

  rmIsolatedVerts(G);
  computeEdges(G);
}

// includes end points with deg !=2 or endpoint = i
void findPathAlong(Graph & G, int v0, int v1, std::vector<int>& path)
{
  int v2;
  int vstart = v0;
  while (1) {
    path.push_back(v1);
    if (G.IV[v1].size() != 2) {
      break;
    }
    //find v2, neighbor of v1 that's not v0.
    if (G.IV[v1][0] == v0) {
      v2 = G.IV[v1][1];
    }
    else {
      v2 = G.IV[v1][0];
    }
    if (v2!=vstart) {
      v0 = v1;
      v1 = v2;
    }
    else {
      //reached a junction or an endpoint.
      break;
    }
  }
}

void findPath(Graph & G, std::vector<int>& path)
{
  path.clear();
  if (G.IV.size() == 0) {
    computeIncidence(G);
  }
  int i = 0;
  for (; i < (int)G.V.size(); i++) {
    if (G.VLabel.size() == G.V.size() && G.VLabel[i] != 0) {
      continue;
    }
    if (G.IV[i].size() == 2) {
      break;
    }
  }
  if (i == (int)G.V.size()) {
    return;
  }
  std::vector<int> path0, path1;
  int v1 = G.IV[i][0];
  int v2 = G.IV[i][1];
  findPathAlong(G, i, v1, path0);
  std::reverse(path0.begin(), path0.end());
  path.insert(path.end(), path0.begin(), path0.end());
  path.push_back(i);
  findPathAlong(G, i, v2, path1);
  path.insert(path.end(), path1.begin(), path1.end());
}

void mergeCloseVerts(Graph & G, float eps)
{
  if (G.IV.size() == 0) {
    computeIncidence(G);
  }
  while (1) {
    //candidates for merging
    std::set<int> candidates;
    for (size_t i = 0; i < G.IV.size(); i++) {
      for (int j = 0; j<G.IV[i].size(); j++) {
        int vj = G.IV[i][j];
        float dist = (G.V[i] - G.V[vj]).norm();
        if (dist < eps) {
          candidates.insert(i);
        }
      }
    }
    if (candidates.size() == 0) {
      break;
    }
    for (std::set<int>::iterator it = candidates.begin(); it != candidates.end(); it++) {
      int vi = (*it);
      for (int j = 0; j < G.IV[vi].size(); j++) {
        int vj = G.IV[vi][j];
        float dist = (G.V[vi] - G.V[vj]).norm();
        if (dist < eps) {
          contractEdge(G, vi, vj);
        }
      }
    }
  }
  rmIsolatedVerts(G);
  computeEdges(G);
}

void rmIsolatedVerts(Graph & G)
{
  std::vector<int> Vnew(G.V.size());
  int vCnt = 0;
  for (size_t i = 0; i < G.IV.size(); i++) {
    if (G.IV[i].size() > 0) {
      Vnew[i] = vCnt;
      vCnt++;
    }
    else {
      Vnew[i] = -1;
    }
  }
  std::vector<Eigen::Vector3f> newPos(vCnt);
  std::vector<std::vector<int> > newIV(vCnt);
  vCnt = 0;
  for (size_t i = 0; i < G.IV.size(); i++) {
    int newIdx = Vnew[i];
    if (newIdx < 0) {
      continue;
    }
    for (size_t j = 0; j < G.IV[i].size(); j++) {
      int v = Vnew[G.IV[i][j]];
      if (v < 0) {
        std::cout << "rmIsolatedVerts negative neighbor " << i << " " << j << "\n";
      }
      newIV[newIdx].push_back(v);
    }
    newPos[vCnt] = G.V[i];
    vCnt++;
  }
  G.V = newPos;
  G.IV = newIV;
}

void computeEdges(Graph & G)
{
  EdgeMap edges;
  G.E.clear();
  for (size_t i = 0; i<G.IV.size(); i++) {
    for (size_t j = 0; j < G.IV[i].size(); j++) {
      int v[2];
      v[0] = i;
      v[1] = G.IV[i][j];
      std::sort(std::begin(v), std::end(v));
      std::pair<int, int> e = std::make_pair(v[0], v[1]);
      edges[e]=1;
    }
  }
  for (EdgeMap::const_iterator it = edges.begin(); it != edges.end(); it++) {
    std::vector<int> e(2, 0);
    e[0] = it->first.first;
    e[1] = it->first.second;
    G.E.push_back(e);
  }
}

void computeIncidence(Graph & G)
{
  G.IV.resize(G.V.size());
  for (size_t i = 0; i < G.IV.size(); i++) {
    G.IV[i].clear();
  }
  for (size_t i = 0; i < G.E.size(); i++) {
    G.IV[G.E[i][0]].push_back(G.E[i][1]);
    G.IV[G.E[i][1]].push_back(G.E[i][0]);
  }
}

std::vector<float> permute(const std::vector<float> & a, int p)
{
  int N = (int)a.size();
  std::vector<int> idx(N, 0);
  std::vector<float> ret(N, 0);
  int base = N;
  for (size_t i = 0; i < N; i++) {
    idx[i] = p % base;
    p = p / base;
  }
  std::vector<bool> taken(N, false);
  for (int i = 0; i < N; i++) {
    int cnt = 0;
    for (int j = 0; j < N; j++) {
      if (taken[j]) {
        continue;
      }
      if (cnt == idx[i]) {
        ret[j] = a[i];
        taken[j] = true;
        break;
      }
      cnt++;
    }
  }
  return ret;
}

std::vector<float> mirrorAxis(std::vector<float> & a, int c)
{
  int N = (int)a.size();
  std::vector<int> idx(N, 0);
  std::vector<float> ret(N, 0);
  int base = 2;
  for (int i = 0; i < N; i++) {
    idx[i] = c % base;
    c = c / base;
  }
  for (int i = 0; i < N; i++) {
    if (idx[i] > 0) {
      ret[i] = 1 - a[i];
    }
    else {
      ret[i] = a[i];
    }
  }
  return ret;
}

Eigen::Vector3f toVector3f(const std::vector<float> & a)
{
  Eigen::Vector3f e;
  for (int i = 0; i < 3; i++) {
    e[i] = a[i];
  }
  return e;
}

void mirrorGraphCubic(Graph & G)
{
  //mirror each edge 6 times
  int nPerm = 6;
  int dim = 3;
  int nEdges = (int)G.E.size();
  //the first permutation is the original graph. Skip it.
  for (int i = 0; i < nEdges; i++) {
    std::vector<float> v1(dim), v2(dim);
    for (int p = 1; p < nPerm; p++) {
      eigen2vector(G.V[G.E[i][0]], v1);
      eigen2vector(G.V[G.E[i][1]], v2);
      v1 = permute(v1, p);
      v2 = permute(v2, p);
      int vidx = G.V.size();
      G.V.push_back(toVector3f(v1));
      G.V.push_back(toVector3f(v2));
      std::vector<int> edge(2, 0);
      edge[0] = vidx;
      edge[1] = vidx + 1;
      G.E.push_back(edge);
    }
  }
  
  //mirror new edges 8 times.
  nEdges = (int)G.E.size();
  int nComb = 8;
  for (int i = 0; i < nEdges; i++) {
    std::vector<float> v1(dim), v2(dim);
    for (int p = 1; p < nComb; p++) {
      eigen2vector(G.V[G.E[i][0]], v1);
      eigen2vector(G.V[G.E[i][1]], v2);
      v1 = mirrorAxis(v1, p);
      v2 = mirrorAxis(v2, p);
      int vidx = G.V.size();
      G.V.push_back(toVector3f(v1));
      G.V.push_back(toVector3f(v2));
      std::vector<int> edge(2, 0);
      edge[0] = vidx;
      edge[1] = vidx + 1;
      G.E.push_back(edge);
    }
  }

}
