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
  
#tree
    InternalNode.h
    Iterator.h
    LeafManager.h
    LeafNode.h
    LeafNodeBool.h
    NodeUnion.h
    RootNode.h
    Tree.h
    TreeIterator.h
    Util.h
    ValueAccessor.h
 
)

INSTALL(FILES ${SRC}
        DESTINATION ${CMAKE_BINARY_DIR}/../Deploy/include/tree
        )