#ifndef MESH_H
#define MESH_H

#include <map>
#include <vector>
#include <fstream>
#include <array>
class TrigMesh{
public:
	typedef std::array<float, 3> Vector3s;
	typedef std::array<float, 2> Vector2s;
	typedef std::array<int, 3> Vector3i;
	typedef std::array<std::array<float, 3>, 3> Matrix3f;
	std::vector<Vector3s>v;
  std::vector<Vector3s>n;
  std::vector<Vector2s>tex;
  std::vector<Vector3i>texId;
  //element index corresponding to a triangle.
  std::vector<int> ei;
  //face index corresponding to a quad.
  std::vector<int> fi;
  ///@brief vertex index in element mesh corresponding to v.
  std::vector<int> vidx;
  ///@brief triangles
  std::vector<Vector3i>t;

  std::vector<Matrix3f> tcolor;

  TrigMesh();
  TrigMesh(const std::vector<Vector3s>&_v,
      const std::vector<Vector3i>&_t);
  TrigMesh(const Vector3s *_v, const Vector3i *_t);
  
  TrigMesh(const char * filename,bool normalize);
  void load_mesh(const char * filename, bool normalize=true);
  void save(const char * filename);
  void save(std::ostream &out, std::vector<Vector3s> *vert=0);
  void load(std::istream &in);
  void read_ply(std::istream & f);
  void read_obj(std::istream &f);

  void save_obj(const char * filename);
  void load_tex(const char * filename);
  
  void compute_norm();
  void rescale();
  void append(const TrigMesh & m);
  TrigMesh & operator= (const TrigMesh& m);
  virtual void update();
};


void makeCube(TrigMesh & m, const TrigMesh::Vector3s &mn,
    const TrigMesh::Vector3s &mx);
///@brief cube [0,1]^3
extern TrigMesh UNIT_CUBE;

void adjlist(const TrigMesh & m, std::vector<std::vector<int> > & adjMat);

void savePly(TrigMesh * tm, std::string filename);

TrigMesh::Vector3s operator+(const TrigMesh::Vector3s & a, const TrigMesh::Vector3s & b);
TrigMesh::Vector3s operator-(const TrigMesh::Vector3s & a, const TrigMesh::Vector3s & b);
TrigMesh::Vector3s operator*(float a, const TrigMesh::Vector3s & b);
TrigMesh::Vector3s cross(const TrigMesh::Vector3s & a, const TrigMesh::Vector3s & b);
TrigMesh::Vector3s normalize(TrigMesh::Vector3s & a);
//make a plane in [0,1]x[0,1].
void makePlane(TrigMesh * m, int nx, int ny);
#endif
