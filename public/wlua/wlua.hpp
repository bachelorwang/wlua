extern "C" {
#include <lua5.3/lauxlib.h>
#include <lua5.3/lua.h>
#include <lua5.3/lualib.h>
}
#include <tuple>
#include <type_traits>

namespace wlua {

namespace details {

template <typename TSignature, TSignature* TFp>
struct function;

template <typename TRet, typename... TArgs, TRet (*TFp)(TArgs...)>
struct function<TRet(TArgs...), TFp> {
  using result_type = TRet;
  using parameter_types = std::tuple<TArgs...>;
  static constexpr size_t parameter_count = sizeof...(TArgs);
  using type = TRet (*)(TArgs...);
  static constexpr TRet (*pointer)(TArgs...) = TFp;
};

template <typename TArg>
inline auto pop_arg(TArg& arg, lua_State* handle, size_t i)
    -> std::enable_if_t<std::is_integral_v<TArg>, void> {
  arg = lua_tointeger(handle, i);
}

template <typename TArg>
inline auto pop_arg(TArg& arg, lua_State* handle, size_t i)
    -> std::enable_if_t<std::is_floating_point_v<TArg>, void> {
  arg = lua_tonumber(handle, i);
}

inline auto pop_arg(const char*& arg, lua_State* handle, size_t i) {
  arg = lua_tostring(handle, i);
}

template <typename TArgs, size_t... TIndexes>
inline void pop_all_args(lua_State* handle,
                         TArgs& args,
                         std::index_sequence<TIndexes...>) {
  (pop_arg(std::get<TIndexes>(args), handle, TIndexes + 1), ...);
}

template <typename TFunction, typename>
struct invoker;

template <typename TFunction>
struct invoker<TFunction, std::true_type> {
  template <size_t... TIndexes>
  static int invoke(lua_State* handle,
                    typename TFunction::parameter_types&& args,
                    std::index_sequence<TIndexes...>) {
    (*TFunction::pointer)(std::get<TIndexes>(args)...);
    return 0;
  }
};

template <typename TFunction>
struct invoker<TFunction, std::false_type> {
  template <size_t... TIndexes>
  static int invoke(lua_State* handle,
                    typename TFunction::parameter_types&& args,
                    std::index_sequence<TIndexes...>) {
    auto result = (*TFunction::pointer)(std::get<TIndexes>(args)...);
    lua_push_result(handle, result);
    return 1;
  }

 private:
  template <typename TType>
  static inline auto lua_push_result(lua_State* handle, TType result)
      -> std::enable_if_t<std::is_integral_v<TType>> {
    lua_pushinteger(handle, result);
  }
};

template <typename TFunction>
struct lua_bind {
  static int invoke(lua_State* handle) {
    typename TFunction::parameter_types args;
    constexpr auto indexes =
        std::make_index_sequence<TFunction::parameter_count>{};
    pop_all_args(handle, args, indexes);
    using is_void =
        typename std::is_same<typename TFunction::result_type, void>::type;
    using TInvoker = invoker<TFunction, is_void>;
    return TInvoker::invoke(handle, std::move(args), indexes);
  }
};

inline void push_arg(lua_State* handle, lua_Integer arg) {
  lua_pushinteger(handle, arg);
}

inline void push_arg(lua_State* handle, lua_Number arg) {
  lua_pushnumber(handle, arg);
}

inline void push_arg(lua_State* handle, const char* arg) {
  lua_pushstring(handle, arg);
}

template <typename... TArgs>
inline void lua_invoke(lua_State* handle, const char* name, TArgs&&... args) {
  lua_getglobal(handle, name);
  ((push_arg(handle, args)), ...);
}

template <typename TResultType>
inline auto pop_result(lua_State* handle)
    -> std::enable_if_t<std::is_integral_v<TResultType>, lua_Integer> {
  return lua_tointeger(handle, -1);
}

template <typename TResultType>
inline auto pop_result(lua_State* handle)
    -> std::enable_if_t<std::is_floating_point_v<TResultType>, lua_Number> {
  return lua_tonumber(handle, -1);
}

template <typename TResultType>
inline auto pop_result(lua_State* handle)
    -> std::enable_if_t<std::is_same_v<const char*, TResultType>, const char*> {
  return lua_tostring(handle, -1);
}

template <typename TResultType>
inline auto pop_result(lua_State* handle)
    -> std::enable_if_t<std::is_same_v<void, TResultType>, void> {
  return;
}

template <typename TSignature>
struct c_bind;

template <typename TRet, typename... TArgs>
struct c_bind<TRet(TArgs...)> {
  lua_State* handle_ = nullptr;
  const char* func_name_ = nullptr;
  TRet operator()(TArgs&&... args) {
    lua_invoke(handle_, func_name_, std::forward<TArgs>(args)...);
    lua_pcall(handle_, sizeof...(args), 1, 0);
    return pop_result<TRet>(handle_);
  }
};

}  // namespace details

#define WLUA_FUNCTION(fp) \
  ::wlua::details::lua_bind<::wlua::details::function<decltype(fp), fp>>

template <typename TBind>
void add_function(lua_State* handle, const char* name) {
  lua_pushcfunction(handle, &TBind::invoke);
  lua_setglobal(handle, name);
}

template <typename TSignature>
details::c_bind<TSignature> get_function(lua_State* handle, const char* name) {
  return details::c_bind<TSignature>{handle, name};
}

}  // namespace wlua
