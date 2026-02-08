cmake_minimum_required(VERSION 3.30)
project(TaskA3)

set(CMAKE_CXX_STANDARD 20)

add_executable(TaskA3 main.cpp
        RandomStreamGen.h
        RandomStreamGen.cpp
        MurmurHash3.h
        MurmurHash3.cpp
        HashFuncGen.h
        HashFuncGen.cpp
        HyperLogLog.h
        HyperLogLog.cpp
        HyperLogLogMax.h
        HyperLogLogMax.cpp)
