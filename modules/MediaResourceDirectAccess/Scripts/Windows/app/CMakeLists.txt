# Minimum CMake required
cmake_minimum_required(VERSION 3.0)

OPTION(WINDOWS_OS
       "Use Windows OS"
       ON
)

IF(WINDOWS_OS)
  ADD_DEFINITIONS("-D_WINDOWS_OS_")
ENDIF()


# Project
project(MRDASampleApp)

AUX_SOURCE_DIRECTORY(../../../sample_app SAMPLE_SRC)

set(TARGET MRDASampleApp)

set(DIR_SRC
  ${SAMPLE_SRC}
  )

link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../lib/install/lib/)

# Targets
add_executable(${TARGET}
  ${DIR_SRC}
  )

target_link_libraries(${TARGET} libWinGuest)

include_directories(
${CMAKE_CURRENT_SOURCE_DIR}/../lib/install/include
)