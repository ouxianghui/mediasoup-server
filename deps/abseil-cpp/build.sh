cur_dir=$(cd "$(dirname "$0")"; pwd)
parent_dir=$(dirname $(pwd))
main_dir=$(dirname ${parent_dir})

echo "cur_dir = ${cur_dir}"
echo "parent_dir = ${parent_dir}"
echo "main_dir = ${main_dir}"

/bin/rm -rf build
/bin/rm -rf CMakeCache.txt
/bin/rm -rf cmake_install.cmake

mkdir build
cd build
cmake -DCMAKE_CXX_STANDARD=17 -DCMAKE_INSTALL_PREFIX=${main_dir}/release ..
make
make install
/bin/cp -rf absl/strings/*.a ${main_dir}/release/lib
/bin/cp -rf absl/base/*.a ${main_dir}/release/lib
/bin/cp -rf absl/types/*.a ${main_dir}/release/lib
/bin/cp -rf absl/hash/*.a ${main_dir}/release/lib
/bin/cp -rf absl/container/*.a ${main_dir}/release/lib
