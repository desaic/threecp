#ifndef CONTACT_HPP
#define CONTACT_HPP

#include <Eigen/Dense>
#include <vector>
#include <iostream> 

class ElementMesh;
class TrigMesh;

struct Contact{

  Contact() : v1(-1){
    m[0] = -1;
    m[1] = -1;
    v2[0] = -1;
  }

  //mesh index. 
  //vertex - face collision.
  //-1 for fixed objects
  //0 and + for meshes
  //-1 is always the second index if any.
  int m[2];
  //vertex.
  int v1;
  //quad vertices.
  int v2[4];
  //point of contact
  Eigen::Vector3d x;
  //bilinear weights.
  float alpha[2];
  //normal from 2nd to 1st
  Eigen::Vector3d normal;
  //two tangent directions
  Eigen::Vector3d d1, d2;

};

void saveContact(std::ostream & out, const std::vector<Contact> & contact);

void loadContact(std::istream & in, std::vector<Contact> & contact);

bool addContact(std::vector<ElementMesh *> & em_,
  std::vector<TrigMesh *> trigm,
  int m0, int m1, int t0, int v1,
  const std::vector<std::vector<Eigen::Vector3d> >& x0,
  std::vector<Contact> & contact);

bool detectCollision(std::vector<ElementMesh *> & em_,
  std::vector<TrigMesh *> trigm,
  const std::vector<std::vector<Eigen::Vector3d> >& x0,
  std::vector<Contact> & contact);

#endif