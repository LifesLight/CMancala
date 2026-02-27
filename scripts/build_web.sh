source ~/coding/emsdk/emsdk_env.sh

echo "Building Standard Release..."
rm -rf ~/coding/CMancala/build_web
mkdir -p ~/coding/CMancala/build_web
cd ~/coding/CMancala/build_web

emcmake cmake .. -DCMAKE_BUILD_TYPE=Release
make