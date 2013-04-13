set(INC
	.
)

set(INC_SYS
	${TBB_INCLUDE_DIR}
	${BOOST_INCLUDE_DIR}
	${OPENEXR_INCLUDE_DIR}
	${ZLIB_INCLUDE_DIRS}
)

set(SRC
  
#io
    Archive.cc
    Archive.h
    Compression.cc
    Compression.h
    File.cc
    File.h
    GridDescriptor.cc
    GridDescriptor.h
    Stream.cc
    Stream.h
 
)

INSTALL(FILES ${SRC}
        DESTINATION ${CMAKE_BINARY_DIR}/../Deploy/include/io
        )
