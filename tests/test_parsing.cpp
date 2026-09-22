#include "metrics.hpp"
#include "network_storage.hpp"
#include <cassert>
int main() {
  unsigned long long idle=0,total=0;
  assert(gs::parse_cpu_line("cpu 10 2 3 4 1 0 0 0",idle,total) && idle==5 && total==20);
  assert(!gs::parse_cpu_line("cpu nope",idle,total));
  unsigned long long mem=0,avail=0;
  assert(gs::parse_meminfo("MemTotal: 100 kB\nMemAvailable: 40 kB\n",mem,avail) && mem==102400 && avail==40960);
  unsigned long long swap=0,free=0;
  assert(gs::parse_swapinfo("SwapTotal: 2 kB\nSwapFree: 1 kB\n",swap,free) && swap==2048 && free==1024);
  assert(gs::format_bytes(1024)=="1.0K");
}
