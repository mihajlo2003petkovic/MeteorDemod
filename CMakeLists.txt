cmake_minimum_required(VERSION 3.5)

include(ExternalProject)

project(Meteordemod LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

if (NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "" FORCE)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")


if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.9.2")
    message(FATAL_ERROR "Insufficient gcc version, minimum required: 4.9.2, provided: " + "${CMAKE_CXX_COMPILER_VERSION}")
  endif()
endif()


ExternalProject_Add(sgp4
  #GIT_REPOSITORY    https://github.com/.....
  #GIT_TAG           master
  SOURCE_DIR        "${CMAKE_CURRENT_SOURCE_DIR}/external/sgp4"
  BINARY_DIR        "${CMAKE_BINARY_DIR}/sgp4-build"
  INSTALL_COMMAND   cmake -E echo "Skipping install step."
  CMAKE_ARGS        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
)

ExternalProject_Add(libcorrect
  #GIT_REPOSITORY    https://github.com/.....
  #GIT_TAG           master
  SOURCE_DIR        "${CMAKE_CURRENT_SOURCE_DIR}/external/libcorrect"
  BINARY_DIR        "${CMAKE_BINARY_DIR}/libcorrect-build"
  INSTALL_COMMAND   cmake -E echo "Skipping install step."
  CMAKE_ARGS        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
)

find_package(OpenCV)

link_directories(
    ${CMAKE_BINARY_DIR}/sgp4-build/libsgp4
    ${CMAKE_BINARY_DIR}/libcorrect-build/lib
)

add_definitions(-D_USE_MATH_DEFINES -D_SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING)

add_executable(meteordemod 
    main.cpp
    imageproc/spreadimage.cpp
    imageproc/spreadimage.h
    imageproc/threatimage.cpp
    imageproc/threatimage.h
    decoder/bitio.cpp
    decoder/bitio.h
    decoder/correlation.cpp
    decoder/correlation.h
    decoder/meteorimage.cpp
    decoder/meteorimage.h
    decoder/packetparser.cpp
    decoder/packetparser.h
    decoder/reedsolomon.cpp
    decoder/reedsolomon.h
    decoder/viterbi.cpp
    decoder/viterbi.h
    decoder/deinterleaver.cpp
    decoder/meteordecoder.cpp
    decoder/meteordecoder.h
    common/settings.cpp
    common/settings.h
    common/version.h
    tools/matrix.cpp
    tools/matrix.h
    tools/tlereader.cpp
    tools/tlereader.h
    tools/vector.cpp
    tools/vector.h
    tools/pixelgeolocationcalculator.cpp
    tools/pixelgeolocationcalculator.h
    tools/databuffer.cpp
    tools/databuffer.h
    tools/iniparser.cpp
    tools/iniparser.h
    tools/threadpool.cpp
    tools/threadpool.h
    GIS/shapereader.cpp
    GIS/shapereader.h
    GIS/shaperenderer.cpp
    GIS/shaperenderer.h
    GIS/dbfilereader.cpp
    GIS/dbfilereader.h
    DSP/meteordemodulator.cpp
    DSP/agc.cpp
    DSP/pll.cpp
	DSP/meteorcostas.cpp
	DSP/phasecontrolloop.cpp
	DSP/polyphasebank.cpp
	DSP/mm.cpp
	DSP/window.cpp
	DSP/windowedsinc.cpp
    DSP/filter.cpp
    DSP/iqsource.cpp
    DSP/wavreader.cpp
)

include_directories(
    ${OpenCV_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/imageproc
    ${CMAKE_SOURCE_DIR}/decoder
    ${CMAKE_SOURCE_DIR}/common
    ${CMAKE_SOURCE_DIR}/tools
    ${CMAKE_SOURCE_DIR}/DSP
    ${CMAKE_SOURCE_DIR}/external/sgp4/libsgp4
    ${CMAKE_SOURCE_DIR}/external/libcorrect/include
)

add_dependencies(meteordemod sgp4)
add_dependencies(meteordemod libcorrect)    

if(WIN32)
    target_link_libraries(meteordemod
        ${OpenCV_LIBS}
        sgp4.lib
        correct.lib
    )
else()
    target_link_libraries(meteordemod
        ${OpenCV_LIBS}
        sgp4.a
        correct.a
        stdc++fs
    )
endif()

if(WIN32)
    install(TARGETS meteordemod DESTINATION ${CMAKE_INSTALL_PREFIX})
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/resources/ DESTINATION ${CMAKE_INSTALL_PREFIX}/resources)
else()
    install(TARGETS meteordemod DESTINATION bin COMPONENT binaries)
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/resources/ DESTINATION $ENV{HOME}/.config/meteordemod COMPONENT config
    USE_SOURCE_PERMISSIONS
    )
endif()

if(UNIX AND NOT APPLE)
    find_file (DEBIAN_FOUND debian_version debconf.conf PATHS /etc)
    if (DEBIAN_FOUND)
        SET(CPACK_GENERATOR "DEB")
        SET(CPACK_PACKAGE_VERSION_MAJOR "2")
        SET(CPACK_PACKAGE_VERSION_MINOR "5")
        SET(CPACK_PACKAGE_VERSION_PATCH "5")
        SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "Digitelektro")
        SET(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/Digitelektro/MeteorDemod")
        SET(CPACK_DEBIAN_PACKAGE_DESCRIPTION "Russian Meteor M2 weather satellite data decoder")
		SET(CPACK_DEBIAN_PACKAGE_DEPENDS "python3-opencv")
        INCLUDE(CPack)
    endif()
endif()
