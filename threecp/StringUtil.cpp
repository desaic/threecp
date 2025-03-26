#include "StringUtil.h"
std::string get_suffix(const std::string& str,
                       const std::string& delimiters) {
  size_t pos = str.find_last_of(delimiters);
  if (pos == std::string::npos) {
    return "";  // No suffix found
  }
  return str.substr(pos + 1);
}

std::string remove_suffix(const std::string& str, const std::string& suffix) {
  size_t pos = str.rfind(suffix);
  if (pos != std::string::npos) {
    return str.substr(0, pos);
  } else {
    return str;  // Suffix not found, return original string
  }
}

std::string get_file_name(const std::string& str) {
  // Find the last occurrence of '\' or '/' using rfind()
  size_t slash1 = str.rfind('\\');
  size_t slash2 = str.rfind('/');
  if (slash1 == std::string::npos ||
      (slash1 != std::string::npos && slash2 != std::string::npos &&
       slash2 > slash1)) {
    slash1 = slash2;
  } 
  // If no slash is found, return the entire string
  if (slash1 == std::string::npos) {
    return str;
  }

  // Extract the substring after the last slash
  return str.substr(slash1 + 1);
}
