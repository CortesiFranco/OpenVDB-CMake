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
  
#math
    BBox.h
    Coord.h
    FiniteDifference.h
    Hermite.cc
    Hermite.h
    LegacyFrustum.h
    Maps.cc
    Maps.h
    Mat.h
    Mat3.h
    Mat4.h
    Math.h
    Operators.h
    Proximity.cc
    Proximity.h
    QuantizedUnitVec.cc
    QuantizedUnitVec.h
    Quat.h
    Stencils.h
    Transform.cc
    Transform.h
    Tuple.h
    Vec2.h
    Vec3.h
    Vec4.h
 
)

INSTALL(FILES ${SRC}
        DESTINATION ${CMAKE_BINARY_DIR}/../Deploy/include/math
        )