cur_dir=$(cd "$(dirname "$0")"; pwd)
parent_dir=$(dirname $(pwd))

cd ${parent_dir}/deps/oatpp/
./build.sh