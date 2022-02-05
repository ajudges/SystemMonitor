#include "process.h"

#include <unistd.h>

#include <cctype>
#include <sstream>
#include <string>
#include <vector>
#include "linux_parser.h"

using std::string;
using std::to_string;
using std::vector;

Process::Process(int pid) : pid_(pid) {}

int Process::Pid() {
  return pid_;
}

float Process::CpuUtilization() {
  return LinuxParser::ActiveJiffies(this->pid_) / LinuxParser::ActiveJiffies();
  // return 0;
}

string Process::Command() { 
  return LinuxParser::Command(this->pid_);
  // return string();
}

string Process::Ram() { 
  // return LinuxParser::Ram(this->pid_);
  return string();
}

string Process::User() { 
  return LinuxParser::User(this->pid_);
}

long int Process::UpTime() {
  // return LinuxParser::UpTime(this->pid_);
  return 0;
}

// TODO: Overload the "less than" comparison operator for Process objects
// REMOVE: [[maybe_unused]] once you define the function
bool Process::operator<(Process const& a [[maybe_unused]]) const {
  return true;
}