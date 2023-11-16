cur_dir=$(cd "$(dirname "$0")"; pwd)
parent_dir=$(dirname $(pwd))
cd script
 ./buildsigslot.sh
 ./buildopenssl.sh
 ./buildoatpp.sh
 ./buildoatppwebsocket.sh
 ./buildoatppopenssl.sh
 ./builduv.sh
 ./buildjson.sh
 ./buildcatch.sh
 ./buildsrtp.sh
 ./buildusrsctp.sh
 ./buildabsl.sh
 ./buildsdp.sh
 ./buildconcurrentqueue.sh

 #./buildwebrtc.sh


