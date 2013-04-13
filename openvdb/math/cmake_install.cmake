# Install script for directory: D:/7D/Rendering/Sources/VDB11/openvdb/math

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
   "D:/7D/Rendering/Sources/VDB11/openvdb/../Deploy/include/math/BBox.h;D:/7D/Rendering/Sources/VDB11/openvdb/../Deploy/include/math/Coord.h;D:/7D/Rendering/Sources/VDB11/openvdb/../Deploy/include/math/FiniteDifference.h;D:/7D/Rendering/Sources/VDB11/openvdb/../Deploy/include/math/Hermite.cc;D:/7D/Rendering/Sources/VDB11/openvdb/../Deploy/include/math/Hermite.h;D:/7D/Rendering/Sources/VDB11/openvdb/../Deploy/include/math/LegacyFrustum.h;D:/7D/Rendering/Sources/VDB11/openvdb/../Deploy/include/math/Maps.cc;D:/7D/Rendering/Sources/VDB11/openvdb/../Deploy/include/math/Maps.h;D:/7D/Rendering/Sources/VDB11/openvdb/../Deploy/include/math/Mat.h;D:/7D/Rendering/Sources/VDB11/openvdb/../Deploy/include/math/Mat3.h;D:/7D/Rendering/Sources/VDB11/openvdb/../Deploy/include/math/Mat4.h;D:/7D/Rendering/Sources/VDB11/openvdb/../Deploy/include/math/Math.h;D:/7D/Rendering/Sources/VDB11/openvdb/../Deploy/include/math/Operators.h;D:/7D/Rendering/Sources/VDB11/openvdb/../Deploy/include/math/Proximity.cc;D:/7D/Rendering/Sources/VDB11/openvdb/../Deploy/include/math/Proximity.h;D:/7D/Rendering/Sources/VDB11/openvdb/../Deploy/include/math/QuantizedUnitVec.cc;D:/7D/Rendering/Sources/VDB11/openvdb/../Deploy/include/math/QuantizedUnitVec.h;D:/7D/Rendering/Sources/VDB11/openvdb/../Deploy/include/math/Quat.h;D:/7D/Rendering/Sources/VDB11/openvdb/../Deploy/include/math/Stencils.h;D:/7D/Rendering/Sources/VDB11/openvdb/../Deploy/include/math/Transform.cc;D:/7D/Rendering/Sources/VDB11/openvdb/../Deploy/include/math/Transform.h;D:/7D/Rendering/Sources/VDB11/openvdb/../Deploy/include/math/Tuple.h;D:/7D/Rendering/Sources/VDB11/openvdb/../Deploy/include/math/Vec2.h;D:/7D/Rendering/Sources/VDB11/openvdb/../Deploy/include/math/Vec3.h;D:/7D/Rendering/Sources/VDB11/openvdb/../Deploy/include/math/Vec4.h")
  IF (CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  ENDIF (CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
  IF (CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  ENDIF (CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
FILE(INSTALL DESTINATION "D:/7D/Rendering/Sources/VDB11/openvdb/../Deploy/include/math" TYPE FILE FILES
    "D:/7D/Rendering/Sources/VDB11/openvdb/math/BBox.h"
    "D:/7D/Rendering/Sources/VDB11/openvdb/math/Coord.h"
    "D:/7D/Rendering/Sources/VDB11/openvdb/math/FiniteDifference.h"
    "D:/7D/Rendering/Sources/VDB11/openvdb/math/Hermite.cc"
    "D:/7D/Rendering/Sources/VDB11/openvdb/math/Hermite.h"
    "D:/7D/Rendering/Sources/VDB11/openvdb/math/LegacyFrustum.h"
    "D:/7D/Rendering/Sources/VDB11/openvdb/math/Maps.cc"
    "D:/7D/Rendering/Sources/VDB11/openvdb/math/Maps.h"
    "D:/7D/Rendering/Sources/VDB11/openvdb/math/Mat.h"
    "D:/7D/Rendering/Sources/VDB11/openvdb/math/Mat3.h"
    "D:/7D/Rendering/Sources/VDB11/openvdb/math/Mat4.h"
    "D:/7D/Rendering/Sources/VDB11/openvdb/math/Math.h"
    "D:/7D/Rendering/Sources/VDB11/openvdb/math/Operators.h"
    "D:/7D/Rendering/Sources/VDB11/openvdb/math/Proximity.cc"
    "D:/7D/Rendering/Sources/VDB11/openvdb/math/Proximity.h"
    "D:/7D/Rendering/Sources/VDB11/openvdb/math/QuantizedUnitVec.cc"
    "D:/7D/Rendering/Sources/VDB11/openvdb/math/QuantizedUnitVec.h"
    "D:/7D/Rendering/Sources/VDB11/openvdb/math/Quat.h"
    "D:/7D/Rendering/Sources/VDB11/openvdb/math/Stencils.h"
    "D:/7D/Rendering/Sources/VDB11/openvdb/math/Transform.cc"
    "D:/7D/Rendering/Sources/VDB11/openvdb/math/Transform.h"
    "D:/7D/Rendering/Sources/VDB11/openvdb/math/Tuple.h"
    "D:/7D/Rendering/Sources/VDB11/openvdb/math/Vec2.h"
    "D:/7D/Rendering/Sources/VDB11/openvdb/math/Vec3.h"
    "D:/7D/Rendering/Sources/VDB11/openvdb/math/Vec4.h"
    )
ENDIF(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")

