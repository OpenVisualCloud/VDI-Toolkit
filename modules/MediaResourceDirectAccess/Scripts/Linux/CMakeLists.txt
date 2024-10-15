# Minimum CMake required
cmake_minimum_required(VERSION 3.0)

OPTION(LINUX_OS
       "Use Linux OS"
       ON
)

IF(LINUX_OS)
  ADD_DEFINITIONS("-D_LINUX_OS_ -Werror")
ENDIF()


# Project
project(MRDAHostService)

AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/../../HostService SERVICE_SRC)
AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/../../HostService/EncodeService SERVICE_ENCODE_SRC)
AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/../../HostService/EncodeService/VPLEncode SERVICE_VPLENCODE_SRC)
AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/../../HostService/EncodeService/FFmpegEncode SERVICE_FFMPEGENCODE_SRC)
AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/../../utils UTILS_SRC)

set(TARGET HostService)

# Set default build type to RelWithDebInfo if not specified
if(NOT CMAKE_BUILD_TYPE)
  message(
    STATUS "Default CMAKE_BUILD_TYPE not set using Release with Debug Info")
  set(CMAKE_BUILD_TYPE
      "RelWithDebInfo"
      CACHE
        STRING
        "Choose build type from: None Debug Release RelWithDebInfo MinSizeRel"
        FORCE)
endif()


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

set(proto_path "${CMAKE_CURRENT_SOURCE_DIR}/../../protos")

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
  ${SERVICE_SRC}
  ${SERVICE_ENCODE_SRC}
  ${SERVICE_VPLENCODE_SRC}
  ${SERVICE_FFMPEGENCODE_SRC}
  ${UTILS_SRC}
  )

# Targets
add_executable(${TARGET}
  ${all_proto_srcs}
  ${all_grpc_srcs}
  ${DIR_SRC}
  )

target_link_libraries(${TARGET}
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF})

OPTION(VPL_SUPPORT
  "Use VPL support"
  OFF
)

OPTION(FFMPEG_SUPPORT
  "Use FFMPEG support"
  ON
)

OPTION(ENABLE_TRACE
  "Use trace log"
  OFF
)

IF(ENABLE_TRACE)
  message(STATUS "Using trace log")
  ADD_DEFINITIONS("-D_ENABLE_TRACE_")
ENDIF(ENABLE_TRACE)

IF(VPL_SUPPORT)
  ADD_DEFINITIONS("-D_VPL_SUPPORT_")
  find_package(VPL REQUIRED)
  target_link_libraries(${TARGET} VPL::dispatcher)
  set(LIBVA_SUPPORT
    ON
    CACHE BOOL "Enable hardware support.")
  if(LIBVA_SUPPORT)
      find_package(PkgConfig REQUIRED)
      # note: pkg-config version for libva is *API* version
      pkg_check_modules(PKG_LIBVA IMPORTED_TARGET libva>=1.2)
      pkg_check_modules(PKG_LIBVA_DRM IMPORTED_TARGET libva-drm>=1.2)
      if(PKG_LIBVA_FOUND)
          target_compile_definitions(${TARGET} PUBLIC -DLIBVA_SUPPORT)
          set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
          set(THREADS_PREFER_PTHREAD_FLAG TRUE)
          find_package(Threads REQUIRED)
          target_link_libraries(${TARGET} PkgConfig::PKG_LIBVA
                              PkgConfig::PKG_LIBVA_DRM Threads::Threads)
          target_include_directories(${TARGET} PUBLIC ${PKG_LIBVA_INCLUDE_DIRS})
      else()
          message(
          SEND_ERROR
              "libva not found: set LIBVA_SUPPORT=OFF to build ${TARGET} without libva support"
          )
      endif()
  else()
      message(STATUS "Building ${TARGET} without hardware support")
  endif()
ENDIF(VPL_SUPPORT)

IF(FFMPEG_SUPPORT)
  ADD_DEFINITIONS("-D_FFMPEG_SUPPORT_")
  target_link_libraries(${TARGET} avcodec avutil swscale)
ENDIF(FFMPEG_SUPPORT)

include_directories(
  ${proto_path}
  )
