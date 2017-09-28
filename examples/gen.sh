ROOT=`pwd`

# empty
echo "Generate files for Empty.thrift"
cd $ROOT/empty && thrift --gen cpp -out gen-cpp Empty.thrift

# proxy
echo "Generate files for Proxy.thrift"
cd $ROOT/proxy && thrift --gen cpp -out gen-cpp Proxy.thrift

# parallel
echo "Generate files for Parallel.thrift"
cd $ROOT/parallel && thrift --gen cpp -out gen-cpp Parallel.thrift

# pbrpc
echo "Generate files for Proxy.proto"
cd $ROOT/pbrpc && protoc --cpp_out=. Proxy.proto

# flatbuffers
if [ `command -v flatc` ]; then
    echo "Generate files for table.fbs"
    cd $ROOT/flatbuffers && flatc --cpp table.fbs
fi
