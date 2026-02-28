rm -r ~/coding/CMancala/build
mkdir ~/coding/CMancala/build
cd ~/coding/CMancala/build
cmake .. -DCMAKE_C_COMPILER="/usr/bin/gcc-14" -DCMAKE_CXX_COMPILER="/usr/bin/g++-14" -DENABLE_LZ4="ON"
make