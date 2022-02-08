#include "processor.h"

#include <chrono>
#include <thread>

#include "linux_parser.h"

// Return the aggregate CPU utilization
float Processor::Utilization() {
  float total = (float)LinuxParser::Jiffies();
  float idle = (float)LinuxParser::IdleJiffies();

  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  float totalDiff = (float)LinuxParser::Jiffies() - total;
  float idleDiff = (float)LinuxParser::IdleJiffies() - idle;

  if (totalDiff == 0) {
    return (float)LinuxParser::ActiveJiffies() / (float)LinuxParser::Jiffies();
  }

  return (totalDiff - idleDiff) / totalDiff;
}