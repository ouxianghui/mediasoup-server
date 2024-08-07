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
cmake -DCMAKE_INSTALL_PREFIX=${main_dir}/release ..
make -j8
make install