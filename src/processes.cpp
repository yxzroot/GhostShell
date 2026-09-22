#include "processes.hpp"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <dirent.h>
#include <fstream>
#include <limits>
#include <sstream>
#include <unordered_map>

namespace gs {
namespace {
struct Sample { std::string state="?"; unsigned long long ticks=0; };
std::unordered_map<int, Sample> previous;
unsigned long long previous_total=0;

bool decimal_pid(const char* name) {
  if (!name || !*name) return false;
  for (const unsigned char* p=reinterpret_cast<const unsigned char*>(name); *p; ++p)
    if (!std::isdigit(*p)) return false;
  return true;
}
bool parse_stat(const std::string& line, Process& p, unsigned long long& ticks) {
  const size_t close=line.rfind(')');
  if (close==std::string::npos || close+2>=line.size()) return false;
  std::istringstream fields(line.substr(close+2));
  if (!(fields>>p.state)) return false;
  std::string token;
  unsigned long long utime=0, stime=0;
  // After state, fields are proc(5)..proc(13); utime and stime are 11th/12th here.
  for (int i=0; i<10; ++i) if (!(fields>>token)) return false;
  try { utime=std::stoull(token); } catch (...) { return false; }
  if (!(fields>>token)) return false;
  try { stime=std::stoull(token); } catch (...) { return false; }
  ticks=utime+stime;
  return true;
}
}
std::vector<Process> collect_processes(Sort sort, size_t limit) {
  unsigned long long total=0, value=0;
  std::ifstream stat("/proc/stat"); std::string line;
  if (std::getline(stat,line) && line.rfind("cpu ",0)==0) {
    std::istringstream values(line.substr(4)); while (values>>value) total+=value;
  }
  const unsigned long long delta=(total>previous_total)?total-previous_total:0;
  std::vector<Process> out;
  DIR* dir=opendir("/proc"); if (!dir) return out;
  while (dirent* entry=readdir(dir)) {
    if (!decimal_pid(entry->d_name)) continue;
    Process p; p.pid=std::atoi(entry->d_name);
    std::ifstream f(std::string("/proc/")+entry->d_name+"/stat");
    if (!std::getline(f,line)) continue;
    unsigned long long ticks=0; if (!parse_stat(line,p,ticks)) continue;
    const auto old=previous.find(p.pid);
    if (delta && old!=previous.end() && ticks>=old->second.ticks)
      p.cpu=std::min(100.0,100.0*(ticks-old->second.ticks)/static_cast<double>(delta));
    previous[p.pid] = Sample{p.state, ticks};
    std::ifstream status(std::string("/proc/")+entry->d_name+"/status");
    std::string key, val, unit;
    while (status>>key>>val) {
      if (key=="Name:") p.name=val;
      else if (key=="VmRSS:") { try { p.rss=std::stoll(val); } catch (...) { p.rss=0; } status>>unit; p.mem=p.rss; }
    }
    if (p.name.empty()) p.name="?";
    out.push_back(p);
  }
  closedir(dir); previous_total=total;
  auto cmp=[&](const Process&a,const Process&b) { if(sort==Sort::Memory)return a.rss>b.rss; if(sort==Sort::Pid)return a.pid<b.pid; return a.cpu>b.cpu; };
  std::sort(out.begin(),out.end(),cmp); if(out.size()>limit) out.resize(limit); return out;
}
}
