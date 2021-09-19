function(add_example arg1)
  add_executable(${arg1} "${arg1}.cpp")
  target_link_libraries(${arg1} PRIVATE wlua)
endfunction()

add_example(luabind)
add_example(cbind)

file(COPY "scripts" DESTINATION ${CMAKE_BINARY_DIR})
