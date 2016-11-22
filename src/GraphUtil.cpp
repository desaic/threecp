#include "GraphUtil.hpp"
#include "ArrayUtil.hpp"
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

void contractVertDegree2(Graph & G)
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
      //make new edge connecting its 2 neighbors
      int v0 = G.IV[vi][0];
      int v1 = G.IV[vi][1];
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
