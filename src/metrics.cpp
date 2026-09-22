#include "metrics.hpp"
#include <fstream>
#include <sstream>
#include <sys/sysinfo.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <map>
#include <algorithm>
namespace gs {
bool parse_cpu_line(const std::string& l,unsigned long long& idle,unsigned long long& total) {
 std::istringstream s(l); std::string cpu; unsigned long long u,n,sy,id,io,irq,si,st,gu=0,gn=0;
 if (!(s >> cpu >> u >> n >> sy >> id >> io >> irq >> si >> st)) {
  return false;
 }
 s >> gu >> gn;
 idle=id+io; total=u+n+sy+id+io+irq+si+st+gu+gn; return true;
}
bool parse_meminfo(const std::string& text,unsigned long long& total,unsigned long long& avail) {
 total=avail=0; std::istringstream in(text); std::string k; unsigned long long v; std::string unit;
 while(in>>k>>v>>unit) { if(k=="MemTotal:") total=v*1024; else if(k=="MemAvailable:") avail=v*1024; }
 return total>0;
}
bool parse_swapinfo(const std::string& text,unsigned long long& total,unsigned long long& free) {
 total=free=0; std::istringstream in(text); std::string k,unit; unsigned long long v;
 while(in>>k>>v>>unit){if(k=="SwapTotal:")total=v*1024;else if(k=="SwapFree:")free=v*1024;} return total>0;
}
Metrics read_metrics() {
 Metrics m; std::ifstream f("/proc/meminfo"); std::string t((std::istreambuf_iterator<char>(f)),{}); unsigned long long a;
 parse_meminfo(t,m.total_mem,a); m.free_mem=a; parse_swapinfo(t,m.swap_total,m.swap_free); if(m.total_mem)m.mem=100.0*(m.total_mem-a)/m.total_mem; if(m.swap_total)m.swap=100.0*(m.swap_total-m.swap_free)/m.swap_total;
 std::ifstream u("/proc/uptime"); u>>m.uptime; if(!u){struct sysinfo si{};if(sysinfo(&si)==0)m.uptime=si.uptime;}
 std::ifstream l("/proc/loadavg"); l>>m.load; if(!l){struct sysinfo si{};if(sysinfo(&si)==0)m.load=si.loads[0]/static_cast<double>(1<<SI_LOAD_SHIFT);}
 static std::map<std::string,std::pair<unsigned long long,unsigned long long>> prev; std::ifstream c("/proc/stat"); std::string line;
 while(std::getline(c,line)){
  if(line.rfind("cpu",0)!=0) break;
  std::istringstream q(line);
  std::string name;
  q>>name;
  unsigned long long i,tt;
  if(parse_cpu_line(line,i,tt)){
   auto old=prev[name];
   double usage=(old.second && tt>=old.second && i>=old.first && tt>old.second)
    ?100.0*((tt-old.second)-(i-old.first))/(tt-old.second):0;
   m.cores.push_back({name,usage});
   if(name=="cpu") m.cpu=std::max(0.0,std::min(100.0,usage));
   prev[name]={i,tt};
  }
 }
 std::ifstream d("/proc/diskstats"); while(std::getline(d,line)){std::istringstream q(line);int maj,min;std::string dev;unsigned long long x; if(!(q>>maj>>min>>dev))continue;std::vector<unsigned long long> flds;while(q>>x)flds.push_back(x);if(flds.size()>6){m.disk_read+=flds[2]*512;m.disk_write+=flds[6]*512;}}
 char host[256]{};if(gethostname(host,sizeof(host))==0)m.hostname=host;struct utsname un{};if(uname(&un)==0)m.kernel=std::string(un.sysname)+" "+un.release;std::ifstream hw("/sys/devices/virtual/dmi/id/product_name");std::getline(hw,m.hardware);if(m.hardware.empty())m.hardware="Linux host";
 return m;
}
}
