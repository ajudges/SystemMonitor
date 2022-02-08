#include "format.h"

#include <string>

using std::string;
using std::to_string;

string Format::ElapsedTime(long seconds) {
  return to_string(long(seconds / 3600)) + ':' +
         to_string(long((seconds / 60) % 60)) + ':' +
         to_string(long(seconds % 60));
}