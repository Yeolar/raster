ROOT=`pwd`

# empty
cd $ROOT/empty && thrift --gen cpp -out gen-cpp Empty.thrift

# proxy
cd $ROOT/proxy && thrift --gen cpp -out gen-cpp Proxy.thrift

# parallel
cd $ROOT/parallel && thrift --gen cpp -out gen-cpp Parallel.thrift

# pbrpc
cd $ROOT/pbrpc && protoc --cpp_out=. Empty.proto
