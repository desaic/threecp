#pragma once

#include <string>
#include <vector>

struct KeyboardInput {
  std::string keys;
  bool ctrl = false;
  bool shift = false;
  bool alt = false;
  bool hasKey ()const{return !keys.empty();}
  std::vector<std::string> splitKeys()const;
};
