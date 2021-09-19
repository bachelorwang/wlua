#include <iomanip>
#include <iostream>
#include "wlua/wlua.hpp"

int main(int argc, char** argv) {
  auto handle = luaL_newstate();
  luaL_openlibs(handle);
  luaL_loadfile(handle, "scripts/cbind.lua");
  lua_pcall(handle, 0, 0, 0);

  auto simple = wlua::get_function<lua_Integer(lua_Integer, const char*)>(
      handle, "simple");
  auto return_string =
      wlua::get_function<const char*(void)>(handle, "return_string");
  auto no_return = wlua::get_function<void(void)>(handle, "no_return");
  simple(42, "is the answer");
  std::cout << return_string() << std::endl;
  no_return();
  return 0;
}
