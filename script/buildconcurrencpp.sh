cur_dir=$(cd "$(dirname "$0")"; pwd)
parent_dir=$(dirname $(pwd))

cd ${parent_dir}/deps/concurrencpp
./build.sh