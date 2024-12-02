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
project(MRDASampleEncodeApp)

AUX_SOURCE_DIRECTORY(../src SAMPLE_SRC)

set(TARGET MRDASampleEncodeApp)

set(DIR_SRC
  ${SAMPLE_SRC}
  )

link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../MediaResourceDirectAccess/Scripts/Windows/lib/install/lib/)

# Targets
add_executable(${TARGET}
  ${DIR_SRC}
  )

target_link_libraries(${TARGET} libWinGuest)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../MediaResourceDirectAccess/Scripts/Windows/lib/install/lib/install/include
  ${CMAKE_CURRENT_SOURCE_DIR}/../../../MediaResourceDirectAccess/utils
)