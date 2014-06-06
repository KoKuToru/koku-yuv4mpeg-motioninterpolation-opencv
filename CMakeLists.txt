cmake_minimum_required(VERSION 2.8)
find_package( OpenCV REQUIRED )
project(koku-yuv4mpeg-motioninterpolation)
ADD_EXECUTABLE(koku-yuv4mpeg-motioninterpolation main.cpp yuv4mpeg.cpp popen_istream.cpp oflow.cpp pstream.h)
set(CMAKE_CXX_FLAGS "-std=c++0x -ldl -lrt -lGL -lfftw3 -lSDL -O3 -rdynamic -pthread")
target_link_libraries(koku-yuv4mpeg-motioninterpolation pthread ${OpenCV_LIBS})

