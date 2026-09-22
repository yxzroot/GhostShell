#pragma once
#include <string>
#include <vector>
namespace gs {
struct CpuCore { std::string name; double usage=0; };
struct Metrics { double cpu=0, mem=0, swap=0, load=0; long long uptime=0; unsigned long long total_mem=0, free_mem=0, swap_total=0, swap_free=0, disk_read=0, disk_write=0; std::vector<CpuCore> cores; std::string hostname, kernel, hardware; };
bool parse_cpu_line(const std::string&, unsigned long long&, unsigned long long&);
bool parse_meminfo(const std::string&, unsigned long long&, unsigned long long&);
bool parse_swapinfo(const std::string&, unsigned long long&, unsigned long long&);
Metrics read_metrics();
}
