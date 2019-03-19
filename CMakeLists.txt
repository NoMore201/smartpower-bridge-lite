cmake_minimum_required(VERSION 3.7)
project(smartpower)

if(DEBUG)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g -Wall -std=c++11")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -g --pedantic -Wall --std=c99")
else(DEBUG)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -Wall -std=c++11")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 --pedantic -Wall --std=c99")
endif(DEBUG)

find_library(LIBHIDAPI hidapi-libusb)

add_subdirectory(src)
