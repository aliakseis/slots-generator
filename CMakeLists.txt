cmake_minimum_required(VERSION 3.14)

project(slots-generator)

find_package(expat CONFIG REQUIRED)

add_executable ( slots-generator generator.cpp )
target_link_libraries ( slots-generator PRIVATE expat::expat )
