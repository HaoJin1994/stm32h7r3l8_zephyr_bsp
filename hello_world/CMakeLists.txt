# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.28.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(hello_world)

target_sources(app PRIVATE 
    src/main.c
    #src/flash_rw.c
    #src/sha256.c
)


target_include_directories(app PRIVATE
    include
)
