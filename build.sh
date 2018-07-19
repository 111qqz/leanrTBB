#!/bin/bash

TARGET=$1
echo ${TARGET}
g++ -std=c++11 ${TARGET}.cpp -L/home/sensetime/workspace/graph/3rdparty/tbb/lib/intel64/gcc4.7 -ltbb -o ${TARGET}.out

