#include "ElementMesh.hpp"
#include "Element.hpp"
#include "Eigen/Sparse"

#include "Timer.hpp"

#include <iostream>
#include <fstream>

Eigen::SparseMatrix<double>
ElementMesh::getStiffnessSparse()
{
  int N = dim * (int)x.size();
  Timer timer;
  if (Kpattern.size() == 0){
    std::vector<int> I, J;
    stiffnessPattern(I, J);
  }

  int nV = e[0]->nV();
  if (sblocks.size() == 0){
    sblocks.resize(x.size());
    for (unsigned int ii = 0; ii < e.size(); ii++){
      Element * ele = e[ii];
      nV = ele->nV();
      for (int jj = 0; jj < nV; jj++){
        int vj = ele->at(jj);
        for (int kk = 0; kk < nV; kk++){
          int vk = ele->at(kk);
          auto it = sblocks[vj].find(vk);
          if (it == sblocks[vj].end()){
            sblocks[vj][vk] = Eigen::MatrixXd::Zero(dim, dim);
          }
        }
      }
    }
  }
  for (unsigned int vi = 0; vi < x.size(); vi++){
    for (auto it = sblocks[vi].begin(); it != sblocks[vi].end(); it++){
      it->second = Eigen::MatrixXd::Zero(dim,dim);
    }
  }

  double localStiffTime = 0;
  double assembleTime = 0;
  
  for(unsigned int ii = 0;ii<e.size();ii++){
    timer.startWall();
    Element * ele = e[ii];
    nV = ele->nV();
    Eigen::MatrixXd K  = getStiffness(ii);
    timer.endWall();
    localStiffTime += timer.getSecondsWall();
    timer.startWall();
    for(int jj = 0; jj<nV; jj++){
      int vj = ele->at(jj);
      for(int kk = 0; kk<nV; kk++){
        int vk = ele->at(kk);
        auto it = sblocks[vj].find(vk);
        it->second += K.block(dim * jj, dim * kk, dim, dim);
      }
    }
    timer.endWall();
    assembleTime += timer.getSecondsWall();
  }
  //std::cout << "local stiffness time " << localStiffTime << "\n";
  //std::cout << "assemble time " << assembleTime << "\n";
  timer.startWall();
  int cnt = 0;
  for(unsigned int vi = 0; vi<x.size(); vi++){
    for(int di = 0; di<dim; di++){
      int col = vi*dim + di;
      for (auto it = sblocks[vi].begin(); it != sblocks[vi].end(); it++){
        int vj = it->first;
        for(int dj = 0; dj<dim; dj++){
          int row = vj*dim+dj;
#ifdef DEBUG
          if(row>=N || col>=N){
            std::cout<<"Index out of bound in stiffness assembly" <<col<<" "<<row<<"\n";
          }
#endif
          Kpattern.valuePtr()[cnt] = (it->second)(di, dj);
          cnt++;
        }
      }
    }
  }

  timer.endWall();
  //std::cout<<"Stiffness conversion time "<<timer.getSecondsWall()<<"\n";
  return Kpattern;
}

Eigen::SparseMatrix<double>
ElementMesh::getStiffnessDamping(Eigen::SparseMatrix<double> & D,
  std::vector<float> dcoef)
{
  int N = dim * (int)x.size();
  Timer timer;
  if (Kpattern.size() == 0){
    std::vector<int> I, J;
    stiffnessPattern(I, J);
  }
  D = Kpattern;
  int nV = e[0]->nV();
  if (sblocks.size() == 0){
    sblocks.resize(x.size());
    for (unsigned int ii = 0; ii < e.size(); ii++){
      Element * ele = e[ii];
      nV = ele->nV();
      for (int jj = 0; jj < nV; jj++){
        int vj = ele->at(jj);
        for (int kk = 0; kk < nV; kk++){
          int vk = ele->at(kk);
          auto it = sblocks[vj].find(vk);
          if (it == sblocks[vj].end()){
            sblocks[vj][vk] = Eigen::MatrixXd::Zero(dim, dim);
          }
        }
      }
    }
  }
  for (unsigned int vi = 0; vi < x.size(); vi++){
    for (auto it = sblocks[vi].begin(); it != sblocks[vi].end(); it++){
      it->second = Eigen::MatrixXd::Zero(dim, dim);
    }
  }
  std::vector< std::map< int, Eigen::MatrixXd > > Dblocks = sblocks;

  double localStiffTime = 0;
  double assembleTime = 0;

  for (unsigned int ii = 0; ii<e.size(); ii++){
    timer.startWall();
    int mat = me[ii];
    float damp = dcoef[mat];
    Element * ele = e[ii];
    nV = ele->nV();
    Eigen::MatrixXd K = getStiffness(ii);
    timer.endWall();
    localStiffTime += timer.getSecondsWall();
    timer.startWall();
    for (int jj = 0; jj<nV; jj++){
      int vj = ele->at(jj);
      for (int kk = 0; kk<nV; kk++){
        int vk = ele->at(kk);
        auto it = sblocks[vj].find(vk);
        it->second += K.block(dim * jj, dim * kk, dim, dim);
        it = Dblocks[vj].find(vk);
        it->second += damp * K.block(dim * jj, dim * kk, dim, dim);
      }
    }
    timer.endWall();
    assembleTime += timer.getSecondsWall();
  }
  //std::cout << "local stiffness time " << localStiffTime << "\n";
  //std::cout << "assemble time " << assembleTime << "\n";
  timer.startWall();
  int cnt = 0;
  for (unsigned int vi = 0; vi<x.size(); vi++){
    for (int di = 0; di<dim; di++){
      int cnt1 = cnt;
      for (auto it = sblocks[vi].begin(); it != sblocks[vi].end(); it++){
        int vj = it->first;
        for (int dj = 0; dj<dim; dj++){
          int row = vj*dim + dj;
          Kpattern.valuePtr()[cnt1] = (it->second)(di, dj);
          cnt1++;
        }
      }

      for (auto it = Dblocks[vi].begin(); it != Dblocks[vi].end(); it++){
        int vj = it->first;
        for (int dj = 0; dj<dim; dj++){
          int row = vj*dim + dj;
          D.valuePtr()[cnt] = (it->second)(di, dj);
          cnt++;
        }
      }

    }
  }

  timer.endWall();
  //std::cout<<"Stiffness conversion time "<<timer.getSecondsWall()<<"\n";
  return Kpattern;

}

void ElementMesh::stiffnessPattern(std::vector<int> & I, std::vector<int> & J)
{
  int N = dim * (int)x.size();
  Kpattern.resize(N, N);
  std::cout << "N " << N << "\n";
  Kpattern.reserve(Eigen::VectorXi::Constant(N, 81));
  for(unsigned int ii = 0;ii<e.size();ii++){
    Element * ele = e[ii];
    for(int jj = 0; jj<ele->nV(); jj++){
      int vj = ele->at(jj);
      for(int kk = 0; kk<ele->nV(); kk++){
        int vk = ele->at(kk);
        for(int dim1= 0 ;dim1<dim; dim1++){
          for(int dim2= 0 ;dim2<dim; dim2++){
            Kpattern.coeffRef(dim*vj + dim1, dim*vk + dim2) += 1;
          }
        }
      }
    }
  }
  Kpattern.makeCompressed();
  I.push_back(0);
  for (int ii = 0; ii<Kpattern.cols(); ii++){
    for (Eigen::SparseMatrix<double>::InnerIterator it(Kpattern, ii); it; ++it){
      J.push_back(it.row());
      //std::cout << it.row() << "\n";
    }
   I.push_back((int)J.size());
  }
}
