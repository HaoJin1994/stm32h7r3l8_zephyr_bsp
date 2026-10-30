# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/home/ubuntu/work/aknw_h7rsx/hello_world")
  file(MAKE_DIRECTORY "/home/ubuntu/work/aknw_h7rsx/hello_world")
endif()
file(MAKE_DIRECTORY
  "/home/ubuntu/work/aknw_h7rsx/hello_world/build/hello_world"
  "/home/ubuntu/work/aknw_h7rsx/hello_world/build/_sysbuild/sysbuild/images/hello_world-prefix"
  "/home/ubuntu/work/aknw_h7rsx/hello_world/build/_sysbuild/sysbuild/images/hello_world-prefix/tmp"
  "/home/ubuntu/work/aknw_h7rsx/hello_world/build/_sysbuild/sysbuild/images/hello_world-prefix/src/hello_world-stamp"
  "/home/ubuntu/work/aknw_h7rsx/hello_world/build/_sysbuild/sysbuild/images/hello_world-prefix/src"
  "/home/ubuntu/work/aknw_h7rsx/hello_world/build/_sysbuild/sysbuild/images/hello_world-prefix/src/hello_world-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/ubuntu/work/aknw_h7rsx/hello_world/build/_sysbuild/sysbuild/images/hello_world-prefix/src/hello_world-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/ubuntu/work/aknw_h7rsx/hello_world/build/_sysbuild/sysbuild/images/hello_world-prefix/src/hello_world-stamp${cfgdir}") # cfgdir has leading slash
endif()
