#ifndef ELEMENT_MESH_UTIL_HPP
#define ELEMENT_MESH_UTIL_HPP

#include "ElementMesh.hpp"

class ElementRegGrid;

extern int HexFaces[6][4];

///@brief save abaqus format.
void saveAbq(ElementMesh * em, std::string filename, float scale = 1);

//find the vertex that would collide during loading.
//Hack to avoid collision detection
void specialVertexBound(ElementMesh * m, float lb);

void hexToTrigMesh(ElementMesh * em, TrigMesh * tm);

void updateSurfVert(const std::vector<Eigen::Vector3d> & x, TrigMesh * tm, const std::vector<int> & vidx);

void saveSimState(const ElementMesh * em, std::ostream & out);

void loadSimState(ElementMesh * em, std::istream & in, bool loadDisplacement = false);

void subdivideHex(ElementMesh * em, int nSub = 2);

///@brief moves rest pose X to origin
///and sets x=X.
void placeAtOrigin(ElementMesh * em);

void translate(ElementMesh * em, const Eigen::Vector3d & vec);

///@param O new corner (0,0,0) of bounding box.
///@param Frame. Each column is an axis in world space.
void applyFrame(ElementMesh * em, const Eigen::Vector3d & O,
  const Eigen::Matrix3d & Frame);

bool hitWall(float wallDist, const std::vector<Eigen::Vector3d> & x, int dim, int sign);

void assignGridMat(const std::vector<double> & s,
  const std::vector<int> & gridSize, ElementRegGrid * grid);
Eigen::Vector3d eleCenter(const ElementMesh * e, int i);
#endif