extern "C" {
#include <lua5.3/lauxlib.h>
#include <lua5.3/lua.h>
#include <lua5.3/lualib.h>
}
#include <tuple>
#include <type_traits>

namespace wlua {

namespace details {

template <typename _signature, _signature* _fp>
struct function;

template <typename _ret, typename... _args, _ret (*_fp)(_args...)>
struct function<_ret(_args...), _fp> {
  using result_type = _ret;
  using parameter_types = std::tuple<_args...>;
  static constexpr size_t parameter_count = sizeof...(_args);
  using type = _ret (*)(_args...);
  static constexpr _ret (*pointer)(_args...) = _fp;
};

template <typename _arg>
static inline auto pop_arg(_arg& arg, lua_State* handle, size_t i)
    -> std::enable_if_t<std::is_integral_v<_arg>, void> {
  arg = lua_tointeger(handle, i);
}

template <typename _arg>
static inline auto pop_arg(_arg& arg, lua_State* handle, size_t i)
    -> std::enable_if_t<std::is_floating_point_v<_arg>, void> {
  arg = lua_tonumber(handle, i);
}

static inline auto pop_arg(const char*& arg, lua_State* handle, size_t i) {
  arg = lua_tostring(handle, i);
}

template <typename _args, size_t... _indexes>
static inline void pop_all_args(lua_State* handle,
                                _args& args,
                                std::index_sequence<_indexes...>) {
  (pop_arg(std::get<_indexes>(args), handle, _indexes + 1), ...);
}

template <typename _function, typename>
struct invoker;

template <typename _function>
struct invoker<_function, std::true_type> {
  template <size_t... _indexes>
  static int invoke(lua_State* handle,
                    typename _function::parameter_types&& args,
                    std::index_sequence<_indexes...>) {
    (*_function::pointer)(std::get<_indexes>(args)...);
    return 0;
  }
};

template <typename _function>
struct invoker<_function, std::false_type> {
  template <size_t... _indexes>
  static int invoke(lua_State* handle,
                    typename _function::parameter_types&& args,
                    std::index_sequence<_indexes...>) {
    auto result = (*_function::pointer)(std::get<_indexes>(args)...);
    lua_push_result(handle, result);
    return 1;
  }

 private:
  template <typename _type>
  static inline auto lua_push_result(lua_State* handle, _type result)
      -> std::enable_if_t<std::is_integral_v<_type>> {
    lua_pushinteger(handle, result);
  }
};

template <typename _function>
struct lua_bind {
  static int invoke(lua_State* handle) {
    typename _function::parameter_types args;
    constexpr auto indexes =
        std::make_index_sequence<_function::parameter_count>{};
    pop_all_args(handle, args, indexes);
    using is_void =
        typename std::is_same<typename _function::result_type, void>::type;
    using _invoker = invoker<_function, is_void>;
    return _invoker::invoke(handle, std::move(args), indexes);
  }
};

static inline void push_arg(lua_State* handle, lua_Integer arg) {
  lua_pushinteger(handle, arg);
}

static inline void push_arg(lua_State* handle, lua_Number arg) {
  lua_pushnumber(handle, arg);
}

static inline void push_arg(lua_State* handle, const char* arg) {
  lua_pushstring(handle, arg);
}

template <typename... _args>
inline void lua_invoke(lua_State* handle, const char* name, _args&&... args) {
  lua_getglobal(handle, name);
  ((push_arg(handle, args)), ...);
}

template <typename _result_type>
static inline auto pop_result(lua_State* handle)
    -> std::enable_if_t<std::is_integral_v<_result_type>, lua_Integer> {
  return lua_tointeger(handle, -1);
}

template <typename _result_type>
static inline auto pop_result(lua_State* handle)
    -> std::enable_if_t<std::is_floating_point_v<_result_type>, lua_Number> {
  return lua_tonumber(handle, -1);
}

template <typename _result_type>
static inline auto pop_result(lua_State* handle)
    -> std::enable_if_t<std::is_same_v<const char*, _result_type>,
                        const char*> {
  return lua_tostring(handle, -1);
}

template <typename _result_type>
static inline auto pop_result(lua_State* handle)
    -> std::enable_if_t<std::is_same_v<void, _result_type>, void> {
  return;
}

template <typename _signature>
struct c_bind;

template <typename _ret, typename... _args>
struct c_bind<_ret(_args...)> {
  lua_State* _handle = nullptr;
  const char* _func_name = nullptr;
  _ret operator()(_args&&... args) {
    lua_invoke(_handle, _func_name, std::forward<_args>(args)...);
    lua_pcall(_handle, sizeof...(args), 1, 0);
    return pop_result<_ret>(_handle);
  }
};

}  // namespace details

#define WLUA_FUNCTION(fp) \
  ::wlua::details::lua_bind<::wlua::details::function<decltype(fp), fp>>

template <typename _bind>
void add_function(lua_State* handle, const char* name) {
  lua_pushcfunction(handle, &_bind::invoke);
  lua_setglobal(handle, name);
}

template <typename _signature>
details::c_bind<_signature> get_function(lua_State* handle, const char* name) {
  return details::c_bind<_signature>{handle, name};
}

}  // namespace wlua
