#include "linux_parser.h"

#include <dirent.h>
#include <unistd.h>

#include <sstream>
#include <string>
#include <vector>

using std::stof;
using std::string;
using std::to_string;
using std::vector;

std::vector<string> GetProcessStat(int &pid);

string LinuxParser::OperatingSystem() {
  string line;
  string key;
  string value;
  std::ifstream filestream(kOSPath);
  if (filestream.is_open()) {
    while (std::getline(filestream, line)) {
      std::replace(line.begin(), line.end(), ' ', '_');
      std::replace(line.begin(), line.end(), '=', ' ');
      std::replace(line.begin(), line.end(), '"', ' ');
      std::istringstream linestream(line);
      while (linestream >> key >> value) {
        if (key == "PRETTY_NAME") {
          std::replace(value.begin(), value.end(), '_', ' ');
          return value;
        }
      }
    }
  }
  return value;
}

string LinuxParser::Kernel() {
  string os, kernel, version;
  string line;
  std::ifstream stream(kProcDirectory + kVersionFilename);
  if (stream.is_open()) {
    std::getline(stream, line);
    std::istringstream linestream(line);
    linestream >> os >> kernel >> version;
  }
  return version;
}

// BONUS: Update this to use std::filesystem
vector<int> LinuxParser::Pids() {
  vector<int> pids;
  DIR *directory = opendir(kProcDirectory.c_str());
  struct dirent *file;
  while ((file = readdir(directory)) != nullptr) {
    // Is this a directory?
    if (file->d_type == DT_DIR) {
      // Is every character of the name a digit?
      string filename(file->d_name);
      if (std::all_of(filename.begin(), filename.end(), isdigit)) {
        int pid = stoi(filename);
        pids.push_back(pid);
      }
    }
  }
  closedir(directory);
  return pids;
}

float LinuxParser::MemoryUtilization() {
  float memTotal{0.};
  float memFree{0.};
  string line;

  std::ifstream filestream(kProcDirectory + kMeminfoFilename);
  if (filestream.is_open()) {
    while (std::getline(filestream, line) &&
           (memTotal == 0. or memFree == 0.)) {
      std::replace(line.begin(), line.end(), ':', ' ');
      std::istringstream linestream(line);
      string key;
      float value;
      while (linestream >> key >> value) {
        if (key == "MemTotal") {
          memTotal = value;
          continue;
        }
        if (key == "MemFree") {
          memFree = value;
        }
      }
    }
  }
  return (memTotal - memFree) / memTotal;
}

long LinuxParser::UpTime() {
  long uptime{0};
  string line;
  std::ifstream stream(kProcDirectory + kUptimeFilename);
  if (stream.is_open()) {
    std::getline(stream, line);
    std::istringstream linestream(line);
    linestream >> uptime;
  }
  return uptime;
}

long LinuxParser::Jiffies() {
  std::vector<string> utilization = CpuUtilization();
  long jiffies{0};
  for (string &jiffie : utilization) {
    jiffies += std::stol(jiffie);
  }
  return jiffies;
}

std::vector<string> GetProcessStat(int &pid) {
  std::vector<string> utilization;
  std::ifstream stream(LinuxParser::kProcDirectory + to_string(pid) +
                       LinuxParser::kStatFilename);
  bool is_finished{false};
  while (stream.is_open() && (not is_finished)) {
    string line;
    std::getline(stream, line);
    std::istringstream linestream(line);
    string buffer;
    int startTime{21};
    for (int i = 0; i <= 21; ++i) {
      linestream >> buffer;
      if ((i >= LinuxParser::utime && i <= LinuxParser::cstime) ||
          i == startTime) {
        utilization.emplace_back(buffer);
      }
    }
    is_finished = true;
  }
  return utilization;
}

long LinuxParser::ActiveJiffies(int pid) {
  auto stats = GetProcessStat(pid);
  long totalUtilization{0};
  for (auto &stat : stats) {
    totalUtilization += std::stol(stat);
  }
  auto uptime = LinuxParser::UpTime(pid);
  if (uptime == 0) {
    return 100 * ((totalUtilization / sysconf(_SC_CLK_TCK)));
  } else
    return 100 * ((totalUtilization / sysconf(_SC_CLK_TCK)) / uptime);
}

long LinuxParser::ActiveJiffies() { return Jiffies() - IdleJiffies(); }

long LinuxParser::IdleJiffies() {
  std::vector<string> utilization = CpuUtilization();
  return std::stol(utilization[kIdle_]) + std::stol(utilization[kIOwait_]);
}

vector<string> LinuxParser::CpuUtilization() {
  std::vector<string> utilization;
  string line;
  string key;
  string value;
  std::ifstream stream(kProcDirectory + kStatFilename);
  while (stream.is_open() && utilization.empty()) {
    std::getline(stream, line);
    std::istringstream linestream(line);
    while (linestream >> key) {
      if (key == "cpu") {
        for (int i = kUser_; i <= kGuestNice_; ++i) {
          linestream >> value;
          utilization.emplace_back(value);
        }
        break;
      }
    }
  }
  return utilization;
}

string GetValue(string &filePath, string &key) {
  string value;
  std::ifstream stream(filePath);
  if (stream.is_open()) {
    string line;
    string buffer;
    while (std::getline(stream, line)) {
      std::istringstream linestream(line);
      while (linestream >> buffer) {
        if (buffer == key) {
          linestream >> value;
          return value;
        }
      }
    }
  }
  return value;
}

int LinuxParser::TotalProcesses() {
  string line;
  string path = kProcDirectory + kStatFilename;
  string key = "processes";
  string value = GetValue(path, key);
  if (value.empty()) {
    return 0;
  }
  return std::stoi(value);
}

int LinuxParser::RunningProcesses() {
  string line;
  string path = kProcDirectory + kStatFilename;
  string key = "procs_running";
  string value = GetValue(path, key);
  if (value.empty()) {
    return 0;
  }
  return std::stol(value);
}

string LinuxParser::Command(int pid) {
  string path = kProcDirectory + std::to_string(pid) + kCmdlineFilename;
  std::ifstream stream(path);
  string line;
  if (stream.is_open()) {
    std::getline(stream, line);
  }
  return line;
}

string LinuxParser::Ram(int pid) {
  string path = kProcDirectory + to_string(pid) + kStatusFilename;
  string key = "VmSize:";
  return to_string(std::stol(GetValue(path, key)) / 1000);
}

string LinuxParser::Uid(int pid) {
  string path = kProcDirectory + to_string(pid) + kStatusFilename;
  string key = "Uid:";
  return GetValue(path, key);
}

string LinuxParser::User(int pid) {
  std::ifstream stream(kPasswordPath);
  if (stream.is_open()) {
    string user;
    string line;
    string delimiter;
    string uPid;
    while (std::getline(stream, line)) {
      std::replace(line.begin(), line.end(), ':', ' ');
      std::istringstream linestream(line);
      while (linestream >> user >> delimiter >> uPid) {
        if (uPid == to_string(pid)) {
          return user;
          break;
        }
      }
    }
  }
  return string();
}

long LinuxParser::UpTime(int pid) {
  auto utilization = GetProcessStat(pid);
  return LinuxParser::UpTime() -
         (std::stol(utilization.back()) / sysconf(_SC_CLK_TCK));
}