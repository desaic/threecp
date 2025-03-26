#ifndef STRING_UTIL_H
#define STRING_UTIL_H
#include <string>
std::string get_suffix(const std::string& str,
                       const std::string& delimiters = ".");
std::string remove_suffix(const std::string& str, const std::string& suffix);
std::string get_file_name(const std::string& str);
#endif