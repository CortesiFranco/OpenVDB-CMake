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
  
#util
    Formats.cc
    Formats.h
    logging.h
    MapsUtil.h
    Name.h
    NodeMasks.h
    NullInterrupter.h
    Util.cc
    Util.h
 
)

INSTALL(FILES ${SRC}
        DESTINATION ${CMAKE_BINARY_DIR}/../Deploy/include/util
        )