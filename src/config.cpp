#include "config.hpp"
#include <fstream>
#include <cstdlib>
namespace gs {
Config load_config(const std::string& path) {
 Config c; std::string p=path;
 if(p.empty()) { const char* h=getenv("HOME"); if(h) p=std::string(h)+"/.config/ghostshell/config"; }
 std::ifstream f(p); std::string k; int v;
 while(f>>k>>v) { if(k=="refresh_ms" && v>50) c.refresh_ms=v; else if(k=="max_processes"&&v>0)c.max_processes=v; }
 return c;
}
}
