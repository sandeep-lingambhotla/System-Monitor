#include <dirent.h>
#include <unistd.h>
#include <string>
#include <vector>

#include "linux_parser.h"

using std::stof;
using std::string;
using std::to_string;
using std::vector;

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
  filestream.close();
  return value;
}

string LinuxParser::Kernel() {
  string os, version, kernel;
  string line;
  std::ifstream stream(kProcDirectory + kVersionFilename);
  if (stream.is_open()) {
    std::getline(stream, line);
    std::istringstream linestream(line);
    linestream >> os >> version >> kernel;
  }
  return kernel;
}

// BONUS: Update this to use std::filesystem
vector<int> LinuxParser::Pids() {
  vector<int> pids;
  DIR* directory = opendir(kProcDirectory.c_str());
  struct dirent* file;
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
  float memTotal=1.0, memFree=1.0;
  string line, key, value;
  std::ifstream stream(kProcDirectory+kMeminfoFilename);
  // Check if the file is open or not
  if(stream.is_open()){
    // get each line of the file. Loop until done
    while(std::getline(stream,line)){
      std::remove(line.begin(),line.end(),':');
      // Convert the line into a stream
      std::istringstream lineStream(line);
      // Read each key value pair
      while(lineStream>>key>>value){
        if(key == "MemTotal"){
          memTotal = stof(value);
        }
        if(key == "MemFree"){
          memFree = stof(value);
          break;
        }
      }
    }
  }
  stream.close();
  return ((memTotal - memFree)/memTotal);
}

long LinuxParser::UpTime(){ 
  string value, line;
  long upTime;
  std::ifstream stream(kProcDirectory+kUptimeFilename);
  if(stream.is_open()){
    while(std::getline(stream,line)){
      std::istringstream lineStream(line);
      lineStream >> value;
      upTime = std::stol(value);
    }
  }
  stream.close();
  return upTime; 
}

//SOURCE https://knowledge.udacity.com/questions/129844
long LinuxParser::Jiffies(){ 
  return LinuxParser::UpTime() * sysconf(_SC_CLK_TCK);
}

// Source https://bit.ly/3iupyot
long LinuxParser::ActiveJiffies(int pid){ 
  string s,a;
  int cnt=0, i=0;
  long ActiveJiffies=0.0;
  std::ifstream stream(kProcDirectory+"/"+std::to_string(pid)+kStatFilename);
  if(stream.is_open()){
    std::getline(stream,s);
    std::istringstream linestream(s);
    while(linestream>>a){
      i = std::stol(a);
      if(cnt>=13 && cnt<=16){
        ActiveJiffies+=i;
      }
      cnt++;
    }
  }
  return ActiveJiffies; 
}

// Source : https://bit.ly/3yzNexp
long LinuxParser::ActiveJiffies(){ 
  vector<string> cpuUtilization = LinuxParser::CpuUtilization();
  return ( std::stol(cpuUtilization[LinuxParser::kUser_])
         + std::stol(cpuUtilization[LinuxParser::kNice_])
         + std::stol(cpuUtilization[LinuxParser::kSystem_])
         + std::stol(cpuUtilization[LinuxParser::kIRQ_])
         + std::stol(cpuUtilization[LinuxParser::kSoftIRQ_])
         + std::stol(cpuUtilization[LinuxParser::kSteal_])
         );
}

// Source : https://bit.ly/3yzNexp
long LinuxParser::IdleJiffies(){ 
  vector<string> cpuUtilization = LinuxParser::CpuUtilization();
  return ( std::stol(cpuUtilization[LinuxParser::kIdle_])
         + std::stol(cpuUtilization[LinuxParser::kIOwait_])
         );
}

vector<string> LinuxParser::CpuUtilization(){ 
  vector<string> cpuUtilization;
  string cpuName, line, stat;
  std::ifstream filestream(kProcDirectory+kStatFilename);
  if(filestream.is_open()){
    std::getline(filestream,line);
    std::istringstream lineStream(line);
    lineStream >> cpuName;
    while(lineStream>>stat){
      cpuUtilization.push_back(stat);
    }
  }
  filestream.close();
  return cpuUtilization;
}

// Source: https://bit.ly/3fIibrE
int LinuxParser::TotalProcesses(){ 
  int totalProcesses;
  std::string line, key, value;
  std::ifstream filestream(kProcDirectory+kStatFilename);
  if(filestream.is_open()){
    while(std::getline(filestream,line)){
      std::istringstream lineStream(line);
      lineStream >> key >> value;
      if(key == "processes"){
        totalProcesses = stoi(value);
      }
    }
  }
  filestream.close();
  return totalProcesses;
}

// Read and return the number of running processes
int LinuxParser::RunningProcesses(){ 
  int totalProcesses;
  std::string line, key, value;
  std::ifstream filestream(kProcDirectory+kStatFilename);
  if(filestream.is_open()){
    while(std::getline(filestream,line)){
      std::istringstream lineStream(line);
      lineStream >> key >> value;
      if(key == "procs_running"){
        totalProcesses = stoi(value);
      }
    }
  }
  filestream.close();
  return totalProcesses;
}

// Read and return the command associated with a process
string LinuxParser::Command(int pid){ 
  std::ifstream filestream(kProcDirectory+"/"+to_string(pid)+kCmdlineFilename);
  std::string line;
  if(filestream.is_open()){
    std::getline(filestream,line);
  }
  filestream.close();
  return line;
}


string LinuxParser::Ram(int pid){ 
  string line, key, value;
  std::ifstream filestream(kProcDirectory+"/"+to_string(pid)+kStatusFilename);
  if(filestream.is_open()){
    while(std::getline(filestream,line)){
      std::istringstream linestream(line);
      linestream >> key >> value;
      if(key == "VmSize"){
        return value;
      }
    }
  }
  filestream.close();
  return value;
}

// Read and return the user ID associated with a process
string LinuxParser::Uid(int pid){ 
  string line, key, value;
  std::ifstream filestream(kProcDirectory+"/"+to_string(pid)+kStatusFilename);
  if(filestream.is_open()){
    while(std::getline(filestream,line)){
      std::istringstream linestream(line);
      linestream >> key >> value;
      if(key == "Uid"){
        return value;
      }
    }
  }
  filestream.close();
  return value;

}

// Read and return the user associated with a process
// Source : https://bit.ly/3CrOAwp
string LinuxParser::User(int pid){ 
  string uid = Uid(pid);
  string line, key, value, x;
  std::ifstream filestream(kPasswordPath);
  if(filestream.is_open()){
    std::getline(filestream,line);
    std::replace(line.begin(), line.end(), ':', ' ');
    std::istringstream linestream(line);
    linestream >> value >> x >> key;
    if(key == uid){
      return value;
    }
  }
  return value;
}