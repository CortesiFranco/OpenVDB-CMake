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
  
#metadata
    Metadata.cc
    Metadata.h
    MetaMap.cc
    MetaMap.h
    StringMetadata.h
 
)

INSTALL(FILES ${SRC}
        DESTINATION ${CMAKE_BINARY_DIR}/../Deploy/include/metadata
        )