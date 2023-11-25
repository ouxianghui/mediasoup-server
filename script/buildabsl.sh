cur_dir=$(cd "$(dirname "$0")"; pwd)
parent_dir=$(dirname $(pwd))

cd ${parent_dir}/deps/abseil-cpp
./build.sh