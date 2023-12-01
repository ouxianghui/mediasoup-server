cur_dir=$(cd "$(dirname "$0")"; pwd)
parent_dir=$(dirname $(pwd))

cd ${parent_dir}/deps/flatbuffers

./build.sh

mkdir ${parent_dir}/release/include/fbs

${parent_dir}/release/bin/flatc --cpp --cpp-field-case-style lower --reflect-names --scoped-enums --filename-suffix '' -o ${parent_dir}/release/include/fbs/FBS ${parent_dir}/worker/fbs/*.fbs
