#include "UIConf.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>
#include<vector>

#include "jsmn.h"

std::string escape_json(const std::string& s) {
  std::ostringstream o;
  for (auto c = s.cbegin(); c != s.cend(); c++) {
    switch (*c) {
      case '"':
        o << "\\\"";
        break;
      case '\\':
        o << "\\\\";
        break;
      case '\b':
        o << "\\b";
        break;
      case '\f':
        o << "\\f";
        break;
      case '\n':
        o << "\\n";
        break;
      case '\r':
        o << "\\r";
        break;
      case '\t':
        o << "\\t";
        break;
      default:
        if ('\x00' <= *c && *c <= '\x1f') {
          o << "\\u" << std::hex << std::setw(4) << std::setfill('0')
            << static_cast<int>(*c);
        } else {
          o << *c;
        }
    }
  }
  return o.str();
}

std::string unescape_json_string(const std::string& str) {
  std::string result;
  for (size_t i = 0; i < str.length(); ++i) {
    if (str[i] == '\\') {
      switch (str[i + 1]) {
        case '"':
        case '\\':
        case '/':
          result += str[i + 1];
          break;
        case 'b':
          result += '\b';
          break;
        case 'f':
          result += '\f';
          break;
        case 'n':
          result += '\n';
          break;
        case 'r':
          result += '\r';
          break;
        case 't':
          result += '\t';
          break;
        default:
          result += str[i];
          break;
      }
      ++i;
    } else {
      result += str[i];
    }
  }
  return result;
}

int ParseFloat(float& f, const std::string& s) {
  try {
    f = std::stod(s);
  } catch (const std::exception& e) {
    return -1;
  }
  return 0;
}

int UIConf::Load(const std::string& confFile) {
  std::ifstream in;
  std::string file = _confFile;
  if (confFile.size() > 0) {
    file = confFile;
  }
  in.open(file);
  if (in.good()) {
    _confFile = file;
  } else {
    std::cout << "failed to open " << file << "\n";
    return -1;
  }

  jsmn_parser parser;
  jsmn_init(&parser);
  std::vector<jsmntok_t> t(_numTokens);

  std::string str;
  in.seekg(0, std::ios::end);
  str.resize(in.tellg());
  in.seekg(0, std::ios::beg);
  in.read(&str[0], str.size());
  in.close();

  int r = jsmn_parse(&parser, str.data(), str.size(), t.data(), t.size());
  if (r < 0) {
    std::cout << "Failed to parse JSON: " << file << " return code " << r
              << "\n";
    return -2;
  }

  /* Assume the top-level element is an object */
  if (r < 1 || t[0].type != JSMN_OBJECT) {
    std::cout << "config does not begin with an object\n";
    return -2;
  }

  /* Loop over all keys of the root object */
  for (int i = 1; i < r; i++) {
    std::string key(&str[t[i].start], t[i].end - t[i].start);
    if (key == "outputFile") {
      i++;
      std::string val(&str[t[i].start], t[i].end - t[i].start);
      outputFile = val;
    } else if (key == "outDir") {
      i++;
      std::string val(&str[t[i].start], t[i].end - t[i].start);
      outDir = unescape_json_string(val);
    }
    else if (key == "workingDir") {
      i++;
      std::string val(&str[t[i].start], t[i].end - t[i].start);
      workingDir = unescape_json_string(val);
    } else if (key == "thicknessMM") {
      i++;
      std::string val(&str[t[i].start], t[i].end - t[i].start);
      thicknessMM = 0.032;
      ParseFloat(thicknessMM, val);
    } else if (key == "voxResMM") {
      i++;
      std::string val(&str[t[i].start], t[i].end - t[i].start);
      voxResMM = 0.032;
      ParseFloat(voxResMM, val);
    } else if (key == "waxGapMM") {
      i++;
      std::string val(&str[t[i].start], t[i].end - t[i].start);
      waxGapMM = 0.032;
      ParseFloat(waxGapMM, val);
    } else {
      std::cout << "Unexpected key: " << key << "\n";
      i++;
    }
  }
  return 0;
}

void PrintJson(const std::string & key, const std::string&val,std::ostream &out,
  bool lastItem = false) {
  out << "\"" << key << "\":\"" << escape_json(val) << "\"";
  if (!lastItem) {
    out << ",\n";
  }
}
void PrintJson(const std::string& key, float val,
               std::ostream& out, bool lastItem = false) {
  out << "\"" << key << "\":" << val;
  if (!lastItem) {
    out << ",";
  }
  out << "\n";
}
int UIConf::Save() {
  std::ofstream out(_confFile);
  if (!out.good()) {
    return -1;
  }
  const bool LAST_ITEM = true;
  out << "{\n";
  PrintJson("outputFile", outputFile, out);
  PrintJson("outDir", outDir, out);
  PrintJson("workingDir", workingDir, out);
  PrintJson("voxResMM", voxResMM, out);
  PrintJson("waxGapMM", waxGapMM, out);
  PrintJson("thicknessMM", thicknessMM, out);
  PrintJson("absoluteZero", -273.15f, out, LAST_ITEM);
  out<<"}\n";
  return 0;
}
