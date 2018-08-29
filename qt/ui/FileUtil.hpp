#ifndef FILEUTIL_HPP
#define FILEUTIL_HPP
#include <vector>
#include <iostream>
#include <fstream>

class FileUtilIn
{
public:
  FileUtilIn();
  FileUtilIn(const std::string filename);
  void open(const std::string filename, std::ios_base::openmode mode= std::ios_base::in);
  void close();
  void readFArr(std::vector<float> & arr);
  void readFArr(std::vector<float> & arr, int size);

  bool good(){ return in.good(); }
  template <typename T>
  friend std::istream & operator>>(std::istream & ii, std::vector<T> & arr);
  std::ifstream in;
};

class FileUtilOut
{
public:

  FileUtilOut();
  FileUtilOut(const std::string filename);
  void open(const std::string filename, std::ios_base::openmode mode = std::ios_base::out);
  void close();

  template <typename T>
  friend std::ostream & operator<<(std::ostream & o, const std::vector<T> & arr);

  std::ofstream out;
};

///@brief make a string dirname/prefixseq.txt
std::string
sequenceFilename(int seq, const char * dirname = 0,
                          const char * prefix  = 0,
                 int padding = 0, const char * suffix= 0);

std::string directoryName(std::string filename);

template <typename T>
std::vector<std::vector<T> > loadArr2d(std::string filename)
{
  FileUtilIn in(filename);
  std::vector<std::vector<T> > arr;
  if (!in.good()){
    return arr;
  }
  int nrow, ncol;
  in.in >> nrow >> ncol;
  arr.resize(nrow);
  for (int i = 0; i < nrow; i++){
    arr[i].resize(ncol);
    for (int j = 0; j < ncol; j++){
      in.in >> arr[i][j];
    }
  }
  return arr;
}

template <typename T>
bool loadArr1d(std::string filename, std::vector<T> & arr)
{
	FileUtilIn in(filename);
	if (!in.good()) {
		return false;
	}
	int nrow;
	in.in >> nrow;
	arr.resize(nrow);
	for (int i = 0; i < nrow; i++) {
		in.in >> arr[i];
	}
	return true;
}

template<typename T> 
bool loadVector3(std::istream & in, std::vector<T> & arr) {
	int nrow, ncol;
	in >> nrow>>ncol;
	if (ncol != 3) {
		return false;
	}
	arr.resize(nrow);
	for (int i = 0; i < nrow; i++) {
		in >> arr[i][0]>> arr[i][1]>> arr[i][2];
	}
	return true;
}

template<typename T>
bool writeVector3(std::ostream & out, std::vector<T> & arr) {
	out << arr.size() << " 3\n";
	for (size_t i = 0; i < arr.size(); i++) {
		out << arr[i][0] <<" "<< arr[i][1] << " "<< arr[i][2]<<"\n";
	}
	return true;
}

std::string loadTxtFile(std::string filename);

void saveHeightObj(std::string filename, const std::vector<float> & h, int width, int height,
	const float * dx, bool saveTrig = true);

void saveFloatImage(std::string filename, const std::vector<float> & img, size_t width, size_t height);

bool loadFloatImage(std::string filename, std::vector<float> & img, size_t& width, size_t& height);

void loadFloatArr(std::string filename, std::vector<float> & img, size_t len);

#endif // FILEUTIL_HPP
