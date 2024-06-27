# Minimum CMake required
cmake_minimum_required(VERSION 3.0)

OPTION(WINDOWS_OS
       "Use Windows OS"
       ON
)

IF(WINDOWS_OS)
  ADD_DEFINITIONS("-D_WINDOWS_OS_" "-DMRDALIBRARY_EXPORTS")
ENDIF()


# Project
project(MRDAWinGuest)

AUX_SOURCE_DIRECTORY(../../../API API_SRC)
AUX_SOURCE_DIRECTORY(../../../SHMemory SHMEMORY_SRC)
AUX_SOURCE_DIRECTORY(../../../utils UTILS_SRC)
AUX_SOURCE_DIRECTORY(../../../WinGuest WINGUEST_SRC)

set(TARGET libWinGuest)

# Protobuf
set(protobuf_MODULE_COMPATIBLE TRUE)
# find_package(Protobuf REQUIRED)
find_package(Protobuf CONFIG REQUIRED)
message(STATUS "Using protobuf ${protobuf_VERSION}")

# Protobuf-compiler
set(_PROTOBUF_PROTOC $<TARGET_FILE:protobuf::protoc>)
# gRPC
find_package(gRPC CONFIG REQUIRED)
message(STATUS "Using gRPC ${gRPC_VERSION}")
set(_GRPC_GRPCPP gRPC::grpc++)
set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:gRPC::grpc_cpp_plugin>)

# List of proto files
set(proto_files
    "MRDAService.proto"
    "MRDAServiceManager.proto")

# Base path for proto files
set(proto_path "${PROJECT_SOURCE_DIR}/../../../protos")

set(all_proto_srcs "")
set(all_grpc_srcs "")

# Process each .proto file
foreach(proto_file ${proto_files})
    get_filename_component(full_proto "${proto_path}/${proto_file}" ABSOLUTE)
    get_filename_component(proto_dir "${full_proto}" PATH)

    # Extract basename for use in generated filenames
    get_filename_component(proto_name "${proto_file}" NAME_WE)

    # Generated sources
    set(proto_srcs "${proto_path}/${proto_name}.pb.cc")
    set(proto_hdrs "${proto_path}/${proto_name}.pb.h")
    set(grpc_srcs "${proto_path}/${proto_name}.grpc.pb.cc")
    set(grpc_hdrs "${proto_path}/${proto_name}.grpc.pb.h")

    # append sources
    list(APPEND all_proto_srcs "${proto_srcs}")
    list(APPEND all_grpc_srcs "${grpc_srcs}")

    add_custom_command(
        OUTPUT "${proto_srcs}" "${proto_hdrs}" "${grpc_srcs}" "${grpc_hdrs}"
        COMMAND ${_PROTOBUF_PROTOC}
        ARGS --grpc_out "${proto_path}"
            --cpp_out "${proto_path}"
            -I "${proto_dir}"
            --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
            "${full_proto}"
        DEPENDS "${full_proto}")
endforeach()

set(DIR_SRC
  ${API_SRC}
  ${SHMEMORY_SRC}
  ${UTILS_SRC}
  ${WINGUEST_SRC}
  )

# Targets
add_library(${TARGET}
  SHARED
  ${all_proto_srcs}
  ${all_grpc_srcs}
  ${DIR_SRC}
  )

target_link_libraries(${TARGET} setupapi)

target_link_libraries(${TARGET}
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF})

include_directories(
  ${proto_path}
  )


INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../../API/MediaResourceDirectAccessAPI.h DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/install/include)
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/build/Release/libWinGuest.dll DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/install/lib)
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/build/Release/libWinGuest.lib DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/install/lib)