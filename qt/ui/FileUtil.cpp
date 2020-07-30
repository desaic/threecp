#include "FileUtil.hpp"
#include <stdlib.h>
#include <string>
#include <iomanip>
#include <sstream>

void
FileUtilIn::open(const std::string filename,std::ios_base::openmode mode )
{
  in.open(filename,mode);
  if(!in.good()){
    std::cout<<"cannot open "<<filename<<"\n";
  }
}

void FileUtilIn::close()
{
  in.close();
}

void
FileUtilIn::readFArr(std::vector<float> & arr)
{
  int size;
  in>>size;
  arr.resize(size);
  for(int ii = 0;ii<size;ii++){
    in>>arr[ii];
  }
}

void
FileUtilIn::readFArr(std::vector<float> & arr, int size)
{
  arr.resize(size);
  for(int ii = 0;ii<size;ii++){
    in>>arr[ii];
  }
}

template <typename T>
std::istream &
operator>>(std::istream & i, std::vector<T> & arr)
{
  int size = 0;
  i>>size;
  arr.resize(size);
  for(int ii = 0;ii<size;ii++){
    i>>arr[ii];
  }
}

FileUtilIn::FileUtilIn(const std::string filename):in(filename)
{
  if(!in.good()){
    std::cout<<"cannot open "<<filename<<"\n";
  }
}

FileUtilIn::FileUtilIn()
{}

void
FileUtilOut::open(const std::string filename, std::ios_base::openmode mode)
{
  out.open(filename,mode);
  if(!out.good()){
    std::cout<<"cannot open output"<<filename<<"\n";
  }
}

void
FileUtilOut::close()
{
  out.close();
}

FileUtilOut::FileUtilOut(const std::string filename):out(filename)
{
  if(!out.good()){
    std::cout<<"cannot open output"<<filename<<"\n";
  }
}

FileUtilOut::FileUtilOut()
{}

template <typename T>
std::ostream &
operator<<(std::ostream & o, const std::vector<T> & arr)
{
  o<<arr.size()<<"\n";
  for(unsigned int ii = 0 ;ii<arr.size();ii++){
    o<<arr[ii]<<"\n";
  }
}

std::string
sequenceFilename(int seq, const char * dirname,
                          const char * prefix,
                 int padding, const char * _suffix)
{
  std::stringstream ss;
  if(dirname != 0){
    ss<<dirname<<"/";
  }
  if(prefix != 0){
    ss<<prefix;
  }
  std::string suffix(".txt");
  if(_suffix!=0){
    suffix = std::string(_suffix);
  }
  if(padding>0){
    ss << std::setfill('0') << std::setw(padding)<<seq<<suffix;
  }else{
    ss << seq<<suffix;
  }
  return ss.str();
}

std::string directoryName(std::string filename)
{
  std::string path;
  std::string s;

#ifdef __linux__
  char * fullpath;
  char resolve[PATH_MAX];
  fullpath = realpath(filename.c_str(), resolve);
  if (fullpath == NULL){
    std::cout << "Error resolving " << filename << "\n";
  }
  s = std::string(fullpath);
#elif _WIN32
  char full[_MAX_PATH];
  if (_fullpath(full, filename.c_str(), _MAX_PATH) != NULL){
    s = std::string(full);
  }
#endif
  std::size_t found = s.find_last_of("/\\");
  path = s.substr(0, found);
  path += "/";
  return path;
}

std::string
loadTxtFile(std::string filename)
{
	std::ifstream t(filename);
	if (!t.good()) {
		std::cout << "Cannot open " << filename << "\n";
	}
	std::string str;

	t.seekg(0, std::ios::end);
	str.reserve(t.tellg());
	t.seekg(0, std::ios::beg);

	str.assign((std::istreambuf_iterator<char>(t)),
		std::istreambuf_iterator<char>());
	return str;
}

void saveHeightObj(std::string filename, const std::vector<float> & h, int width, int height,
	const float * dx, bool saveTrig) {
	FileUtilOut out(filename);
	if (!out.out.good()) {
		return;
	}
	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			out.out << "v " << j * dx[0] << " " << i * dx[1] << " " << h[i*width + j] * dx[2]<<"\n";
		}
	}
  if(saveTrig){
      for (int i = 0; i < height - 1; i++) {
          for (int j = 0; j < width - 1; j++) {
              out.out << "f " << (i*width + j + 1) << " " << ((i + 1)*width + j + 1) << " " << (i*width + j + 2) << "\n";
              out.out << "f " << ((i + 1)*width + j + 1) << " " << ((i + 1)*width + j + 2) << " " << (i*width + j + 2) << "\n";
          }
      }
  }
	out.close();
}

void saveFloatImage(std::string filename, const std::vector<float> & img, size_t width, size_t height) {
	FileUtilOut out;
	out.open(filename, std::ofstream::binary);
	if (!out.out.good()) {
		return;
	}
	out.out.write((const char *)&width, sizeof(size_t));
	out.out.write((const char *)&height, sizeof(size_t));
	if (img.size() != width * height) {
		return;
	}
	out.out.write((const char *)img.data(), img.size() * sizeof(float));
	out.close();
}

bool loadFloatImage(std::string filename, std::vector<float> & img, size_t & width, size_t & height) {
	FileUtilIn in;
	in.open(filename, std::ifstream::binary);
	const int MAX_IMG_SIZE = 2000;
	if (!in.good()) {
		return false;
	}
	in.in.read((char*)&width, sizeof(size_t));
	in.in.read((char*)&height, sizeof(size_t));
	if (width > MAX_IMG_SIZE || height > MAX_IMG_SIZE) {
		return false;
	}
	img.resize(width*height);
	in.in.read((char*)img.data(), img.size() * sizeof(float));
	in.close();
  return true;
}

void loadFloatArr(std::string filename, std::vector<float> & img, size_t len) {
  FileUtilIn in;
  in.open(filename, std::ifstream::binary);
  const int MAX_IMG_SIZE = 2000;
  if (!in.good()) {
    return;
  }
  img.resize(len);
  in.in.read((char*)img.data(), img.size() * sizeof(float));
  in.close();
}