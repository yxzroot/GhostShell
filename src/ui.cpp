#include "ui.hpp"
#include "config.hpp"
#include "logo.hpp"
#include "metrics.hpp"
#include "network_storage.hpp"
#include "processes.hpp"
#include <ncurses.h>
#include <csignal>
#include <algorithm>
#include <deque>
#include <string>
#include <vector>

namespace {
volatile sig_atomic_t stop=0;
volatile sig_atomic_t resized=0;

enum Color { Accent=1, Good=2, Warn=3, Muted=4, Select=5 };

void on_signal(int signal) { if(signal==SIGWINCH) resized=1; else stop=1; }
void apply_color(WINDOW* w, int color) { if(has_colors()) wattron(w,COLOR_PAIR(color)); }
void remove_color(WINDOW* w, int color) { if(has_colors()) wattroff(w,COLOR_PAIR(color)); }

bool ensure_window(WINDOW*& window,int rows,int cols,int y,int x) {
  if(rows<=0 || cols<=0) return false;
  if(window) {
    int old_rows=0,old_cols=0,old_y=0,old_x=0;
    getmaxyx(window,old_rows,old_cols); getbegyx(window,old_y,old_x);
    if(old_rows!=rows || old_cols!=cols || old_y!=y || old_x!=x) {
      delwin(window); window=nullptr;
    }
  }
  if(!window) window=newwin(rows,cols,y,x);
  return window!=nullptr;
}

void destroy_windows(std::vector<WINDOW*>& windows) {
  for(WINDOW*& window:windows) { if(window) delwin(window); window=nullptr; }
}

void title(WINDOW* w,const char* text) {
  box(w,0,0);
  int cols=0,rows=0; getmaxyx(w,rows,cols);
  if(cols>4) { apply_color(w,Accent); mvwaddnstr(w,0,2,text,cols-4); remove_color(w,Accent); }
}

void text(WINDOW* w,int y,int x,const std::string& value,int width,int color=0) {
  if(width<=0) return;
  if(color) apply_color(w,color);
  mvwaddnstr(w,y,x,value.c_str(),width);
  if(color) remove_color(w,color);
}

std::string bar(double value,int width) {
  if(width<=0) return {};
  const int filled=std::max(0,std::min(width,static_cast<int>(value/100.0*width)));
  return std::string(filled,'#')+std::string(width-filled,'.');
}

void push_history(std::deque<double>& history,double value) {
  history.push_back(value);
  while(history.size()>48) history.pop_front();
}

void draw_header(WINDOW* w,int width,int height,const gs::Metrics& m) {
  werase(w);
  if(width<50 || height<2) return;
  const std::string host=m.hostname.empty()?"Linux host":m.hostname;
  const bool show_logo=width>=120 && height>=7;
  const int logo_width=show_logo?42:0;
  const int text_width=std::max(1,width-logo_width-3);
  apply_color(w,Accent); wattron(w,A_BOLD);
  mvwaddnstr(w,1,1,"GHOSTSHELL",text_width);
  wattroff(w,A_BOLD); remove_color(w,Accent);
  text(w,2,1,"LIVE SYSTEM MONITOR",text_width,Muted);
  const std::string status="● ONLINE  "+host;
  text(w,3,1,status,text_width,Good);
  if(show_logo) {
    std::string mark=gs::logo(42);
    size_t start=0;
    int row=0;
    while(start<mark.size() && row<6) {
      size_t end=mark.find('\n',start);
      if(end==std::string::npos) end=mark.size();
      text(w,row, width-logo_width, mark.substr(start,end-start), logo_width, Accent);
      start=end+1; ++row;
    }
  }
  if(height>=4) {
    std::string line(width-2,' ');
    for(int i=0;i<width-2;i++) line[i]=(i%2==0)?'-':' ';
    text(w,height-1,1,line,width-2,Muted);
  }
}

void draw_metric_card(WINDOW* w,int y,int x,int width,const char* name,const std::string& value,
                      const std::string& detail,double percent,int color) {
  if(width<13) return;
  apply_color(w,color); wattron(w,A_BOLD); mvwaddnstr(w,y,x,name,width-2); wattroff(w,A_BOLD); remove_color(w,color);
  wattron(w,A_BOLD);
  text(w,y+1,x,value,width-2,0);
  wattroff(w,A_BOLD);
  text(w,y+2,x,detail,width-2,Muted);
  if(width>=18) text(w,y+3,x,bar(percent,width-2),width-2,color);
}

void draw_dashboard(WINDOW* w,const gs::Metrics& m,const gs::Network& n,const gs::Storage& st,
                    std::deque<double>& cpu_history) {
  int height=0,width=0; getmaxyx(w,height,width); werase(w);
  title(w," LIVE OVERVIEW ");
  if(height<6 || width<50) return;
  const double disk=100.0*st.used/std::max(1ULL,st.total);
  const int gap=1;
  if(width>=70) {
    const int card_width=(width-1-gap*3)/4;
    const int x1=1, x2=x1+card_width+gap, x3=x2+card_width+gap, x4=x3+card_width+gap;
    draw_metric_card(w,1,x1,card_width,"CPU",std::to_string(static_cast<int>(m.cpu))+"%",
                     "load "+std::to_string(m.load).substr(0,4),m.cpu,Good);
    draw_metric_card(w,1,x2,card_width,"MEMORY",std::to_string(static_cast<int>(m.mem))+"%",
                     gs::format_bytes(m.free_mem)+" free",m.mem,Accent);
    draw_metric_card(w,1,x3,card_width,"NETWORK","RX "+gs::format_bytes(n.rx),
                     "TX "+gs::format_bytes(n.tx),0,Warn);
    draw_metric_card(w,1,x4,card_width,"DISK",gs::format_bytes(st.used),
                     gs::format_bytes(st.total)+" total",disk,Muted);
  } else {
    const int card_width=(width-1-gap)/2;
    const int x1=1, x2=x1+card_width+gap;
    draw_metric_card(w,1,x1,card_width,"CPU",std::to_string(static_cast<int>(m.cpu))+"%",
                     "load "+std::to_string(m.load).substr(0,4),m.cpu,Good);
    draw_metric_card(w,1,x2,card_width,"MEMORY",std::to_string(static_cast<int>(m.mem))+"%",
                     gs::format_bytes(m.free_mem)+" free",m.mem,Accent);
    draw_metric_card(w,5,x1,card_width,"NETWORK","RX "+gs::format_bytes(n.rx),
                     "TX "+gs::format_bytes(n.tx),0,Warn);
    draw_metric_card(w,5,x2,card_width,"DISK",gs::format_bytes(st.used),
                     gs::format_bytes(st.total)+" total",disk,Muted);
  }
  if(height>=7 && width>=100) {
    text(w,6,1,"CPU HISTORY",10,Muted);
    const int graph_width=std::min(width-13,static_cast<int>(cpu_history.size()));
    for(int i=0;i<graph_width;i++) {
      const int index=static_cast<int>(cpu_history.size())-graph_width+i;
      const int level=std::max(0,std::min(8,static_cast<int>(cpu_history[index]/12.5)));
      const char glyph=" .:-=+*#"[level];
      mvwaddch(w,6,11+i,glyph);
    }
  }
}

void draw_status(WINDOW* w,const gs::Metrics& m) {
  int height=0,width=0; getmaxyx(w,height,width); werase(w); title(w," SYSTEM STATUS ");
  if(height<3 || width<25) return;
  text(w,1,2,"UPTIME",8,Muted); text(w,1,11,std::to_string(m.uptime)+"s",16,0);
  text(w,2,2,"KERNEL",8,Muted); text(w,2,11,m.kernel, std::max(1,width-13),0);
  if(height>=4) { text(w,3,2,"HARDWARE",8,Muted); text(w,3,11,m.hardware,std::max(1,width-13),0); }
  if(height>=5) { text(w,4,2,"CORES",8,Muted); text(w,4,11,std::to_string(m.cores.size()>0?m.cores.size()-1:0),10,0); }
}

void draw_cores(WINDOW* w,const gs::Metrics& m) {
  int height=0,width=0; getmaxyx(w,height,width); werase(w); title(w," CPU CORES ");
  if(height<3 || width<35) return;
  int x=2;
  for(size_t i=1;i<m.cores.size() && x+10<width-1;i++) {
    const std::string label="C"+std::to_string(i)+" "+std::to_string(static_cast<int>(m.cores[i].usage))+"%";
    text(w,1,x,label,10,m.cores[i].usage>80?Warn:Good);
    x+=11;
  }
}

void draw_processes(WINDOW* w,const std::vector<gs::Process>& processes,int& selected,gs::Sort sort) {
  int height=0,width=0; getmaxyx(w,height,width); werase(w);
  const char* sort_name=sort==gs::Sort::Cpu?"CPU":sort==gs::Sort::Memory?"MEM":"PID";
  std::string heading=" PROCESSES  /  SORT "+std::string(sort_name)+" ";
  title(w,heading.c_str());
  if(height<4 || width<48) return;
  if(selected>=static_cast<int>(processes.size())) selected=std::max(0,static_cast<int>(processes.size())-1);
  apply_color(w,Muted); wattron(w,A_BOLD);
  text(w,1,2,"PID",7); text(w,1,11,"CPU",8); text(w,1,21,"MEM",10); text(w,1,34,"NAME",std::max(1,width-36));
  wattroff(w,A_BOLD); remove_color(w,Muted);
  const int rows=height-3;
  for(int i=0;i<static_cast<int>(processes.size()) && i<rows;i++) {
    const gs::Process& p=processes[i];
    if(i==selected) { wattron(w,COLOR_PAIR(Select)|A_BOLD); }
    mvwprintw(w,i+2,2,"%-7d",p.pid);
    mvwprintw(w,i+2,11,"%6.1f%%",p.cpu);
    text(w,i+2,21,gs::format_bytes(static_cast<unsigned long long>(std::max(0LL,p.rss)*1024)),10);
    text(w,i+2,34,p.name,std::max(1,width-36));
    if(i==selected) wattroff(w,COLOR_PAIR(Select)|A_BOLD);
  }
}

void draw_footer(WINDOW* w,bool help) {
  werase(w);
  int height=0,width=0; getmaxyx(w,height,width);
  const std::string message=help
    ?" q quit   ? close help   s sort   1/2/3 metric   arrows/jk navigate   g/G bounds "
    :" q quit   ? help   s sort   1/2/3 metric   arrows/jk navigate ";
  text(w,0,1,message,std::max(1,width-2),Muted);
}
}

namespace gs {
int run_ui() {
  Config cfg=load_config();
  initscr(); cbreak(); noecho(); keypad(stdscr,TRUE);
  timeout(std::max(50,cfg.refresh_ms)); curs_set(0);
  if(has_colors()) {
    start_color(); use_default_colors();
    init_pair(Accent,COLOR_CYAN,-1); init_pair(Good,COLOR_GREEN,-1);
    init_pair(Warn,COLOR_YELLOW,-1); init_pair(Muted,COLOR_BLUE,-1);
    init_pair(Select,COLOR_BLACK,COLOR_CYAN);
  }
  std::signal(SIGINT,on_signal); std::signal(SIGTERM,on_signal); std::signal(SIGWINCH,on_signal);
  Sort sort=Sort::Cpu; int selected=0; bool help=false;
  std::deque<double> cpu_history;
  std::vector<WINDOW*> windows(6,nullptr);
  while(!stop) {
    if(resized) {
      endwin(); refresh(); clear(); destroy_windows(windows); resized=0;
    }
    int h=0,w=0; getmaxyx(stdscr,h,w);
    Metrics metrics=read_metrics(); Network network=read_network(); Storage storage=read_storage();
    push_history(cpu_history,metrics.cpu);
    const int header_height=h>=12?(w>=120?7:4):0;
    erase();
    if(h<10 || w<45) {
      destroy_windows(windows);
      if(h>0) {
        text(stdscr,0,0,"GHOSTSHELL",w,Accent);
        if(h>1) text(stdscr,1,0,"Terminal too small - resize for dashboard",w,Muted);
        if(h>2) mvprintw(h-1,0,"q quit | CPU %.1f%% | MEM %.1f%%",metrics.cpu,metrics.mem);
      }
      wnoutrefresh(stdscr); doupdate();
      const int ch=getch(); if(ch=='q'||ch=='Q') stop=1;
      continue;
    }
    ensure_window(windows[0],header_height,w,0,0);
    const int overview_height=w>=100?8:w>=70?9:10;
    const int overview_top=header_height;
    ensure_window(windows[1],overview_height,w,overview_top,0);
    const int status_width=w>=90?27:0;
    const int status_top=overview_top+overview_height;
    if(status_width>0) ensure_window(windows[2],5,status_width,status_top,0);
    else if(windows[2]) { delwin(windows[2]); windows[2]=nullptr; }
    const int content_left=status_width>0?status_width+1:0;
    const int cores_top=status_top;
    const bool show_cores=h>=22 && w>=80;
    if(show_cores) ensure_window(windows[3],3,w-content_left,cores_top,content_left);
    else if(windows[3]) { delwin(windows[3]); windows[3]=nullptr; }
    const int proc_top=show_cores?cores_top+3:cores_top;
    if(proc_top<h-2) ensure_window(windows[4],h-proc_top-1,w,proc_top,0);
    else if(windows[4]) { delwin(windows[4]); windows[4]=nullptr; }
    ensure_window(windows[5],1,w,h-1,0);

    if(windows[0]) draw_header(windows[0],w,header_height,metrics);
    if(windows[1]) draw_dashboard(windows[1],metrics,network,storage,cpu_history);
    if(windows[2]) draw_status(windows[2],metrics);
    if(windows[3]) draw_cores(windows[3],metrics);
    std::vector<Process> processes=collect_processes(sort,cfg.max_processes);
    if(windows[4]) draw_processes(windows[4],processes,selected,sort);
    if(windows[5]) draw_footer(windows[5],help);

    wnoutrefresh(stdscr);
    for(WINDOW* window:windows) if(window) wnoutrefresh(window);
    doupdate();
    const int ch=getch();
    if(ch=='q'||ch=='Q') stop=1;
    else if(ch=='?') help=!help;
    else if(ch=='s') sort=sort==Sort::Cpu?Sort::Memory:sort==Sort::Memory?Sort::Pid:Sort::Cpu;
    else if(ch=='1') sort=Sort::Cpu;
    else if(ch=='2') sort=Sort::Memory;
    else if(ch=='3') sort=Sort::Pid;
    else if(ch=='g') selected=0;
    else if(ch=='G') selected=std::max(0,static_cast<int>(processes.size())-1);
    else if(ch==KEY_UP||ch=='k') selected=std::max(0,selected-1);
    else if(ch==KEY_DOWN||ch=='j') selected=std::min(std::max(0,static_cast<int>(processes.size())-1),selected+1);
  }
  destroy_windows(windows); endwin(); return 0;
}
}
