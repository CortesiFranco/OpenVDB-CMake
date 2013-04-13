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
  
#tools
    Composite.h
    Filter.h
    GridOperators.h
    GridTransformer.h
    Interpolation.h
    LevelSetAdvect.h
    LevelSetFilter.h
    LevelSetRebuild.h
    LevelSetSphere.h
    LevelSetTracker.h
    LevelSetUtil.h
    MeshToVolume.h
    Morphology.h
    ParticlesToLevelSet.h
    PointAdvect.h
    PointScatter.h
    ValueTransformer.h
    VolumeToMesh.h
 
)

INSTALL(FILES ${SRC}
        DESTINATION ${CMAKE_BINARY_DIR}/../Deploy/include/tools
        )