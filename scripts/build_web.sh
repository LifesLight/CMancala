set -e

source ~/coding/emsdk/emsdk_env.sh

# --- 1. Standard Release Build (No Dev Features) ---
echo "Building Standard Release..."
rm -rf ~/coding/CMancala/build_web
mkdir -p ~/coding/CMancala/build_web
cd ~/coding/CMancala/build_web

emcmake cmake .. -DCMAKE_BUILD_TYPE=Release -DENABLE_WEB_DEV=OFF
make

# --- 2. Dev Build (With Cache Settings & Memory Growth) ---
echo "Building Dev Release..."
rm -rf ~/coding/CMancala/build_web_dev
mkdir -p ~/coding/CMancala/build_web_dev
cd ~/coding/CMancala/build_web_dev

# We use a different output name (e.g., MancalaDev) or just keep it separate folders
emcmake cmake .. -DCMAKE_BUILD_TYPE=Release -DENABLE_WEB_DEV=ON
make
