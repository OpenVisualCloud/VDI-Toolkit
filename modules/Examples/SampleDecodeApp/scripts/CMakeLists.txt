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

IF(ENABLE_TRACE)
  ADD_DEFINITIONS("-D_ENABLE_TRACE_")
ENDIF()


# Project
project(MRDASampleDecodeApp)

set(SAMPLE_HW_SRC ../src/sample_decode.cpp)
set(SAMPLE_SW_SRC ../src/sample_decode_sw.cpp)

set(TARGET_HW MRDASampleDecodeApp)
set(TARGET_SW MRDASampleDecodeAppSW)

set(DIR_SRC_HW
  ${SAMPLE_HW_SRC}
  )
set(DIR_SRC_SW
  ${SAMPLE_SW_SRC}
  )

link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../MediaResourceDirectAccess/Scripts/Windows/lib/install/lib/)
link_directories($ENV{MSYS2_HOME}/ffmpeg-7.1/ffmpeg_install/bin/)

# Targets
add_executable(${TARGET_HW}
  ${DIR_SRC_HW}
  )
add_executable(${TARGET_SW}
  ${DIR_SRC_SW}
  )

target_link_libraries(${TARGET_HW} libWinGuest)
target_link_libraries(${TARGET_HW} avformat avcodec)

target_link_libraries(${TARGET_SW} libWinGuest)
target_link_libraries(${TARGET_SW} avformat avcodec avutil swscale)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../MediaResourceDirectAccess/Scripts/Windows/lib/install/lib/install/include
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../MediaResourceDirectAccess/utils
  $ENV{MSYS2_HOME}/ffmpeg-7.1/ffmpeg_install/include
)