#pragma once
#include <string>
namespace gs {
struct Config { int refresh_ms=1000; int max_processes=200; bool colors=true; };
Config load_config(const std::string& path="");
}
