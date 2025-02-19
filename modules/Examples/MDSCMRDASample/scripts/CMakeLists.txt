# Minimum CMake required
cmake_minimum_required(VERSION 3.0)

OPTION(WINDOWS_OS
       "Use Windows OS"
       ON
)

IF(WINDOWS_OS)
  ADD_DEFINITIONS("-D_WINDOWS_OS_")
ENDIF()

OPTION(ENABLE_TRACE
  "Use trace log"
  OFF
)

OPTION(DUMP_RGBA
  "Dump captured rgba results"
  OFF
)

IF(ENABLE_TRACE)
  ADD_DEFINITIONS("-D_ENABLE_TRACE_")
ENDIF()

IF(DUMP_RGBA)
  ADD_DEFINITIONS("-DDUMP_RGBA")
ENDIF()

# Project
project(MDSCMRDASampleApp)

AUX_SOURCE_DIRECTORY(../src SAMPLE_SRC)

set(TARGET MDSCMRDASampleApp)

set(DIR_SRC
  ${SAMPLE_SRC}
  )

link_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../MediaResourceDirectAccess/Scripts/Windows/lib/install/lib/
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../multidispscreencap/bin
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../multidispscreencap/MDSCSample/deps/ffmpeg-6.1.1-full_build-shared/lib
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../../dgpu/mediasample/MediaSample/deps/lib
  )

# Targets
add_executable(${TARGET}
  ${DIR_SRC}
  )

target_link_libraries(${TARGET} QESLib D3D11 libWinGuest MultiDisplayScreenCapture avcodec avdevice avfilter avformat avutil postproc swresample swscale)

include_directories(
${CMAKE_CURRENT_SOURCE_DIR}/../../../MediaResourceDirectAccess/Scripts/Windows/lib/install/include
${CMAKE_CURRENT_SOURCE_DIR}/../../../MediaResourceDirectAccess/utils
${CMAKE_CURRENT_SOURCE_DIR}/../../../multidispscreencap/MultiDisplayScreenCapture
${CMAKE_CURRENT_SOURCE_DIR}/../../../multidispscreencap/MDSCSample/deps/nlohmann
${CMAKE_CURRENT_SOURCE_DIR}/../../../multidispscreencap/MDSCSample/deps/ffmpeg-6.1.1-full_build-shared/include
${CMAKE_CURRENT_SOURCE_DIR}/../../../../dgpu/mediasample/MediaSample/deps/include
)