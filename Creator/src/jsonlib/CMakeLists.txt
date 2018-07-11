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

make_library(json ${JSON_SRCS} ${JSON_HDRS})
set_property(TARGET json PROPERTY CXX_STANDARD 11)
