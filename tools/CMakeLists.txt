cmake_minimum_required(VERSION 3.25)
project(MotoGPTools LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
find_package(rexglue 0.10.0 EXACT REQUIRED CONFIG)
add_executable(motogp_inspect inspect.cpp)
target_link_libraries(motogp_inspect PRIVATE rex::runtime)
if(WIN32)
  add_custom_command(TARGET motogp_inspect POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      $<TARGET_FILE:rex::runtime> $<TARGET_FILE_DIR:motogp_inspect>)
endif()
