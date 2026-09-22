#pragma once
#include <cstddef>
#include <string>
#include <vector>
namespace gs {
struct Process { int pid=0; std::string user,name,state; double cpu=0,mem=0; long long rss=0; };
enum class Sort { Cpu, Memory, Pid };
std::vector<Process> collect_processes(Sort sort=Sort::Cpu, size_t limit=200);
}
