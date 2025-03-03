cmake_minimum_required(VERSION 3.31)
project(qt-grpc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Core Protobuf Grpc)

set(proto_files "${CMAKE_CURRENT_LIST_DIR}/proto/clientguide.proto")

qt_standard_project_setup()
qt_add_executable(qt-grpc main.cpp)

qt_add_protobuf(qt-grpc PROTO_FILES ${proto_files})
qt_add_grpc(qt-grpc CLIENT PROTO_FILES ${proto_files})

target_link_libraries(qt-grpc PRIVATE Qt::Core Qt::Protobuf Qt::Grpc)

include(GNUInstallDirs)
install(TARGETS qt-grpc
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
