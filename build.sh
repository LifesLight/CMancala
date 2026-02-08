rm -r /home/alex/coding/CMancala/build
mkdir /home/alex/coding/CMancala/build
cd /home/alex/coding/CMancala/build
cmake .. -DCMAKE_C_COMPILER="/usr/bin/gcc-14" -DCMAKE_CXX_COMPILER="/usr/bin/g++-14"
make