#include "network_storage.hpp"
#include <fstream>
#include <sstream>
#include <sys/statvfs.h>
namespace gs {
Network read_network(){Network n;std::ifstream f("/proc/net/dev");std::string l;while(std::getline(f,l)){auto p=l.find(':');if(p==std::string::npos)continue;std::istringstream s(l.substr(p+1));unsigned long long r=0,t=0; if(!(s>>r)) continue; for(int i=0;i<7;i++) if(!(s>>t)) { t=0; break; } if(s>>t) { n.rx+=r; n.tx+=t; }}return n;}
Storage read_storage(const std::string&p){struct statvfs s{};Storage x;if(statvfs(p.c_str(),&s)==0){x.total=(unsigned long long)s.f_blocks*s.f_frsize;x.used=x.total-(unsigned long long)s.f_bfree*s.f_frsize;}return x;}
std::string format_bytes(unsigned long long n){const char*u[]={"B","K","M","G","T"};int i=0;double v=n;while(v>=1024&&i<4)v/=1024,++i;std::ostringstream s;s.precision(1);s<<std::fixed<<v<<u[i];return s.str();}
}
