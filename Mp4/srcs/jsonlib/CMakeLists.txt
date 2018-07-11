# Copyright © 2016 Nokia Corporation and/or its subsidiary(-ies). All rights reserved. 
#


cmake_minimum_required(VERSION 3.2 FATAL_ERROR)

project( json LANGUAGES CXX )

set(JSON_SRCS
    json.cc
    )
    
set(JSON_HDRS
    json.hh
    )

add_library(json STATIC ${JSON_SRCS} ${JSON_HDRS})
set_property(TARGET json PROPERTY CXX_STANDARD 11)

add_library(json_for_lib OBJECT ${JSON_SRCS} ${JSON_HDRS})
set_property(TARGET json_for_lib PROPERTY CXX_STANDARD 11)
set_property(TARGET json_for_lib PROPERTY POSITION_INDEPENDENT_CODE 1)

add_library(json_for_dll OBJECT ${JSON_SRCS} ${JSON_HDRS})
set_property(TARGET json_for_dll PROPERTY CXX_STANDARD 11)
set_property(TARGET json_for_dll PROPERTY POSITION_INDEPENDENT_CODE 1)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set_property(TARGET json PROPERTY COMPILE_OPTIONS -Wno-missing-declarations)
endif()
