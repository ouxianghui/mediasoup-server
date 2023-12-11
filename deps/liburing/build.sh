cur_dir=$(cd "$(dirname "$0")"; pwd)
parent_dir=$(dirname $(pwd))
main_dir=$(dirname ${parent_dir})

echo "cur_dir = ${cur_dir}"
echo "parent_dir = ${parent_dir}"
echo "main_dir = ${main_dir}"

/bin/rm -rf build

chmod 777 config

./configure --prefix=${main_dir}/release

make -j4
make install
