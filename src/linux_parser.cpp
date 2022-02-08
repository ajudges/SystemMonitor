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


//-----------------------------------------------------------------------------
// Util
//-----------------------------------------------------------------------------
// Helper for getting value by key
template <typename T>
T findValueByKey(std::string const &keyFilter, std::string const &filename) {
  std::string line, key;
  T value;

  std::ifstream stream(LinuxParser::kProcDirectory + filename);
  if (stream.is_open()) {
    while (std::getline(stream, line)) {
      std::istringstream linestream(line);
      while (linestream >> key >> value) {
        if (key == keyFilter) {
          return value;
        }
      }
    }
  }
  return value;
};

// Helper for getting process stats
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

//-----------------------------------------------------------------------------
// System
//-----------------------------------------------------------------------------

// Read and return OS name
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
        if (key == LinuxParser::filterOSName) {
          std::replace(value.begin(), value.end(), '_', ' ');
          return value;
        }
      }
    }
  }
  return value;
}

// Read and return Linux Kernel version
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

// Read and return the id of processes on the system
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

// Read and return system memory utilization
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
        if (key == LinuxParser::filterMemTotal) {
          memTotal = value;
          continue;
        }
        if (key == LinuxParser::filterMemFree) {
          memFree = value;
        }
      }
    }
  }
  return (memTotal - memFree) / memTotal;
}

// Read and return uptime of processes
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

// Read and return the total number of processes on the system
int LinuxParser::TotalProcesses() {
  string line;
  string path = kProcDirectory + kStatFilename;
  string key = LinuxParser::filterProcesses;
  int value = findValueByKey<int>(key, path);
  return value;
}

// Read and return the running processes on the system
int LinuxParser::RunningProcesses() {
  string line;
  string path = kStatFilename;
  string key = LinuxParser::filterProcsRunning;
  int value = findValueByKey<int>(key, path);
  return value;
}

//-----------------------------------------------------------------------------
// Processor
//-----------------------------------------------------------------------------

// Read and return the number of jiffies for the system
long LinuxParser::Jiffies() {
  std::vector<string> utilization = CpuUtilization();
  long jiffies{0};
  for (string &jiffie : utilization) {
    jiffies += std::stol(jiffie);
  }
  return jiffies;
}

// Read and return the number of active jiffies for the system
long LinuxParser::ActiveJiffies() { return Jiffies() - IdleJiffies(); }

// Read and return the number of idle jiffies for the system
long LinuxParser::IdleJiffies() {
  std::vector<string> utilization = CpuUtilization();
  return std::stol(utilization[kIdle_]) + std::stol(utilization[kIOwait_]);
}

// Read and return CPU utilization
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
      if (key == LinuxParser::filterCpu) {
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

//-----------------------------------------------------------------------------
// Process
//-----------------------------------------------------------------------------

// Read and return the number of active jiffies for a process
long LinuxParser::ActiveJiffies(int pid) {
  auto stats = GetProcessStat(pid);
  long totalUtilization{0};
  for (auto &stat : stats) {
    totalUtilization += std::stol(stat);
  }
  auto uptime = LinuxParser::UpTime(pid);
  if (uptime == 0) {
    return (totalUtilization / sysconf(_SC_CLK_TCK));
  } else
    return (totalUtilization / sysconf(_SC_CLK_TCK)) / uptime;
}

// Read and return the command associated with a process
string LinuxParser::Command(int pid) {
  string path = kProcDirectory + std::to_string(pid) + kCmdlineFilename;
  std::ifstream stream(path);
  string line;
  if (stream.is_open()) {
    std::getline(stream, line);
  }
  return line;
}

// Read and return RAM utilization by a process
string LinuxParser::Ram(int pid) {
  string path = to_string(pid) + kStatusFilename;
  string key = filterProcMem;
  auto ram = findValueByKey<string>(key, path);
  if (ram.empty()){
    return to_string(0);
  }
  return to_string(std::stol(ram) / 1000);
}

// Read and return the user ID associated with a process
string LinuxParser::Uid(int pid) {
  string path = to_string(pid) + kStatusFilename;
  string key = LinuxParser::filterUID;
  return findValueByKey<string>(key, path);
}

// Read and return the user associated with a process
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
        if (uPid == LinuxParser::Uid(pid)) {
          return user;
          break;
        }
      }
    }
  }
  return string();
}

// Read and return the uptime of a process
long LinuxParser::UpTime(int pid) {
  auto utilization = GetProcessStat(pid);
  return LinuxParser::UpTime() -
         (std::stol(utilization.back()) / sysconf(_SC_CLK_TCK));
}
