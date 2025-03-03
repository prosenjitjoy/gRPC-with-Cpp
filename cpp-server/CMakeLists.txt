cmake_minimum_required(VERSION 3.31)

project(cpp-grpc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(protobuf)
find_package(gRPC)

set(proto_files "${CMAKE_CURRENT_LIST_DIR}/proto/clientguide.proto")
set(proto_out "${CMAKE_CURRENT_BINARY_DIR}")

set(generated_files
    "${proto_out}/clientguide.pb.h"
    "${proto_out}/clientguide.pb.cc"
    "${proto_out}/clientguide.grpc.pb.h"
    "${proto_out}/clientguide.grpc.pb.cc"
)

add_custom_command(
    OUTPUT ${generated_files}
    COMMAND protoc
    ARGS
        --grpc_out "${proto_out}"
        --cpp_out "${proto_out}"
        -I "${CMAKE_CURRENT_LIST_DIR}/proto"
        --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
        "${proto_files}"
    DEPENDS "${proto_files}"
)

add_executable(cpp-grpc main.cpp ${generated_files} ${proto_files})
target_include_directories(cpp-grpc PRIVATE ${proto_out})
target_link_libraries(cpp-grpc
    PRIVATE
        protobuf::libprotobuf
        gRPC::grpc++
)

include(GNUInstallDirs)
install(TARGETS cpp-grpc
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
