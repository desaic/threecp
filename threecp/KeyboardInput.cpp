#include "KeyboardInput.h"

static std::vector<std::string> splitBySpace(const std::string& input) {
  std::vector<std::string> words;
  std::size_t start = 0;
  std::size_t end = input.find(' ');

  while (end != std::string::npos) {
    if (end > start) {
      words.push_back(input.substr(start, end - start));
    }
    start = end + 1;
    end = input.find(' ', start);
  }

  if (start < input.length()) {
    words.push_back(input.substr(start));
  }

  return words;
}

std::vector<std::string> KeyboardInput::splitKeys()const{	
  return  splitBySpace(keys);
}