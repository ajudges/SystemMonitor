#include "format.h"

#include <string>

using std::string;
using std::to_string;

// INPUT: Long int measuring seconds
// OUTPUT: HH:MM:SS
string Format::ElapsedTime(long seconds) {
  string hh = to_string(long(seconds / 3600));
  hh.insert(0, 2 - hh.length(), '0');
  string mm = to_string(long((seconds / 60) % 60));
  mm.insert(0, 2 - mm.length(), '0');
  string ss = to_string(long(seconds % 60));
  ss.insert(0, 2 - ss.length(), '0');
  std::string output = hh + ":" + mm + ":" + ss; 
  return output;
}