#include <iomanip>
#include <iostream>
#include "wlua/wlua.hpp"

int simple(int x, int y) {
  std::cout << "input: (" << x << ',' << y << ')' << std::endl;
  return x * y + y;
}

void no_return(float x, double y, const char* z) {
  std::cout << "input: (" << std::setprecision(14) << x << ',' << y << ',' << z
            << ')' << std::endl;
}

int main(int argc, char** argv) {
  auto handle = luaL_newstate();
  luaL_openlibs(handle);

  wlua::add_function<WLUA_FUNCTION(simple)>(handle, "simple");
  wlua::add_function<WLUA_FUNCTION(no_return)>(handle, "no_return");
  luaL_loadfile(handle, "scripts/simple.lua");
  lua_pcall(handle, 0, 0, 0);
  return 0;
}
