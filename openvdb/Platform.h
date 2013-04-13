///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2012-2013 DreamWorks Animation LLC
//
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
//
// Redistributions of source code must retain the above copyright
// and license notice and the following restrictions and disclaimer.
//
// *     Neither the name of DreamWorks Animation nor the names of
// its contributors may be used to endorse or promote products derived
// from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// IN NO EVENT SHALL THE COPYRIGHT HOLDERS' AND CONTRIBUTORS' AGGREGATE
// LIABILITY FOR ALL CLAIMS REGARDLESS OF THEIR BASIS EXCEED US$250.00.
//
///////////////////////////////////////////////////////////////////////////
///
/// @file Platform.h

#ifndef OPENVDB_PLATFORM_HAS_BEEN_INCLUDED
#define OPENVDB_PLATFORM_HAS_BEEN_INCLUDED

#include "PlatformConfig.h"

/// Use OPENVDB_DEPRECATED to mark functions as deprecated.
/// It should be placed right before the signature of the function,
/// e.g., "OPENVDB_DEPRECATED void functionName();".
#ifdef OPENVDB_DEPRECATED
#undef OPENVDB_DEPRECATED
#endif
#ifdef _MSC_VER
    #define OPENVDB_DEPRECATED  __declspec(deprecated)
#else
    #define OPENVDB_DEPRECATED  __attribute__ ((deprecated))
#endif

/// Macro for determining if GCC version is >= than X.Y
#if defined(__GNUC__)
    #define OPENVDB_CHECK_GCC(MAJOR, MINOR) \
        (__GNUC__ > MAJOR || (__GNUC__ == MAJOR && __GNUC_MINOR__ >= MINOR))
#else
    #define OPENVDB_CHECK_GCC(MAJOR, MINOR) 0
#endif

/// For compilers that need templated function specializations to have
/// storage qualifiers, we need to declare the specializations as static inline.
/// Otherwise, we'll get linker errors about multiply defined symbols.
#if defined(__GNUC__) && OPENVDB_CHECK_GCC(4, 4)
    #define OPENVDB_STATIC_SPECIALIZATION
#else
    #define OPENVDB_STATIC_SPECIALIZATION static
#endif

/// Visual C++ does not have constants like M_PI unless this is defined.
/// @note This is needed even though the core library is built with this but
/// hcustom 12.1 doesn't define it. So this is needed for HDK operators.
#ifndef _USE_MATH_DEFINES
    #define _USE_MATH_DEFINES
#endif

/// Visual C++ does not have round
#ifdef _MSC_VER
    #include <boost/math/special_functions/round.hpp>
    using boost::math::round;
#endif

/// Visual C++ uses _copysign() instead of copysign()
#ifdef _MSC_VER
    #include <float.h>
    static inline double copysign(double x, double y) { return _copysign(x, y); }
#endif

/// Visual C++ does not have stdint.h which defines types like uint64_t.
/// So for portability we instead include boost/cstdint.hpp.
#include <boost/cstdint.hpp>
using boost::int8_t;
using boost::int16_t;
using boost::int32_t;
using boost::int64_t;
using boost::uint8_t;
using boost::uint16_t;
using boost::uint32_t;
using boost::uint64_t;

/// Helper macros for defining library symbol visibility
#ifdef OPENVDB_EXPORT
#undef OPENVDB_EXPORT
#endif
#ifdef OPENVDB_IMPORT
#undef OPENVDB_IMPORT
#endif
#ifdef __GNUC__
    #define OPENVDB_EXPORT __attribute__((visibility("default")))
    #define OPENVDB_IMPORT __attribute__((visibility("default")))
#endif
#ifdef _WIN32
    #ifdef OPENVDB_DLL
        #define OPENVDB_EXPORT __declspec(dllexport)
        #define OPENVDB_IMPORT __declspec(dllimport)
    #else
        #define OPENVDB_EXPORT
        #define OPENVDB_IMPORT
    #endif
#endif

/// All classes and public free standing functions must be explicitly marked
/// as \<lib\>_API to be exported. The \<lib\>_PRIVATE macros are defined when
/// building that particular library.
#ifdef OPENVDB_API
#undef OPENVDB_API
#endif
#ifdef OPENVDB_PRIVATE
    #define OPENVDB_API OPENVDB_EXPORT
#else
    #define OPENVDB_API OPENVDB_IMPORT
#endif
#ifdef OPENVDB_HOUDINI_API
#undef OPENVDB_HOUDINI_API
#endif
#ifdef OPENVDB_HOUDINI_PRIVATE
    #define OPENVDB_HOUDINI_API OPENVDB_EXPORT
#else
    #define OPENVDB_HOUDINI_API OPENVDB_IMPORT
#endif

#endif // OPENVDB_PLATFORM_HAS_BEEN_INCLUDED

// Copyright (c) 2012-2013 DreamWorks Animation LLC
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
