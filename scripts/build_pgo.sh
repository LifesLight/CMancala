set -e

CC="${CC:-/usr/bin/gcc-14}"
CXX="${CXX:-/usr/bin/g++-14}"
BUILD_DIR="${BUILD_DIR:-build-pgo}"
JOBS="${JOBS:-$(nproc)}"

echo "CC=${CC}"
echo "CXX=${CXX}"
echo "BUILD_DIR=${BUILD_DIR}"

cmake -S . -B "${BUILD_DIR}" \
  -DCMAKE_C_COMPILER="${CC}" \
  -DCMAKE_CXX_COMPILER="${CXX}" \
  -DENABLE_PGO=ON -DPGO_STAGE=generate

cmake --build "${BUILD_DIR}" -j "${JOBS}"

echo
echo "==> Run the instrumented binary to collect profile data now."
echo "Running: ${BUILD_DIR}/Mancala --benchmark"
"${BUILD_DIR}/Mancala" --benchmark

cmake -S . -B "${BUILD_DIR}" \
  -DCMAKE_C_COMPILER="${CC}" \
  -DCMAKE_CXX_COMPILER="${CXX}" \
  -DENABLE_PGO=ON -DPGO_STAGE=use

cmake --build "${BUILD_DIR}" -j "${JOBS}"

echo
echo "PGO optimized binary built at: ${BUILD_DIR}/Mancala"
