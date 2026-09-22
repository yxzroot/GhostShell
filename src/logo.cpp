#include "logo.hpp"
namespace gs {
std::string logo(int width) {
 if(width < 42) return width >= 10 ? "G H O S T S H E L L" : "GS";
 const std::string lines[]={
 "        .-~~~-.",
 "   .-~~(       )~~-.",
 " .'                   `.",
 "(   ✦ G H O S T S H E L L ✦   )",
 " `.                     .'",
 "   `-~~-.___________.-~~'"
 };
 std::string out; for(int i=0;i<6;i++){std::string s=lines[i]; if((int)s.size()>width)s.resize(width); out+=s; if(i<5)out+='\n';} return out;
}
}
