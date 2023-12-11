cur_dir=$(cd "$(dirname "$0")"; pwd)
parent_dir=$(dirname $(pwd))
cd script
  ./buildabsl.sh
  ./buildsigslot.sh
  ./buildconcurrentqueue.sh
  ./buildopenssl.sh
  ./buildjson.sh
  ./buildcatch.sh
  ./buildoatpp.sh
  ./buildliburing.sh
  ./buildoatppwebsocket.sh
  ./buildoatppopenssl.sh
  ./builduv.sh
  ./buildsrtp.sh
  ./buildusrsctp.sh
  ./buildflatbuffers.sh
  #./buildwebrtc.sh
  #./buildworker.sh


