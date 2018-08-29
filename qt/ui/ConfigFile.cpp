#include "ConfigFile.hpp"
#include "FileUtil.hpp"
#include <sstream>

int ConfigFile::load(const std::string filename)
{
  FileUtilIn in(filename);
  if (!in.good()){
    return -1;
  }
  dir = directoryName(filename);
  std::cout<<"Config file is in "<<dir<<"\n";
  std::string line;
  while (std::getline(in.in, line)){
    std::istringstream ss(line);
    if (line.size() < 3){
      continue;
    }
    std::string token;
    ss >> token;
    if (token[0] == '#'){
      continue;
    }
    std::string val;
    ss >> val;
    if(val=="list"){
      int len = 0;
      ss>>len;
      if(len<=0){
        std::cout<<"Read list of length 0?\n";
        continue;
      }
      std::vector<std::string> l(len);
      for(int ii = 0; ii<len; ii++){
        in.in>>l[ii];
        if(l[ii][0] == '#'){
          ii--;
        }
      }
      m[token] = l;
//      std::cout << token << ":";
//      for(unsigned int ii =0 ; ii<m[token].size(); ii++){
//        std::cout<< " "<<m[token][ii];
//      }
//      std::cout<< "\n";
    }else{
      m[token] = std::vector<std::string>(1, val);
//      std::cout << token << ": " << m[token][0] << "\n";
    }
  }
  in.close();

  return 0;
}

void ConfigFile::getFloat(const std::string & key, float & val)const
{
  if (hasOpt(key)){
    val = std::stof(m.find(key)->second[0]);
  }
}

void ConfigFile::getDouble(const std::string & key, double & val)const
{
	if (hasOpt(key)) {
		val = std::stod(m.find(key)->second[0]);
	}
}

bool ConfigFile::getInt(const std::string & key, int & val)const
{
  if (!hasOpt(key)){
    return false;
  }
  val = std::stoi(m.find(key)->second[0]);
  return true;
}

int ConfigFile::getInt(const std::string & key) const {
	if (!hasOpt(key)) {
		return -1;
	}
	return std::stoi(m.find(key)->second[0]);
}
std::string ConfigFile::getString(const std::string & key)const
{
  if (!hasOpt(key)){
    return std::string();
  }
  return m.find(key)->second[0];
}

std::vector<std::string>
ConfigFile::getStringVector(const std::string & key)const
{
  if (!hasOpt(key)){
    return std::vector<std::string>();
  }
  const std::vector<std::string> & val = m.find(key)->second;
  return val;
}

void ConfigFile::getBool(const std::string & key, bool & val)const
{
  auto iter = m.find(key);
  if (iter!=m.end()){
    val = (iter->second[0] == "true");
  }
}

bool
ConfigFile::getIntVector(const std::string & key, std::vector<int> & a)const
{
  if(!hasOpt(key)){
    return false;
  }
  const std::vector<std::string> & val = m.find(key)->second;
  int len = (int)(val.size());
  a.resize(len);
  for(int ii = 0; ii<len; ii++){
    a[ii] = std::stoi(val[ii]);
  }
  return true;
}

std::vector<float> ConfigFile::getFloatVector(const std::string&key)const
{
  std::vector<float> a;
  if(!hasOpt(key)){
    return a;
  }
  const std::vector<std::string> & val = m.find(key)->second;
  int len = (int)(val.size());
  a.resize(len);
  for(int ii = 0; ii<len; ii++){
    a[ii] = std::stof(val[ii]);
  }
  return a;
}

bool ConfigFile::hasOpt(const std::string & key)const
{
  return m.find(key) != m.end();
}
