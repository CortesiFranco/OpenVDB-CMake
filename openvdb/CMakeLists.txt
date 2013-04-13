###################################################################################################################
#						   							   User Input												  #

# TBB Include Dir:
set(TBB_INCLUDE_DIR D:/7D/Rendering/Sources/Blender/trunk/lib/windows/tbb/include/)
set(TBB_LIB_DIR D:/7D/Rendering/Sources/Blender/trunk/lib/windows/tbb/lib/)

# Boost Include Dir:
set(BOOST_INCLUDE_DIR D:/7D/Rendering/Sources/Blender/trunk/lib/windows/boost/include/)

# Open EXR Include Dir:
set(HALF_INCLUDE_DIR D:/7D/Rendering/Sources/Blender/trunk/lib/windows/openexr/include/)
set(HALF_LIB_DIR D:/7D/Rendering/Sources/Blender/trunk/lib/windows/openexr/lib/)

# Zlib Include Dir:
set(ZLIB_INCLUDE_DIRS D:/7D/Rendering/Sources/Blender/trunk/lib/windows/zlib/include)


###################################################################################################################
#						   							Opitional Flags										          #

option(WITH_OPENVDB_PRINT "hola" OFF)
option(WITH_OPENVDB_VIEW "hola" OFF)
option(WITH_UNITTEST "hola" OFF)

###################################################################################################################
#						   							Platform Specific								 	     	 #

IF(WIN32)
	if(MINGW)
	#do
	else(MSVC)
	#do
	endif()
ENDIF()

IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
   # Mac OS X specific code
   SET(OperatingSystem "Mac OS X")
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin") 


IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    # Linux specific code
    SET(OperatingSystem "Linux")
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")

###################################################################################################################
#						   																		                  #

project(OpenVDB)
cmake_minimum_required(VERSION 2.8)

set(INC_SYS
	${TBB_INCLUDE_DIR}
	${BOOST_INCLUDE_DIR}
	${HALF_INCLUDE_DIR}
	${ZLIB_INCLUDE_DIRS}
)

# Include external headers
include_directories(${INC_SYS})

# This line allows "openvdb/xxxxx.h" include sintax working in openvdb sources.
include_directories(../)


set(SRC
#Main	
    Exceptions.h
    Grid.cc
    Grid.h
    Metadata.h
    openvdb.h
    Platform.cc
    Platform.h
    Types.h
    version.h

)

add_subdirectory(io)
add_subdirectory(math)
add_subdirectory(metadata)
add_subdirectory(tools)
add_subdirectory(util)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/../Deploy/bin CACHE INTERNAL "" FORCE)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/../Deploy/lib CACHE INTERNAL "" FORCE)



###################################################################################################################
#						   							Link Libraries      										  #

FIND_LIBRARY(TBB_DEBUG tbb_debug ${TBB_LIB_DIR})
#TARGET_LINK_LIBRARIES(${openvdb} ${TBB_DEBUG})


###################################################################################################################
#						   						 OpenVDB Building Output									      #
#DLL Output
add_executable(openvdb main.cc)

#Lib Output
#add_library(libopenvdb main.cc)

#Include Output
INSTALL(FILES ${SRC}
        DESTINATION ${CMAKE_BINARY_DIR}/../Deploy/include
        )
