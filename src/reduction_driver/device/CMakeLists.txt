#
# GauXC Copyright (c) 2020-2024, The Regents of the University of California,
# through Lawrence Berkeley National Laboratory (subject to receipt of
# any required approvals from the U.S. Dept. of Energy). All rights reserved.
#
# See LICENSE.txt for details
#
target_sources( gauxc PRIVATE
  device_reduction_driver.cxx
)

if( GAUXC_ENABLE_NCCL )
    target_sources( gauxc PRIVATE nccl_reduction_driver.cxx )
    message( STATUS "NCCL Has Been Enabled" )
    string(REPLACE "compilers/bin/nvcc" "cmake" NVHPC_CMAKE_DIR ${CMAKE_CUDA_COMPILER})
    set(CMAKE_PREFIX_PATH ${NVHPC_CMAKE_DIR})
    message(STATUS "NVHPC CMake Directory: ${NVHPC_CMAKE_DIR}")
    message(STATUS "CMake Prefix Path set to: ${CMAKE_PREFIX_PATH}")
    find_package( NVHPC REQUIRED COMPONENTS NCCL CUDA MATH)
    find_library(NCCL_LIBRARY
    NAMES nccl
    HINTS ${NVHPC_NCCL_LIBRARY_DIR})
    string(REGEX REPLACE "(.+)(\/lib)$" "\\1/include" NCCL_INCLUDE_DIR ${NVHPC_NCCL_LIBRARY_DIR})
    target_link_libraries( gauxc PUBLIC NVHPC::NCCL )
    set(GAUXC_HAS_NCCL TRUE CACHE BOOL "GauXC has NCCL" FORCE)
endif()
