ROOT=`pwd`

# empty-thrift
cd $ROOT/empty-thrift && thrift --gen cpp -out gen-cpp Empty.thrift

# empty-proto
cd $ROOT/empty-proto && protoc --cpp_out=. Empty.proto
