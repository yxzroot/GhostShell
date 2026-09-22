#pragma once
#include <string>
namespace gs {
struct Network { unsigned long long rx=0,tx=0; };
struct Storage { unsigned long long used=0,total=0; };
Network read_network();
Storage read_storage(const std::string& path="/");
std::string format_bytes(unsigned long long);
}
