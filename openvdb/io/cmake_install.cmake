# Install script for directory: D:/7D/Rendering/Sources/VDB11/openvdb/io

# Set the install prefix
IF(NOT DEFINED CMAKE_INSTALL_PREFIX)
  SET(CMAKE_INSTALL_PREFIX "C:/Program Files (x86)/OpenVDB")
ENDIF(NOT DEFINED CMAKE_INSTALL_PREFIX)
STRING(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
IF(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  IF(BUILD_TYPE)
    STRING(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  ELSE(BUILD_TYPE)
    SET(CMAKE_INSTALL_CONFIG_NAME "")
  ENDIF(BUILD_TYPE)
  MESSAGE(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
ENDIF(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)

# Set the component getting installed.
IF(NOT CMAKE_INSTALL_COMPONENT)
  IF(COMPONENT)
    MESSAGE(STATUS "Install component: \"${COMPONENT}\"")
    SET(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  ELSE(COMPONENT)
    SET(CMAKE_INSTALL_COMPONENT)
  ENDIF(COMPONENT)
ENDIF(NOT CMAKE_INSTALL_COMPONENT)

IF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "D:/7D/Rendering/Sources/VDB11/openvdb/../Deploy/include/io/Archive.cc;D:/7D/Rendering/Sources/VDB11/openvdb/../Deploy/include/io/Archive.h;D:/7D/Rendering/Sources/VDB11/openvdb/../Deploy/include/io/Compression.cc;D:/7D/Rendering/Sources/VDB11/openvdb/../Deploy/include/io/Compression.h;D:/7D/Rendering/Sources/VDB11/openvdb/../Deploy/include/io/File.cc;D:/7D/Rendering/Sources/VDB11/openvdb/../Deploy/include/io/File.h;D:/7D/Rendering/Sources/VDB11/openvdb/../Deploy/include/io/GridDescriptor.cc;D:/7D/Rendering/Sources/VDB11/openvdb/../Deploy/include/io/GridDescriptor.h;D:/7D/Rendering/Sources/VDB11/openvdb/../Deploy/include/io/Stream.cc;D:/7D/Rendering/Sources/VDB11/openvdb/../Deploy/include/io/Stream.h")
  IF (CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  ENDIF (CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
  IF (CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  ENDIF (CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
FILE(INSTALL DESTINATION "D:/7D/Rendering/Sources/VDB11/openvdb/../Deploy/include/io" TYPE FILE FILES
    "D:/7D/Rendering/Sources/VDB11/openvdb/io/Archive.cc"
    "D:/7D/Rendering/Sources/VDB11/openvdb/io/Archive.h"
    "D:/7D/Rendering/Sources/VDB11/openvdb/io/Compression.cc"
    "D:/7D/Rendering/Sources/VDB11/openvdb/io/Compression.h"
    "D:/7D/Rendering/Sources/VDB11/openvdb/io/File.cc"
    "D:/7D/Rendering/Sources/VDB11/openvdb/io/File.h"
    "D:/7D/Rendering/Sources/VDB11/openvdb/io/GridDescriptor.cc"
    "D:/7D/Rendering/Sources/VDB11/openvdb/io/GridDescriptor.h"
    "D:/7D/Rendering/Sources/VDB11/openvdb/io/Stream.cc"
    "D:/7D/Rendering/Sources/VDB11/openvdb/io/Stream.h"
    )
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

