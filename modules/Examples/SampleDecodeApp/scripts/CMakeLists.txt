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

AUX_SOURCE_DIRECTORY(../src SAMPLE_SRC)

set(TARGET MRDASampleDecodeApp)

set(DIR_SRC
  ${SAMPLE_SRC}
  )

link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../MediaResourceDirectAccess/Scripts/Windows/lib/install/lib/)
link_directories($ENV{MSYS2_HOME}/ffmpeg-7.1/ffmpeg_install/bin/)

# Targets
add_executable(${TARGET}
  ${DIR_SRC}
  )

target_link_libraries(${TARGET} libWinGuest)
target_link_libraries(${TARGET} avformat avcodec)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../MediaResourceDirectAccess/Scripts/Windows/lib/install/lib/install/include
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../MediaResourceDirectAccess/utils
  $ENV{MSYS2_HOME}/ffmpeg-7.1/ffmpeg_install/include
)