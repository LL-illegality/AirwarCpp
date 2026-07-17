cmake_minimum_required(VERSION 3.20)
project(AirwarCPP LANGUAGES CXX)

if(POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
endif()

add_subdirectory(AirwarCPP)
