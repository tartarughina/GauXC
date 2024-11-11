#!/bin/bash

cmake -DGAUXC_ENABLE_NCCL=ON \
-DCMAKE_INSTALL_PREFIX=/home/tartarughina/GauXC-def \
-DCMAKE_BUILD_TYPE="Release" \
-DCMAKE_CXX_FLAGS_RELEASE="-O2" \
-DGAUXC_ENABLE_HDF5=ON \
-DGAUXC_ENABLE_CUDA=ON \
-DGAUXC_ENABLE_TESTS=ON \
-DGAUXC_ENABLE_HOST=ON \
-DGAUXC_ENABLE_OPENMP=OFF \
-DBLAS_LIBRARIES=$NVIDIA_PATH/compilers/lib/libblas.so \
-DMAGMA_ROOT_DIR=/soft/libraries/math_libs/magma-2.8.0/PrgEnv-gnu \
-DCMAKE_CUDA_ARCHITECTURES=80 \
-DCMAKE_CXX_COMPILER="g++-12" \
-DCMAKE_C_COMPILER="gcc-12" \
-DCMAKE_CXX_STANDARD=17 \
..

# To run the test
mpirun -np 2 ./standalone_driver /home/tartarughina/GauXC/tests/ref_data/ut_input.inp
