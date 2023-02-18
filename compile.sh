#!/bin/bash

BUILD_TYPE=Release
if [ $# == 1 ]; then
    BUILD_TYPE=${1}
    echo "build with ${BUILD_TYPE}"
fi

DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

echo $BUILD_TYPE

cd $DIR
mkdir -p build/${BUILD_TYPE}
cd build/${BUILD_TYPE}
cmake -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DCMAKE_RUNTIME_OUTPUT_DIRECTORY=${DIR} -DCMAKE_CXX_FLAGS="-Wno-error=deprecated-copy" ../..
make -j$(nproc)
