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

#include <cstdio> // for remove()
#include <sstream>
#include <cppunit/extensions/HelperMacros.h>
#include <openvdb/Exceptions.h>
#include <openvdb/Types.h>
#include <openvdb/math/Transform.h>
#include <openvdb/tree/LeafNode.h>
#include <openvdb/tree/LeafManager.h>
#include <openvdb/io/Compression.h> // for io::RealToHalf
#include <openvdb/math/Math.h> // for Abs()
#include <openvdb/openvdb.h>
#include "util.h" // for unittest_util::makeSphere()

#define ASSERT_DOUBLES_EXACTLY_EQUAL(expected, actual) \
    CPPUNIT_ASSERT_DOUBLES_EQUAL((expected), (actual), /*tolerance=*/0.0);


typedef float                                            ValueType;
typedef openvdb::tree::LeafNode<ValueType,3>             LeafNodeType;
typedef openvdb::tree::InternalNode<LeafNodeType,4>      InternalNodeType1;
typedef openvdb::tree::InternalNode<InternalNodeType1,5> InternalNodeType2;
typedef openvdb::tree::RootNode<InternalNodeType2>       RootNodeType;


class TestTree: public CppUnit::TestFixture
{
public:
    virtual void setUp() { openvdb::initialize(); }
    virtual void tearDown() { openvdb::uninitialize(); }

    CPPUNIT_TEST_SUITE(TestTree);
    CPPUNIT_TEST(testBackground);
    CPPUNIT_TEST(testHalf);
    CPPUNIT_TEST(testValues);
    CPPUNIT_TEST(testSetValue);
    CPPUNIT_TEST(testSetValueOnly);
    CPPUNIT_TEST(testEvalMinMax);
    CPPUNIT_TEST(testResize);
    CPPUNIT_TEST(testHasSameTopology);
    CPPUNIT_TEST(testTopologyCopy);
    CPPUNIT_TEST(testIterators);
    CPPUNIT_TEST(testIO);
    CPPUNIT_TEST(testNegativeIndexing);
    CPPUNIT_TEST(testDeepCopy);
    CPPUNIT_TEST(testMerge);
    CPPUNIT_TEST(testVoxelizeActiveTiles);
    CPPUNIT_TEST(testTopologyUnion);
    CPPUNIT_TEST(testSignedFloodFill);
    CPPUNIT_TEST(testPruneInactive);
    CPPUNIT_TEST(testPruneLevelSet);
    CPPUNIT_TEST(testTouchLeaf);
    CPPUNIT_TEST(testProbeLeaf);
    CPPUNIT_TEST(testLeafManager);
    CPPUNIT_TEST(testProcessBBox);
    CPPUNIT_TEST_SUITE_END();

    void testBackground();
    void testHalf();
    void testValues();
    void testSetValue();
    void testSetValueOnly();
    void testEvalMinMax();
    void testResize();
    void testHasSameTopology();
    void testTopologyCopy();
    void testIterators();
    void testIO();
    void testNegativeIndexing();
    void testDeepCopy();
    void testMerge();
    void testVoxelizeActiveTiles();
    void testTopologyUnion();
    void testSignedFloodFill();
    void testPruneLevelSet();
    void testPruneInactive();
    void testTouchLeaf();
    void testProbeLeaf();
    void testLeafManager();
    void testProcessBBox();

private:
    template<class TreeType> void testWriteHalf();
};


CPPUNIT_TEST_SUITE_REGISTRATION(TestTree);


void
TestTree::testBackground()
{
    const ValueType background = 5.0f;
    RootNodeType root_node(background);
    CPPUNIT_ASSERT(root_node.getLevel()==3);
    ASSERT_DOUBLES_EXACTLY_EQUAL(background, root_node.getValue(openvdb::Coord(5,10,20)));
    const ValueType newBackground = 10.0f;
    root_node.setBackground(newBackground);
    ASSERT_DOUBLES_EXACTLY_EQUAL(newBackground, root_node.getValue(openvdb::Coord(5,10,20)));
}


void
TestTree::testHalf()
{
    testWriteHalf<openvdb::FloatTree>();
    testWriteHalf<openvdb::DoubleTree>();
    testWriteHalf<openvdb::Vec2STree>();
    testWriteHalf<openvdb::Vec2DTree>();
    testWriteHalf<openvdb::Vec3STree>();
    testWriteHalf<openvdb::Vec3DTree>();

    // Verify that non-floating-point grids are saved correctly.
    testWriteHalf<openvdb::BoolTree>();
    testWriteHalf<openvdb::Int32Tree>();
    testWriteHalf<openvdb::Int64Tree>();
}


template<class TreeType>
void
TestTree::testWriteHalf()
{
    typedef openvdb::Grid<TreeType> GridType;
    typedef typename TreeType::ValueType ValueT;
    ValueT background = openvdb::zeroVal<ValueT>();
    background += 5;
    GridType grid(background);

    unittest_util::makeSphere<GridType>(openvdb::Coord(64, 64, 64),
                                        openvdb::Vec3f(35, 30, 40),
                                        /*radius=*/10, grid,
                                        /*dx=*/1.0f, unittest_util::SPHERE_DENSE);
    CPPUNIT_ASSERT(!grid.tree().empty());

    // Write grid blocks in both float and half formats.
    std::ostringstream outFull(std::ios_base::binary);
    grid.setSaveFloatAsHalf(false);
    grid.writeBuffers(outFull);
    outFull.flush();
    const size_t fullBytes = outFull.str().size();
    CPPUNIT_ASSERT_MESSAGE("wrote empty full float buffers", fullBytes > 0);

    std::ostringstream outHalf(std::ios_base::binary);
    grid.setSaveFloatAsHalf(true);
    grid.writeBuffers(outHalf);
    outHalf.flush();
    const size_t halfBytes = outHalf.str().size();
    CPPUNIT_ASSERT_MESSAGE("wrote empty half float buffers", halfBytes > 0);

    if (openvdb::io::RealToHalf<ValueT>::isReal) {
        // Verify that the half float file is "significantly smaller" than the full float file.
        std::ostringstream ostr;
        ostr << "half float buffers not significantly smaller than full float ("
            << halfBytes << " vs. " << fullBytes << " bytes)";
        CPPUNIT_ASSERT_MESSAGE(ostr.str(), halfBytes < size_t(0.75 * fullBytes));
    } else {
        // For non-real data types, "half float" and "full float" file sizes should be the same.
        CPPUNIT_ASSERT_MESSAGE("full float and half float file sizes differ for data of type "
            + std::string(openvdb::typeNameAsString<ValueT>()), halfBytes == fullBytes);
    }

    // Read back the half float data (converting back to full float in the process),
    // then write it out again in half float format.  Verify that the resulting file
    // is identical to the original half float file.
    {
        openvdb::Grid<TreeType> gridCopy(grid);
        gridCopy.setSaveFloatAsHalf(true);
        std::istringstream is(outHalf.str(), std::ios_base::binary);

        // Since the input stream doesn't include a VDB header with file format version info,
        // tag the input stream explicitly with the current version number.
        openvdb::io::setCurrentVersion(is);

        gridCopy.readBuffers(is);

        std::ostringstream outDiff(std::ios_base::binary);
        gridCopy.writeBuffers(outDiff);
        outDiff.flush();

        CPPUNIT_ASSERT_MESSAGE("half-from-full and half-from-half buffers differ",
            outHalf.str() == outDiff.str());
    }
}


void
TestTree::testValues()
{
    ValueType background=5.0f;

    {
        const openvdb::Coord c0(5,10,20), c1(50000,20000,30000);
        RootNodeType root_node(background);
        const float v0=0.234f, v1=4.5678f;
        CPPUNIT_ASSERT(root_node.empty());
        ASSERT_DOUBLES_EXACTLY_EQUAL(root_node.getValue(c0), background);
        ASSERT_DOUBLES_EXACTLY_EQUAL(root_node.getValue(c1), background);
        root_node.setValueOn(c0, v0);
        root_node.setValueOn(c1, v1);
        ASSERT_DOUBLES_EXACTLY_EQUAL(v0,root_node.getValue(c0));
        ASSERT_DOUBLES_EXACTLY_EQUAL(v1,root_node.getValue(c1));
        int count=0;
        for (int i =0; i<256; ++i) {
            for (int j=0; j<256; ++j) {
                for (int k=0; k<256; ++k) {
                    if (root_node.getValue(openvdb::Coord(i,j,k))<1.0f) ++count;
                }
            }
        }
        CPPUNIT_ASSERT(count == 1);
    }

    {
        const openvdb::Coord min(-30,-25,-60), max(60,80,100);
        const openvdb::Coord c0(-5,-10,-20), c1(50,20,90), c2(59,67,89);
        const float v0=0.234f, v1=4.5678f, v2=-5.673f;
        RootNodeType root_node(background);
        CPPUNIT_ASSERT(root_node.empty());
        ASSERT_DOUBLES_EXACTLY_EQUAL(background,root_node.getValue(c0));
        ASSERT_DOUBLES_EXACTLY_EQUAL(background,root_node.getValue(c1));
        ASSERT_DOUBLES_EXACTLY_EQUAL(background,root_node.getValue(c2));
        root_node.setValueOn(c0, v0);
        root_node.setValueOn(c1, v1);
        root_node.setValueOn(c2, v2);
        ASSERT_DOUBLES_EXACTLY_EQUAL(v0,root_node.getValue(c0));
        ASSERT_DOUBLES_EXACTLY_EQUAL(v1,root_node.getValue(c1));
        ASSERT_DOUBLES_EXACTLY_EQUAL(v2,root_node.getValue(c2));
        int count=0;
        for (int i =min[0]; i<max[0]; ++i) {
            for (int j=min[1]; j<max[1]; ++j) {
                for (int k=min[2]; k<max[2]; ++k) {
                    if (root_node.getValue(openvdb::Coord(i,j,k))<1.0f) ++count;
                }
            }
        }
        CPPUNIT_ASSERT(count == 2);
    }
}


void
TestTree::testSetValue()
{
    const float background = 5.0f;
    openvdb::FloatTree tree(background);
    const openvdb::Coord c0( 5, 10, 20), c1(-5,-10,-20);

    ASSERT_DOUBLES_EXACTLY_EQUAL(background, tree.getValue(c0));
    ASSERT_DOUBLES_EXACTLY_EQUAL(background, tree.getValue(c1));
    CPPUNIT_ASSERT_EQUAL(-1, tree.getValueDepth(c0));
    CPPUNIT_ASSERT_EQUAL(-1, tree.getValueDepth(c1));
    CPPUNIT_ASSERT(tree.isValueOff(c0));
    CPPUNIT_ASSERT(tree.isValueOff(c1));

    tree.setValue(c0, 10.0);

    ASSERT_DOUBLES_EXACTLY_EQUAL(10.0, tree.getValue(c0));
    ASSERT_DOUBLES_EXACTLY_EQUAL(background, tree.getValue(c1));
    CPPUNIT_ASSERT_EQUAL( 3, tree.getValueDepth(c0));
    CPPUNIT_ASSERT_EQUAL(-1, tree.getValueDepth(c1));
    CPPUNIT_ASSERT_EQUAL( 3, tree.getValueDepth(openvdb::Coord(7, 10, 20)));
    CPPUNIT_ASSERT_EQUAL( 2, tree.getValueDepth(openvdb::Coord(8, 10, 20)));
    CPPUNIT_ASSERT(tree.isValueOn(c0));
    CPPUNIT_ASSERT(tree.isValueOff(c1));

    tree.setValue(c1, 20.0);

    ASSERT_DOUBLES_EXACTLY_EQUAL(10.0, tree.getValue(c0));
    ASSERT_DOUBLES_EXACTLY_EQUAL(20.0, tree.getValue(c1));
    CPPUNIT_ASSERT_EQUAL( 3, tree.getValueDepth(c0));
    CPPUNIT_ASSERT_EQUAL( 3, tree.getValueDepth(c1));
    CPPUNIT_ASSERT(tree.isValueOn(c0));
    CPPUNIT_ASSERT(tree.isValueOn(c1));

    tree.setValueOnMin(c0, 15.0);
    tree.setValueOnMin(c1, 15.0);

    ASSERT_DOUBLES_EXACTLY_EQUAL(10.0, tree.getValue(c0));
    ASSERT_DOUBLES_EXACTLY_EQUAL(15.0, tree.getValue(c1));

    tree.setValueOnMax(c0, 12.0);
    tree.setValueOnMax(c1, 12.0);

    ASSERT_DOUBLES_EXACTLY_EQUAL(12.0, tree.getValue(c0));
    ASSERT_DOUBLES_EXACTLY_EQUAL(15.0, tree.getValue(c1));
    CPPUNIT_ASSERT_EQUAL(2, int(tree.activeVoxelCount()));

    float minVal = -999.0, maxVal = -999.0;
    tree.evalMinMax(minVal, maxVal);
    ASSERT_DOUBLES_EXACTLY_EQUAL(12.0, minVal);
    ASSERT_DOUBLES_EXACTLY_EQUAL(15.0, maxVal);

    tree.setValueOff(c0, background);

    ASSERT_DOUBLES_EXACTLY_EQUAL(background, tree.getValue(c0));
    ASSERT_DOUBLES_EXACTLY_EQUAL(15.0, tree.getValue(c1));
    CPPUNIT_ASSERT_EQUAL(1, int(tree.activeVoxelCount()));

    tree.setValueOnSum(c0, background);
    tree.setValueOnSum(c1, background);

    ASSERT_DOUBLES_EXACTLY_EQUAL(2*background, tree.getValue(c0));
    ASSERT_DOUBLES_EXACTLY_EQUAL(15.0+background, tree.getValue(c1));
    CPPUNIT_ASSERT_EQUAL(2, int(tree.activeVoxelCount()));

    // Test the extremes of the coordinate range
    ASSERT_DOUBLES_EXACTLY_EQUAL(background, tree.getValue(openvdb::Coord::min()));
    ASSERT_DOUBLES_EXACTLY_EQUAL(background, tree.getValue(openvdb::Coord::max()));
    //std::cerr << "min=" << openvdb::Coord::min() << " max= " << openvdb::Coord::max() << std::endl;
    tree.setValue(openvdb::Coord::min(), 1.0f);
    tree.setValue(openvdb::Coord::max(), 2.0f);
    ASSERT_DOUBLES_EXACTLY_EQUAL(1.0f, tree.getValue(openvdb::Coord::min()));
    ASSERT_DOUBLES_EXACTLY_EQUAL(2.0f, tree.getValue(openvdb::Coord::max()));
}

void
TestTree::testSetValueOnly()
{
    const float background = 5.0f;
    openvdb::FloatTree tree(background);
    const openvdb::Coord c0( 5, 10, 20), c1(-5,-10,-20);

    ASSERT_DOUBLES_EXACTLY_EQUAL(background, tree.getValue(c0));
    ASSERT_DOUBLES_EXACTLY_EQUAL(background, tree.getValue(c1));
    CPPUNIT_ASSERT_EQUAL(-1, tree.getValueDepth(c0));
    CPPUNIT_ASSERT_EQUAL(-1, tree.getValueDepth(c1));
    CPPUNIT_ASSERT(tree.isValueOff(c0));
    CPPUNIT_ASSERT(tree.isValueOff(c1));

    tree.setValueOnly(c0, 10.0);

    ASSERT_DOUBLES_EXACTLY_EQUAL(10.0, tree.getValue(c0));
    ASSERT_DOUBLES_EXACTLY_EQUAL(background, tree.getValue(c1));
    CPPUNIT_ASSERT_EQUAL( 3, tree.getValueDepth(c0));
    CPPUNIT_ASSERT_EQUAL(-1, tree.getValueDepth(c1));
    CPPUNIT_ASSERT_EQUAL( 3, tree.getValueDepth(openvdb::Coord(7, 10, 20)));
    CPPUNIT_ASSERT_EQUAL( 2, tree.getValueDepth(openvdb::Coord(8, 10, 20)));
    CPPUNIT_ASSERT(tree.isValueOff(c0));
    CPPUNIT_ASSERT(tree.isValueOff(c1));

    tree.setValueOnly(c1, 20.0);

    ASSERT_DOUBLES_EXACTLY_EQUAL(10.0, tree.getValue(c0));
    ASSERT_DOUBLES_EXACTLY_EQUAL(20.0, tree.getValue(c1));
    CPPUNIT_ASSERT_EQUAL( 3, tree.getValueDepth(c0));
    CPPUNIT_ASSERT_EQUAL( 3, tree.getValueDepth(c1));
    CPPUNIT_ASSERT(tree.isValueOff(c0));
    CPPUNIT_ASSERT(tree.isValueOff(c1));

    tree.setValue(c0, 30.0);

    ASSERT_DOUBLES_EXACTLY_EQUAL(30.0, tree.getValue(c0));
    ASSERT_DOUBLES_EXACTLY_EQUAL(20.0, tree.getValue(c1));
    CPPUNIT_ASSERT_EQUAL( 3, tree.getValueDepth(c0));
    CPPUNIT_ASSERT_EQUAL( 3, tree.getValueDepth(c1));
    CPPUNIT_ASSERT(tree.isValueOn(c0));
    CPPUNIT_ASSERT(tree.isValueOff(c1));

    tree.setValueOnly(c0, 40.0);

    ASSERT_DOUBLES_EXACTLY_EQUAL(40.0, tree.getValue(c0));
    ASSERT_DOUBLES_EXACTLY_EQUAL(20.0, tree.getValue(c1));
    CPPUNIT_ASSERT_EQUAL( 3, tree.getValueDepth(c0));
    CPPUNIT_ASSERT_EQUAL( 3, tree.getValueDepth(c1));
    CPPUNIT_ASSERT(tree.isValueOn(c0));
    CPPUNIT_ASSERT(tree.isValueOff(c1));

    CPPUNIT_ASSERT_EQUAL(1, int(tree.activeVoxelCount()));
}


namespace {

/// Helper function to test openvdb::tree::Tree::evalMinMax() for various tree types
template<typename TreeT>
void evalMinMaxTest()
{
    typedef typename TreeT::ValueType ValueT;

    struct Local {
        static bool isEqual(const ValueT& a, const ValueT& b) {
            using namespace openvdb; // for operator>()
            return !(math::Abs(a - b) > zeroVal<ValueT>());
        }
    };

    const ValueT
        zero = openvdb::zeroVal<ValueT>(),
        minusTwo = zero + (-2),
        plusTwo = zero + 2,
        five = zero + 5;

    TreeT tree(/*background=*/five);

    // No set voxels (defaults to min = max = zero)
    ValueT minVal = five, maxVal = five;
    tree.evalMinMax(minVal, maxVal);
    CPPUNIT_ASSERT(Local::isEqual(minVal, zero));
    CPPUNIT_ASSERT(Local::isEqual(maxVal, zero));

    // Only one set voxel
    tree.setValue(openvdb::Coord(0, 0, 0), minusTwo);
    minVal = maxVal = five;
    tree.evalMinMax(minVal, maxVal);
    CPPUNIT_ASSERT(Local::isEqual(minVal, minusTwo));
    CPPUNIT_ASSERT(Local::isEqual(maxVal, minusTwo));

    // Multiple set voxels, single value
    tree.setValue(openvdb::Coord(10, 10, 10), minusTwo);
    minVal = maxVal = five;
    tree.evalMinMax(minVal, maxVal);
    CPPUNIT_ASSERT(Local::isEqual(minVal, minusTwo));
    CPPUNIT_ASSERT(Local::isEqual(maxVal, minusTwo));

    // Multiple set voxels, multiple values
    tree.setValue(openvdb::Coord(10, 10, 10), plusTwo);
    tree.setValue(openvdb::Coord(-10, -10, -10), zero);
    minVal = maxVal = five;
    tree.evalMinMax(minVal, maxVal);
    CPPUNIT_ASSERT(Local::isEqual(minVal, minusTwo));
    CPPUNIT_ASSERT(Local::isEqual(maxVal, plusTwo));
}

/// Specialization for boolean trees
template<>
void evalMinMaxTest<openvdb::BoolTree>()
{
    openvdb::BoolTree tree(/*background=*/false);

    // No set voxels (defaults to min = max = zero)
    bool minVal = true, maxVal = false;
    tree.evalMinMax(minVal, maxVal);
    CPPUNIT_ASSERT_EQUAL(false, minVal);
    CPPUNIT_ASSERT_EQUAL(false, maxVal);

    // Only one set voxel
    tree.setValue(openvdb::Coord(0, 0, 0), true);
    minVal = maxVal = false;
    tree.evalMinMax(minVal, maxVal);
    CPPUNIT_ASSERT_EQUAL(true, minVal);
    CPPUNIT_ASSERT_EQUAL(true, maxVal);

    // Multiple set voxels, single value
    tree.setValue(openvdb::Coord(-10, -10, -10), true);
    minVal = maxVal = false;
    tree.evalMinMax(minVal, maxVal);
    CPPUNIT_ASSERT_EQUAL(true, minVal);
    CPPUNIT_ASSERT_EQUAL(true, maxVal);

    // Multiple set voxels, multiple values
    tree.setValue(openvdb::Coord(10, 10, 10), false);
    minVal = true; maxVal = false;
    tree.evalMinMax(minVal, maxVal);
    CPPUNIT_ASSERT_EQUAL(false, minVal);
    CPPUNIT_ASSERT_EQUAL(true, maxVal);
}

/// Specialization for string trees
template<>
void evalMinMaxTest<openvdb::StringTree>()
{
    const std::string
        echidna("echidna"), loris("loris"), pangolin("pangolin");

    openvdb::StringTree tree(/*background=*/loris);

    // No set voxels (defaults to min = max = zero)
    std::string minVal, maxVal;
    tree.evalMinMax(minVal, maxVal);
    CPPUNIT_ASSERT_EQUAL(std::string(), minVal);
    CPPUNIT_ASSERT_EQUAL(std::string(), maxVal);

    // Only one set voxel
    tree.setValue(openvdb::Coord(0, 0, 0), pangolin);
    minVal.clear(); maxVal.clear();
    tree.evalMinMax(minVal, maxVal);
    CPPUNIT_ASSERT_EQUAL(pangolin, minVal);
    CPPUNIT_ASSERT_EQUAL(pangolin, maxVal);

    // Multiple set voxels, single value
    tree.setValue(openvdb::Coord(-10, -10, -10), pangolin);
    minVal.clear(); maxVal.clear();
    tree.evalMinMax(minVal, maxVal);
    CPPUNIT_ASSERT_EQUAL(pangolin, minVal);
    CPPUNIT_ASSERT_EQUAL(pangolin, maxVal);

    // Multiple set voxels, multiple values
    tree.setValue(openvdb::Coord(10, 10, 10), echidna);
    minVal.clear(); maxVal.clear();
    tree.evalMinMax(minVal, maxVal);
    CPPUNIT_ASSERT_EQUAL(echidna, minVal);
    CPPUNIT_ASSERT_EQUAL(pangolin, maxVal);
}

} // unnamed namespace

void
TestTree::testEvalMinMax()
{
    evalMinMaxTest<openvdb::BoolTree>();
    evalMinMaxTest<openvdb::FloatTree>();
    evalMinMaxTest<openvdb::Int32Tree>();
    evalMinMaxTest<openvdb::Vec3STree>();
    evalMinMaxTest<openvdb::Vec2ITree>();
    evalMinMaxTest<openvdb::StringTree>();
}


void
TestTree::testResize()
{
    ValueType background=5.0f;
    //use this when resize is implemented
    RootNodeType root_node(background);
    CPPUNIT_ASSERT(root_node.getLevel()==3);
    ASSERT_DOUBLES_EXACTLY_EQUAL(background, root_node.getValue(openvdb::Coord(5,10,20)));
    //fprintf(stdout,"Root grid  dim=(%i,%i,%i)\n",
    //    root_node.getGridDim(0), root_node.getGridDim(1), root_node.getGridDim(2));
    root_node.setValueOn(openvdb::Coord(5,10,20),0.234f);
    ASSERT_DOUBLES_EXACTLY_EQUAL(root_node.getValue(openvdb::Coord(5,10,20)) , 0.234f);
    root_node.setValueOn(openvdb::Coord(500,200,300),4.5678f);
    ASSERT_DOUBLES_EXACTLY_EQUAL(root_node.getValue(openvdb::Coord(500,200,300)) , 4.5678f);
    {
        ValueType sum=0.0f;
        for (RootNodeType::ChildOnIter root_iter = root_node.beginChildOn();
            root_iter.test(); ++root_iter)
        {
            for (InternalNodeType2::ChildOnIter internal_iter2 = root_iter->beginChildOn();
                internal_iter2.test(); ++internal_iter2)
            {
                for (InternalNodeType1::ChildOnIter internal_iter1 =
                    internal_iter2->beginChildOn(); internal_iter1.test(); ++internal_iter1)
                {
                    for (LeafNodeType::ValueOnIter block_iter =
                        internal_iter1->beginValueOn(); block_iter.test(); ++block_iter)
                    {
                        sum += *block_iter;
                    }
                }
            }
        }
        ASSERT_DOUBLES_EXACTLY_EQUAL(sum, (0.234f + 4.5678f));
    }

    CPPUNIT_ASSERT(root_node.getLevel()==3);
    ASSERT_DOUBLES_EXACTLY_EQUAL(background, root_node.getValue(openvdb::Coord(5,11,20)));
    {
        ValueType sum=0.0f;
        for (RootNodeType::ChildOnIter root_iter = root_node.beginChildOn();
            root_iter.test(); ++root_iter)
        {
            for (InternalNodeType2::ChildOnIter internal_iter2 = root_iter->beginChildOn();
                internal_iter2.test(); ++internal_iter2)
            {
                for (InternalNodeType1::ChildOnIter internal_iter1 =
                    internal_iter2->beginChildOn(); internal_iter1.test(); ++internal_iter1)
                {
                    for (LeafNodeType::ValueOnIter block_iter =
                        internal_iter1->beginValueOn(); block_iter.test(); ++block_iter)
                    {
                        sum += *block_iter;
                    }
                }
            }
        }
        ASSERT_DOUBLES_EXACTLY_EQUAL(sum, (0.234f + 4.5678f));
    }

}


void
TestTree::testHasSameTopology()
{
    // Test using trees of the same type.
    {
        const float background1=5.0f;
        openvdb::FloatTree tree1(background1);

        const float background2=6.0f;
        openvdb::FloatTree tree2(background2);

        CPPUNIT_ASSERT(tree1.hasSameTopology(tree2));
        CPPUNIT_ASSERT(tree2.hasSameTopology(tree1));

        tree1.setValue(openvdb::Coord(-10,40,845),3.456f);
        CPPUNIT_ASSERT(!tree1.hasSameTopology(tree2));
        CPPUNIT_ASSERT(!tree2.hasSameTopology(tree1));

        tree2.setValue(openvdb::Coord(-10,40,845),-3.456f);
        CPPUNIT_ASSERT(tree1.hasSameTopology(tree2));
        CPPUNIT_ASSERT(tree2.hasSameTopology(tree1));

        tree1.setValue(openvdb::Coord(1,-500,-8), 1.0f);
        CPPUNIT_ASSERT(!tree1.hasSameTopology(tree2));
        CPPUNIT_ASSERT(!tree2.hasSameTopology(tree1));

        tree2.setValue(openvdb::Coord(1,-500,-8),1.0f);
        CPPUNIT_ASSERT(tree1.hasSameTopology(tree2));
        CPPUNIT_ASSERT(tree2.hasSameTopology(tree1));
    }
    // Test using trees of different types.
    {
        const float background1=5.0f;
        openvdb::FloatTree tree1(background1);

        const openvdb::Vec3f background2(1.0f,3.4f,6.0f);
        openvdb::Vec3fTree tree2(background2);

        CPPUNIT_ASSERT(tree1.hasSameTopology(tree2));
        CPPUNIT_ASSERT(tree2.hasSameTopology(tree1));

        tree1.setValue(openvdb::Coord(-10,40,845),3.456f);
        CPPUNIT_ASSERT(!tree1.hasSameTopology(tree2));
        CPPUNIT_ASSERT(!tree2.hasSameTopology(tree1));

        tree2.setValue(openvdb::Coord(-10,40,845),openvdb::Vec3f(1.0f,2.0f,-3.0f));
        CPPUNIT_ASSERT(tree1.hasSameTopology(tree2));
        CPPUNIT_ASSERT(tree2.hasSameTopology(tree1));

        tree1.setValue(openvdb::Coord(1,-500,-8), 1.0f);
        CPPUNIT_ASSERT(!tree1.hasSameTopology(tree2));
        CPPUNIT_ASSERT(!tree2.hasSameTopology(tree1));

        tree2.setValue(openvdb::Coord(1,-500,-8),openvdb::Vec3f(1.0f,2.0f,-3.0f));
        CPPUNIT_ASSERT(tree1.hasSameTopology(tree2));
        CPPUNIT_ASSERT(tree2.hasSameTopology(tree1));
    }
}


void
TestTree::testTopologyCopy()
{
    // Test using trees of the same type.
    {
        const float background1=5.0f;
        openvdb::FloatTree tree1(background1);
        tree1.setValue(openvdb::Coord(-10,40,845),3.456f);
        tree1.setValue(openvdb::Coord(1,-50,-8), 1.0f);

        const float background2=6.0f, setValue2=3.0f;
        openvdb::FloatTree tree2(tree1,background2,setValue2,openvdb::TopologyCopy());

        CPPUNIT_ASSERT(tree1.hasSameTopology(tree2));
        CPPUNIT_ASSERT(tree2.hasSameTopology(tree1));

        ASSERT_DOUBLES_EXACTLY_EQUAL(background2, tree2.getValue(openvdb::Coord(1,2,3)));
        ASSERT_DOUBLES_EXACTLY_EQUAL(setValue2, tree2.getValue(openvdb::Coord(-10,40,845)));
        ASSERT_DOUBLES_EXACTLY_EQUAL(setValue2, tree2.getValue(openvdb::Coord(1,-50,-8)));

        tree1.setValue(openvdb::Coord(1,-500,-8), 1.0f);
        CPPUNIT_ASSERT(!tree1.hasSameTopology(tree2));
        CPPUNIT_ASSERT(!tree2.hasSameTopology(tree1));

        tree2.setValue(openvdb::Coord(1,-500,-8),1.0f);
        CPPUNIT_ASSERT(tree1.hasSameTopology(tree2));
        CPPUNIT_ASSERT(tree2.hasSameTopology(tree1));
    }
    // Test using trees of different types.
    {
        const openvdb::Vec3f background1(1.0f,3.4f,6.0f);
        openvdb::Vec3fTree tree1(background1);
        tree1.setValue(openvdb::Coord(-10,40,845),openvdb::Vec3f(3.456f,-2.3f,5.6f));
        tree1.setValue(openvdb::Coord(1,-50,-8), openvdb::Vec3f(1.0f,3.0f,4.5f));

        const float background2=6.0f, setValue2=3.0f;
        openvdb::FloatTree tree2(tree1,background2,setValue2,openvdb::TopologyCopy());

        CPPUNIT_ASSERT(tree1.hasSameTopology(tree2));
        CPPUNIT_ASSERT(tree2.hasSameTopology(tree1));

        ASSERT_DOUBLES_EXACTLY_EQUAL(background2, tree2.getValue(openvdb::Coord(1,2,3)));
        ASSERT_DOUBLES_EXACTLY_EQUAL(setValue2, tree2.getValue(openvdb::Coord(-10,40,845)));
        ASSERT_DOUBLES_EXACTLY_EQUAL(setValue2, tree2.getValue(openvdb::Coord(1,-50,-8)));

        tree1.setValue(openvdb::Coord(1,-500,-8), openvdb::Vec3f(1.0f,0.0f,-3.0f));
        CPPUNIT_ASSERT(!tree1.hasSameTopology(tree2));
        CPPUNIT_ASSERT(!tree2.hasSameTopology(tree1));

        tree2.setValue(openvdb::Coord(1,-500,-8), 1.0f);
        CPPUNIT_ASSERT(tree1.hasSameTopology(tree2));
        CPPUNIT_ASSERT(tree2.hasSameTopology(tree1));
    }
}


void
TestTree::testIterators()
{
    ValueType background=5.0f;
    RootNodeType root_node(background);
    root_node.setValueOn(openvdb::Coord(5,10,20),0.234f);
    root_node.setValueOn(openvdb::Coord(50000,20000,30000),4.5678f);
    {
        ValueType sum=0.0f;
        for (RootNodeType::ChildOnIter root_iter = root_node.beginChildOn();
            root_iter.test(); ++root_iter)
        {
            for (InternalNodeType2::ChildOnIter internal_iter2 = root_iter->beginChildOn();
                internal_iter2.test(); ++internal_iter2)
            {
                for (InternalNodeType1::ChildOnIter internal_iter1 =
                    internal_iter2->beginChildOn(); internal_iter1.test(); ++internal_iter1)
                {
                    for (LeafNodeType::ValueOnIter block_iter =
                        internal_iter1->beginValueOn(); block_iter.test(); ++block_iter)
                    {
                        sum += *block_iter;
                    }
                }
            }
        }
        ASSERT_DOUBLES_EXACTLY_EQUAL((0.234f + 4.5678f), sum);
    }
    {
        // As above, but using dense iterators.
        ValueType sum = 0.0f, val = 0.0f;
        for (RootNodeType::ChildAllIter rootIter = root_node.beginChildAll();
            rootIter.test(); ++rootIter)
        {
            if (!rootIter.isChildNode()) continue;

            for (InternalNodeType2::ChildAllIter internalIter2 =
                rootIter.probeChild(val)->beginChildAll(); internalIter2; ++internalIter2)
            {
                if (!internalIter2.isChildNode()) continue;

                for (InternalNodeType1::ChildAllIter internalIter1 =
                    internalIter2.probeChild(val)->beginChildAll(); internalIter1; ++internalIter1)
                {
                    if (!internalIter1.isChildNode()) continue;

                    for (LeafNodeType::ValueOnIter leafIter =
                        internalIter1.probeChild(val)->beginValueOn(); leafIter; ++leafIter)
                    {
                        sum += *leafIter;
                    }
                }
            }
        }
        ASSERT_DOUBLES_EXACTLY_EQUAL((0.234f + 4.5678f), sum);
    }
    {
        ValueType v_sum=0.0f;
        openvdb::Coord xyz0, xyz1, xyz2, xyz3, xyzSum(0, 0, 0);
        for (RootNodeType::ChildOnIter root_iter = root_node.beginChildOn();
            root_iter.test(); ++root_iter)
        {
            root_iter.getCoord(xyz3);
            for (InternalNodeType2::ChildOnIter internal_iter2 = root_iter->beginChildOn();
                internal_iter2.test(); ++internal_iter2)
            {
                internal_iter2.getCoord(xyz2);
                xyz2 = xyz2 - internal_iter2.parent().getOrigin();
                for (InternalNodeType1::ChildOnIter internal_iter1 =
                    internal_iter2->beginChildOn(); internal_iter1.test(); ++internal_iter1)
                {
                    internal_iter1.getCoord(xyz1);
                    xyz1 = xyz1 - internal_iter1.parent().getOrigin();
                    for (LeafNodeType::ValueOnIter block_iter =
                        internal_iter1->beginValueOn(); block_iter.test(); ++block_iter)
                    {
                        block_iter.getCoord(xyz0);
                        xyz0 = xyz0 - block_iter.parent().getOrigin();
                        v_sum += *block_iter;
                        xyzSum = xyzSum + xyz0 + xyz1 + xyz2 + xyz3;
                    }
                }
            }
        }
        ASSERT_DOUBLES_EXACTLY_EQUAL((0.234f + 4.5678f), v_sum);
        CPPUNIT_ASSERT_EQUAL(openvdb::Coord(5 + 50000, 10 + 20000, 20 + 30000), xyzSum);
    }
}


void
TestTree::testIO()
{
    const char* filename = "/tmp/test.dbg";
    boost::shared_ptr<const char> scopedFile(filename, ::remove);
    {
        ValueType background=5.0f;
        RootNodeType root_node(background);
        root_node.setValueOn(openvdb::Coord(5,10,20),0.234f);
        root_node.setValueOn(openvdb::Coord(50000,20000,30000),4.5678f);

        std::ofstream os(filename, std::ios_base::binary);
        root_node.writeTopology(os);
        root_node.writeBuffers(os);
        CPPUNIT_ASSERT(!os.fail());
    }
    {
        ValueType background=2.0f;
        RootNodeType root_node(background);
        ASSERT_DOUBLES_EXACTLY_EQUAL(background, root_node.getValue(openvdb::Coord(5,10,20)));
        {
            std::ifstream is(filename, std::ios_base::binary);
            // Since the test file doesn't include a VDB header with file format version info,
            // tag the input stream explicitly with the current version number.
            openvdb::io::setCurrentVersion(is);
            root_node.readTopology(is);
            root_node.readBuffers(is);
            CPPUNIT_ASSERT(!is.fail());
        }

        ASSERT_DOUBLES_EXACTLY_EQUAL(0.234f, root_node.getValue(openvdb::Coord(5,10,20)));
        ASSERT_DOUBLES_EXACTLY_EQUAL(5.0f, root_node.getValue(openvdb::Coord(5,11,20)));
        ValueType sum=0.0f;
        for (RootNodeType::ChildOnIter root_iter = root_node.beginChildOn();
            root_iter.test(); ++root_iter)
        {
            for (InternalNodeType2::ChildOnIter internal_iter2 = root_iter->beginChildOn();
                internal_iter2.test(); ++internal_iter2)
            {
                for (InternalNodeType1::ChildOnIter internal_iter1 =
                    internal_iter2->beginChildOn(); internal_iter1.test(); ++internal_iter1)
                {
                    for (LeafNodeType::ValueOnIter block_iter =
                        internal_iter1->beginValueOn(); block_iter.test(); ++block_iter)
                    {
                        sum += *block_iter;
                    }
                }
            }
        }
        ASSERT_DOUBLES_EXACTLY_EQUAL(sum, (0.234f + 4.5678f));
    }
}





void
TestTree::testNegativeIndexing()
{
    ValueType background=5.0f;
    openvdb::FloatTree tree(background);
    CPPUNIT_ASSERT(tree.empty());
    ASSERT_DOUBLES_EXACTLY_EQUAL(tree.getValue(openvdb::Coord(5,-10,20)), background);
    ASSERT_DOUBLES_EXACTLY_EQUAL(tree.getValue(openvdb::Coord(-5000,2000,3000)), background);
    tree.setValue(openvdb::Coord( 5, 10, 20),0.0f);
    tree.setValue(openvdb::Coord(-5, 10, 20),0.1f);
    tree.setValue(openvdb::Coord( 5,-10, 20),0.2f);
    tree.setValue(openvdb::Coord( 5, 10,-20),0.3f);
    tree.setValue(openvdb::Coord(-5,-10, 20),0.4f);
    tree.setValue(openvdb::Coord(-5, 10,-20),0.5f);
    tree.setValue(openvdb::Coord( 5,-10,-20),0.6f);
    tree.setValue(openvdb::Coord(-5,-10,-20),0.7f);
    tree.setValue(openvdb::Coord(-5000, 2000,-3000),4.5678f);
    tree.setValue(openvdb::Coord( 5000,-2000,-3000),4.5678f);
    tree.setValue(openvdb::Coord(-5000,-2000, 3000),4.5678f);
    ASSERT_DOUBLES_EXACTLY_EQUAL(0.0f, tree.getValue(openvdb::Coord( 5, 10, 20)));
    ASSERT_DOUBLES_EXACTLY_EQUAL(0.1f, tree.getValue(openvdb::Coord(-5, 10, 20)));
    ASSERT_DOUBLES_EXACTLY_EQUAL(0.2f, tree.getValue(openvdb::Coord( 5,-10, 20)));
    ASSERT_DOUBLES_EXACTLY_EQUAL(0.3f, tree.getValue(openvdb::Coord( 5, 10,-20)));
    ASSERT_DOUBLES_EXACTLY_EQUAL(0.4f, tree.getValue(openvdb::Coord(-5,-10, 20)));
    ASSERT_DOUBLES_EXACTLY_EQUAL(0.5f, tree.getValue(openvdb::Coord(-5, 10,-20)));
    ASSERT_DOUBLES_EXACTLY_EQUAL(0.6f, tree.getValue(openvdb::Coord( 5,-10,-20)));
    ASSERT_DOUBLES_EXACTLY_EQUAL(0.7f, tree.getValue(openvdb::Coord(-5,-10,-20)));
    ASSERT_DOUBLES_EXACTLY_EQUAL(4.5678f, tree.getValue(openvdb::Coord(-5000, 2000,-3000)));
    ASSERT_DOUBLES_EXACTLY_EQUAL(4.5678f, tree.getValue(openvdb::Coord( 5000,-2000,-3000)));
    ASSERT_DOUBLES_EXACTLY_EQUAL(4.5678f, tree.getValue(openvdb::Coord(-5000,-2000, 3000)));
    int count=0;
    for (int i =-25; i<25; ++i) {
        for (int j=-25; j<25; ++j) {
            for (int k=-25; k<25; ++k) {
                if (tree.getValue(openvdb::Coord(i,j,k))<1.0f) {
                    //fprintf(stderr,"(%i,%i,%i)=%f\n",i,j,k,tree.getValue(openvdb::Coord(i,j,k)));
                    ++count;
                }
            }
        }
    }
    CPPUNIT_ASSERT(count == 8);
    int count2 = 0;
    openvdb::Coord xyz;
    for (openvdb::FloatTree::ValueOnCIter iter = tree.cbeginValueOn(); iter; ++iter) {
        ++count2;
        xyz = iter.getCoord();
        //std::cerr << xyz << " = " << *iter << "\n";
    }
    CPPUNIT_ASSERT(count2 == 11);
    CPPUNIT_ASSERT(tree.activeVoxelCount() == 11);
    {
        count2 = 0;
        for (openvdb::FloatTree::ValueOnCIter iter = tree.cbeginValueOn(); iter; ++iter) {
            ++count2;
            xyz = iter.getCoord();
            //std::cerr << xyz << " = " << *iter << "\n";
        }
        CPPUNIT_ASSERT(count2 == 11);
        CPPUNIT_ASSERT(tree.activeVoxelCount() == 11);
    }
}


void
TestTree::testDeepCopy()
{
    // set up a tree
    const float fillValue1=5.0f;
    openvdb::FloatTree tree1(fillValue1);
    tree1.setValue(openvdb::Coord(-10,40,845), 3.456f);
    tree1.setValue(openvdb::Coord(1,-50,-8), 1.0f);

    // make a deep copy of the tree
    openvdb::TreeBase::Ptr newTree = tree1.copy();

    // cast down to the concrete type to query values
    openvdb::FloatTree *pTree2 = dynamic_cast<openvdb::FloatTree *>(newTree.get());

    // compare topology
    CPPUNIT_ASSERT(tree1.hasSameTopology(*pTree2));
    CPPUNIT_ASSERT(pTree2->hasSameTopology(tree1));

    // trees should be equal
    ASSERT_DOUBLES_EXACTLY_EQUAL(fillValue1, pTree2->getValue(openvdb::Coord(1,2,3)));
    ASSERT_DOUBLES_EXACTLY_EQUAL(3.456f, pTree2->getValue(openvdb::Coord(-10,40,845)));
    ASSERT_DOUBLES_EXACTLY_EQUAL(1.0f, pTree2->getValue(openvdb::Coord(1,-50,-8)));

    // change 1 value in tree2
    openvdb::Coord changeCoord(1, -500, -8);
    pTree2->setValue(changeCoord, 1.0f);

    // topology should no longer match
    CPPUNIT_ASSERT(!tree1.hasSameTopology(*pTree2));
    CPPUNIT_ASSERT(!pTree2->hasSameTopology(tree1));

    // query changed value and make sure it's different between trees
    ASSERT_DOUBLES_EXACTLY_EQUAL(fillValue1, tree1.getValue(changeCoord));
    ASSERT_DOUBLES_EXACTLY_EQUAL(1.0f, pTree2->getValue(changeCoord));
}

void
TestTree::testMerge()
{
    ValueType background=5.0f;
    openvdb::FloatTree tree0(background), tree1(background), tree2(background);
     CPPUNIT_ASSERT(tree2.empty());
    tree0.setValue(openvdb::Coord( 5, 10, 20),0.0f);
    tree0.setValue(openvdb::Coord(-5, 10, 20),0.1f);
    tree0.setValue(openvdb::Coord( 5,-10, 20),0.2f);
    tree0.setValue(openvdb::Coord( 5, 10,-20),0.3f);
    tree1.setValue(openvdb::Coord( 5, 10, 20),0.0f);
    tree1.setValue(openvdb::Coord(-5, 10, 20),0.1f);
    tree1.setValue(openvdb::Coord( 5,-10, 20),0.2f);
    tree1.setValue(openvdb::Coord( 5, 10,-20),0.3f);

    tree0.setValue(openvdb::Coord(-5,-10, 20),0.4f);
    tree0.setValue(openvdb::Coord(-5, 10,-20),0.5f);
    tree0.setValue(openvdb::Coord( 5,-10,-20),0.6f);
    tree0.setValue(openvdb::Coord(-5,-10,-20),0.7f);
    tree0.setValue(openvdb::Coord(-5000, 2000,-3000),4.5678f);
    tree0.setValue(openvdb::Coord( 5000,-2000,-3000),4.5678f);
    tree0.setValue(openvdb::Coord(-5000,-2000, 3000),4.5678f);
    tree2.setValue(openvdb::Coord(-5,-10, 20),0.4f);
    tree2.setValue(openvdb::Coord(-5, 10,-20),0.5f);
    tree2.setValue(openvdb::Coord( 5,-10,-20),0.6f);
    tree2.setValue(openvdb::Coord(-5,-10,-20),0.7f);
    tree2.setValue(openvdb::Coord(-5000, 2000,-3000),4.5678f);
    tree2.setValue(openvdb::Coord( 5000,-2000,-3000),4.5678f);
    tree2.setValue(openvdb::Coord(-5000,-2000, 3000),4.5678f);

    CPPUNIT_ASSERT(tree0.leafCount()!=tree1.leafCount());
    CPPUNIT_ASSERT(tree0.leafCount()!=tree2.leafCount());

    CPPUNIT_ASSERT(!tree2.empty());
    tree1.merge(tree2);
    CPPUNIT_ASSERT(tree2.empty());
    CPPUNIT_ASSERT(tree0.leafCount()==tree1.leafCount());
    CPPUNIT_ASSERT(tree0.nonLeafCount()==tree1.nonLeafCount());
    CPPUNIT_ASSERT(tree0.activeLeafVoxelCount()==tree1.activeLeafVoxelCount());
    CPPUNIT_ASSERT(tree0.inactiveLeafVoxelCount()==tree1.inactiveLeafVoxelCount());
    CPPUNIT_ASSERT(tree0.activeVoxelCount()==tree1.activeVoxelCount());
    CPPUNIT_ASSERT(tree0.inactiveVoxelCount()==tree1.inactiveVoxelCount());

    for (openvdb::FloatTree::ValueOnCIter iter0 = tree0.cbeginValueOn(); iter0; ++iter0) {
        ASSERT_DOUBLES_EXACTLY_EQUAL(*iter0,tree1.getValue(iter0.getCoord()));
    }

    // Test active tile support.
    {
        using namespace openvdb;
        FloatTree treeA(/*background*/0.0), treeB(/*background*/0.0);

        treeA.fill(CoordBBox(Coord(16,16,16), Coord(31,31,31)), /*value*/1.0);
        treeB.fill(CoordBBox(Coord(0,0,0),    Coord(15,15,15)), /*value*/1.0);

        CPPUNIT_ASSERT(4096 == treeA.activeVoxelCount());
        CPPUNIT_ASSERT(4096 == treeB.activeVoxelCount());

        treeA.merge(treeB);

        CPPUNIT_ASSERT(8192 == treeA.activeVoxelCount());
        CPPUNIT_ASSERT(0 == treeB.activeVoxelCount());
    }

}

void
TestTree::testVoxelizeActiveTiles()
{
    using openvdb::CoordBBox;
    using openvdb::Coord;
    // Use a small custom tree so we don't run out of memory when
    // tiles are converted to dense leafs :)
    typedef openvdb::tree::Tree4<float,2, 2, 2>::Type MyTree;
    float background=5.0f;
    const Coord xyz[] = {Coord(-1,-2,-3),Coord( 1, 2, 3)};
    //check two leaf nodes and two tiles at each level 1, 2 and 3
    const int tile_size[4]={0, 1<<2, 1<<(2*2), 1<<(3*2)};
    for (int level=0; level<=3; ++level) {

        MyTree tree(background);
        CPPUNIT_ASSERT_EQUAL(-1,tree.getValueDepth(xyz[0]));
        CPPUNIT_ASSERT_EQUAL(-1,tree.getValueDepth(xyz[1]));

        if (level==0) {
            tree.setValue(xyz[0], 1.0f);
            tree.setValue(xyz[1], 1.0f);
        } else {
            const int n = tile_size[level];
            tree.fill(CoordBBox::createCube(Coord(-n,-n,-n), n), 1.0f, true);
            tree.fill(CoordBBox::createCube(Coord( 0, 0, 0), n), 1.0f, true);
        }

        CPPUNIT_ASSERT_EQUAL(3-level,tree.getValueDepth(xyz[0]));
        CPPUNIT_ASSERT_EQUAL(3-level,tree.getValueDepth(xyz[1]));

        tree.voxelizeActiveTiles();

        CPPUNIT_ASSERT_EQUAL(3      ,tree.getValueDepth(xyz[0]));
        CPPUNIT_ASSERT_EQUAL(3      ,tree.getValueDepth(xyz[1]));
    }
}

void
TestTree::testTopologyUnion()
{
    {//super simple test with only two active values
        const ValueType background=0.0f;
        openvdb::FloatTree tree0(background), tree1(background);
        tree0.setValue(openvdb::Coord( 500, 300, 200), 1.0f);
        tree1.setValue(openvdb::Coord(   8,  11,  11), 2.0f);
        openvdb::FloatTree tree2(tree1);
        tree1.topologyUnion(tree0);

        for (openvdb::FloatTree::ValueOnCIter iter = tree0.cbeginValueOn(); iter; ++iter) {
            CPPUNIT_ASSERT(tree1.isValueOn(iter.getCoord()));
        }
        for (openvdb::FloatTree::ValueOnCIter iter = tree2.cbeginValueOn(); iter; ++iter) {
            CPPUNIT_ASSERT(tree1.isValueOn(iter.getCoord()));
        }
        for (openvdb::FloatTree::ValueOnCIter iter = tree1.cbeginValueOn(); iter; ++iter) {
            ASSERT_DOUBLES_EXACTLY_EQUAL(*iter,tree2.getValue(iter.getCoord()));
        }
    }
    {// test using setValue
        ValueType background=5.0f;
        openvdb::FloatTree tree0(background), tree1(background), tree2(background);
        CPPUNIT_ASSERT(tree2.empty());
        // tree0 = tree1.topologyUnion(tree2)
        tree0.setValue(openvdb::Coord( 5, 10, 20),0.0f);
        tree0.setValue(openvdb::Coord(-5, 10, 20),0.1f);
        tree0.setValue(openvdb::Coord( 5,-10, 20),0.2f);
        tree0.setValue(openvdb::Coord( 5, 10,-20),0.3f);
        tree1.setValue(openvdb::Coord( 5, 10, 20),0.0f);
        tree1.setValue(openvdb::Coord(-5, 10, 20),0.1f);
        tree1.setValue(openvdb::Coord( 5,-10, 20),0.2f);
        tree1.setValue(openvdb::Coord( 5, 10,-20),0.3f);

        tree0.setValue(openvdb::Coord(-5,-10, 20),background);
        tree0.setValue(openvdb::Coord(-5, 10,-20),background);
        tree0.setValue(openvdb::Coord( 5,-10,-20),background);
        tree0.setValue(openvdb::Coord(-5,-10,-20),background);
        tree0.setValue(openvdb::Coord(-5000, 2000,-3000),background);
        tree0.setValue(openvdb::Coord( 5000,-2000,-3000),background);
        tree0.setValue(openvdb::Coord(-5000,-2000, 3000),background);
        tree2.setValue(openvdb::Coord(-5,-10, 20),0.4f);
        tree2.setValue(openvdb::Coord(-5, 10,-20),0.5f);
        tree2.setValue(openvdb::Coord( 5,-10,-20),0.6f);
        tree2.setValue(openvdb::Coord(-5,-10,-20),0.7f);
        tree2.setValue(openvdb::Coord(-5000, 2000,-3000),4.5678f);
        tree2.setValue(openvdb::Coord( 5000,-2000,-3000),4.5678f);
        tree2.setValue(openvdb::Coord(-5000,-2000, 3000),4.5678f);

        // tree3 has the same topology as tree2 but a different value type
        const openvdb::Vec3f background2(1.0f,3.4f,6.0f), vec_val(3.1f,5.3f,-9.5f);
        openvdb::Vec3fTree tree3(background2);
        for (openvdb::FloatTree::ValueOnCIter iter2 = tree2.cbeginValueOn(); iter2; ++iter2) {
            tree3.setValue(iter2.getCoord(), vec_val);
        }

        CPPUNIT_ASSERT(tree0.leafCount()!=tree1.leafCount());
        CPPUNIT_ASSERT(tree0.leafCount()!=tree2.leafCount());
        CPPUNIT_ASSERT(tree0.leafCount()!=tree3.leafCount());

        CPPUNIT_ASSERT(!tree2.empty());
        CPPUNIT_ASSERT(!tree3.empty());
        openvdb::FloatTree tree1_copy(tree1);
        //tree1.topologyUnion(tree2);//should make tree1 = tree0
        tree1.topologyUnion(tree3);//should make tree1 = tree0

        CPPUNIT_ASSERT(tree0.leafCount()==tree1.leafCount());
        CPPUNIT_ASSERT(tree0.nonLeafCount()==tree1.nonLeafCount());
        CPPUNIT_ASSERT(tree0.activeLeafVoxelCount()==tree1.activeLeafVoxelCount());
        CPPUNIT_ASSERT(tree0.inactiveLeafVoxelCount()==tree1.inactiveLeafVoxelCount());
        CPPUNIT_ASSERT(tree0.activeVoxelCount()==tree1.activeVoxelCount());
        CPPUNIT_ASSERT(tree0.inactiveVoxelCount()==tree1.inactiveVoxelCount());

        CPPUNIT_ASSERT(tree1.hasSameTopology(tree0));
        CPPUNIT_ASSERT(tree0.hasSameTopology(tree1));

        for (openvdb::FloatTree::ValueOnCIter iter2 = tree2.cbeginValueOn(); iter2; ++iter2) {
            CPPUNIT_ASSERT(tree1.isValueOn(iter2.getCoord()));
        }
        for (openvdb::FloatTree::ValueOnCIter iter1 = tree1.cbeginValueOn(); iter1; ++iter1) {
            CPPUNIT_ASSERT(tree0.isValueOn(iter1.getCoord()));
        }
        for (openvdb::FloatTree::ValueOnCIter iter0 = tree0.cbeginValueOn(); iter0; ++iter0) {
            CPPUNIT_ASSERT(tree1.isValueOn(iter0.getCoord()));
            ASSERT_DOUBLES_EXACTLY_EQUAL(*iter0,tree1.getValue(iter0.getCoord()));
        }
        for (openvdb::FloatTree::ValueOnCIter iter = tree1_copy.cbeginValueOn(); iter; ++iter) {
            CPPUNIT_ASSERT(tree1.isValueOn(iter.getCoord()));
            ASSERT_DOUBLES_EXACTLY_EQUAL(*iter,tree1.getValue(iter.getCoord()));
        }
        for (openvdb::FloatTree::ValueOnCIter iter = tree1.cbeginValueOn(); iter; ++iter) {
            const openvdb::Coord p = iter.getCoord();
            CPPUNIT_ASSERT(tree3.isValueOn(p) || tree1_copy.isValueOn(p));
        }
    }
    {// another test using setValueOn
         ValueType background=5.0f;
         openvdb::FloatTree tree0(background), tree1(background), tree2(background);
         CPPUNIT_ASSERT(tree2.empty());
         // tree0 = tree1.topologyUnion(tree2)
         tree0.setValue(openvdb::Coord( 5, 10, 20),0.0f);
         tree0.setValue(openvdb::Coord(-5, 10, 20),0.1f);
         tree0.setValue(openvdb::Coord( 5,-10, 20),0.2f);
         tree0.setValue(openvdb::Coord( 5, 10,-20),0.3f);
         tree1.setValue(openvdb::Coord( 5, 10, 20),0.0f);
         tree1.setValue(openvdb::Coord(-5, 10, 20),0.1f);
         tree1.setValue(openvdb::Coord( 5,-10, 20),0.2f);
         tree1.setValue(openvdb::Coord( 5, 10,-20),0.3f);

         tree0.setValue(openvdb::Coord(-5,-10, 20),background);
         tree0.setValue(openvdb::Coord(-5, 10,-20),background);
         tree0.setValue(openvdb::Coord( 5,-10,-20),background);
         tree0.setValue(openvdb::Coord(-5,-10,-20),background);
         tree0.setValue(openvdb::Coord(-5000, 2000,-3000),background);
         tree0.setValue(openvdb::Coord( 5000,-2000,-3000),background);
         tree0.setValue(openvdb::Coord(-5000,-2000, 3000),background);
         tree2.setValue(openvdb::Coord(-5,-10, 20),0.4f);
         tree2.setValue(openvdb::Coord(-5, 10,-20),0.5f);
         tree2.setValue(openvdb::Coord( 5,-10,-20),0.6f);
         tree2.setValue(openvdb::Coord(-5,-10,-20),0.7f);
         tree2.setValue(openvdb::Coord(-5000, 2000,-3000),4.5678f);
         tree2.setValue(openvdb::Coord( 5000,-2000,-3000),4.5678f);
         tree2.setValue(openvdb::Coord(-5000,-2000, 3000),4.5678f);

         // tree3 has the same topology as tree2 but a different value type
         const openvdb::Vec3f background2(1.0f,3.4f,6.0f), vec_val(3.1f,5.3f,-9.5f);
         openvdb::Vec3fTree tree3(background2);

         for (openvdb::FloatTree::ValueOnCIter iter2 = tree2.cbeginValueOn(); iter2; ++iter2) {
             tree3.setValue(iter2.getCoord(), vec_val);
         }

         openvdb::FloatTree tree4(tree1);//tree4 = tree1
         openvdb::FloatTree tree5(tree1);//tree5 = tree1

         tree1.topologyUnion(tree3);//should make tree1 = tree0

         CPPUNIT_ASSERT(tree1.hasSameTopology(tree0));

         for (openvdb::Vec3fTree::ValueOnCIter iter3 = tree3.cbeginValueOn(); iter3; ++iter3) {
             tree4.setValueOn(iter3.getCoord());
             const openvdb::Coord p = iter3.getCoord();
             ASSERT_DOUBLES_EXACTLY_EQUAL(tree1.getValue(p),tree5.getValue(p));
             ASSERT_DOUBLES_EXACTLY_EQUAL(tree4.getValue(p),tree5.getValue(p));
         }

         CPPUNIT_ASSERT(tree4.hasSameTopology(tree0));

         for (openvdb::FloatTree::ValueOnCIter iter4 = tree4.cbeginValueOn(); iter4; ++iter4) {
             const openvdb::Coord p = iter4.getCoord();
             ASSERT_DOUBLES_EXACTLY_EQUAL(tree0.getValue(p),tree5.getValue(p));
             ASSERT_DOUBLES_EXACTLY_EQUAL(tree1.getValue(p),tree5.getValue(p));
             ASSERT_DOUBLES_EXACTLY_EQUAL(tree4.getValue(p),tree5.getValue(p));
         }

         for (openvdb::FloatTree::ValueOnCIter iter = tree1.cbeginValueOn(); iter; ++iter) {
             const openvdb::Coord p = iter.getCoord();
             CPPUNIT_ASSERT(tree3.isValueOn(p) || tree4.isValueOn(p));
         }
    }
    {// test spheres
         const float background=5.0f, R0=10.0f, R1=5.6f;
         const openvdb::Vec3f C0(35.0f, 30.0f, 40.0f), C1(12.3f, 20.5f, 31.0f);
         const openvdb::Coord dim(32, 32, 32);
         openvdb::FloatGrid grid0(background);
         openvdb::FloatGrid grid1(background);
         unittest_util::makeSphere<openvdb::FloatGrid>(dim, C0, R0, grid0,
                                                       1.0f, unittest_util::SPHERE_DENSE);
         unittest_util::makeSphere<openvdb::FloatGrid>(dim, C1, R1, grid1,
                                                       1.0f, unittest_util::SPHERE_DENSE);
         openvdb::FloatTree& tree0 = grid0.tree();
         openvdb::FloatTree& tree1 = grid1.tree();
         openvdb::FloatTree tree0_copy(tree0);
         tree0.topologyUnion(tree1);
         for (openvdb::FloatTree::ValueOnCIter iter = tree1.cbeginValueOn(); iter; ++iter) {
             const openvdb::Coord p = iter.getCoord();
             CPPUNIT_ASSERT(tree0.isValueOn(p));
             ASSERT_DOUBLES_EXACTLY_EQUAL(tree0.getValue(p), tree0_copy.getValue(p));
         }
         for (openvdb::FloatTree::ValueOnCIter iter = tree0_copy.cbeginValueOn(); iter; ++iter) {
             const openvdb::Coord p = iter.getCoord();
             CPPUNIT_ASSERT(tree0.isValueOn(p));
             ASSERT_DOUBLES_EXACTLY_EQUAL(tree0.getValue(p), *iter);
         }
    }
}

void
TestTree::testSignedFloodFill()
{//Employs a custom tree configuration to ensure we flood-fill at all levels!
    typedef openvdb::tree::LeafNode<float,2>     LeafT;//4^3
    typedef openvdb::tree::InternalNode<LeafT,2> InternalT;//4^3
    typedef openvdb::tree::RootNode<InternalT>   RootT;// child nodes are 16^3
    typedef openvdb::tree::Tree<RootT>           TreeT;

    const float outside = 2.0f, inside = -outside, radius = 20.0f;
    openvdb::Grid<TreeT>::Ptr grid = openvdb::Grid<TreeT>::create(outside);
    TreeT& tree = grid->tree();
    const RootT& root = tree.getRootNode();
    const openvdb::Coord dim(3*16, 3*16, 3*16);
    const openvdb::Coord C(16+8,16+8,16+8);

    CPPUNIT_ASSERT(!tree.isValueOn(C));
    CPPUNIT_ASSERT(root.getTableSize()==0);

    //make narrow band of sphere without setting sign for the background values!
    openvdb::Grid<TreeT>::Accessor acc = grid->getAccessor();
    const openvdb::Vec3f center(C[0], C[1], C[2]);
    openvdb::Coord xyz;
    for (xyz[0]=0; xyz[0]<dim[0]; ++xyz[0]) {
        for (xyz[1]=0; xyz[1]<dim[1]; ++xyz[1]) {
            for (xyz[2]=0; xyz[2]<dim[2]; ++xyz[2]) {
                const openvdb::Vec3R p =  grid->transform().indexToWorld(xyz);
                const float dist = (p-center).length() - radius;
                if (fabs(dist) > outside) continue;
                acc.setValue(xyz, dist);
            }
        }
    }
    // Check narrow band with incorrect background
    const size_t size_before = root.getTableSize();
    CPPUNIT_ASSERT(size_before>0);
    CPPUNIT_ASSERT(!tree.isValueOn(C));
    ASSERT_DOUBLES_EXACTLY_EQUAL(outside,tree.getValue(C));
    for (xyz[0]=0; xyz[0]<dim[0]; ++xyz[0]) {
        for (xyz[1]=0; xyz[1]<dim[1]; ++xyz[1]) {
            for (xyz[2]=0; xyz[2]<dim[2]; ++xyz[2]) {
                const openvdb::Vec3R p =  grid->transform().indexToWorld(xyz);
                const float dist = (p-center).length() - radius;
                const float val  =  acc.getValue(xyz);
                if (dist < inside) {
                    ASSERT_DOUBLES_EXACTLY_EQUAL( val, outside);
                } else if (dist>outside) {
                    ASSERT_DOUBLES_EXACTLY_EQUAL( val, outside);
                } else {
                    ASSERT_DOUBLES_EXACTLY_EQUAL( val, dist   );
                }
            }
        }
    }

    CPPUNIT_ASSERT(tree.getValueDepth(C) == -1);//i.e. background value
    tree.signedFloodFill();
    CPPUNIT_ASSERT(tree.getValueDepth(C) ==  0);//added inside tile to root

    // Check narrow band with correct background
    for (xyz[0]=0; xyz[0]<dim[0]; ++xyz[0]) {
        for (xyz[1]=0; xyz[1]<dim[1]; ++xyz[1]) {
            for (xyz[2]=0; xyz[2]<dim[2]; ++xyz[2]) {
                const openvdb::Vec3R p =  grid->transform().indexToWorld(xyz);
                const float dist = (p-center).length() - radius;
                const float val  =  acc.getValue(xyz);
                if (dist < inside) {
                    ASSERT_DOUBLES_EXACTLY_EQUAL( val, inside);
                } else if (dist>outside) {
                    ASSERT_DOUBLES_EXACTLY_EQUAL( val, outside);
                } else {
                    ASSERT_DOUBLES_EXACTLY_EQUAL( val, dist   );
                }
            }
        }
    }

    CPPUNIT_ASSERT(root.getTableSize()>size_before);//added inside root tiles
    CPPUNIT_ASSERT(!tree.isValueOn(C));
    ASSERT_DOUBLES_EXACTLY_EQUAL(inside,tree.getValue(C));
}


void
TestTree::testPruneInactive()
{
    using openvdb::Coord;
    using openvdb::Index32;
    using openvdb::Index64;

    float background = 5.0;
    openvdb::FloatTree tree(background);

    // Verify that the newly-constructed tree is empty and that pruning it has no effect.
    CPPUNIT_ASSERT(tree.empty());
    tree.prune();
    CPPUNIT_ASSERT(tree.empty());
    tree.pruneInactive(background);
    CPPUNIT_ASSERT(tree.empty());

    // Set some active values.
    tree.setValue(Coord(-5, 10, 20), 0.1);
    tree.setValue(Coord(-5,-10, 20), 0.4);
    tree.setValue(Coord(-5, 10,-20), 0.5);
    tree.setValue(Coord(-5,-10,-20), 0.7);
    tree.setValue(Coord( 5, 10, 20), 0.0);
    tree.setValue(Coord( 5,-10, 20), 0.2);
    tree.setValue(Coord( 5,-10,-20), 0.6);
    tree.setValue(Coord( 5, 10,-20), 0.3);
    // Verify that the tree has the expected numbers of active voxels and leaf nodes.
    CPPUNIT_ASSERT_EQUAL(Index64(8), tree.activeVoxelCount());
    CPPUNIT_ASSERT_EQUAL(Index32(8), tree.leafCount());

    // Verify that prune() has no effect, since the values are all different.
    tree.prune();
    CPPUNIT_ASSERT_EQUAL(Index64(8), tree.activeVoxelCount());
    CPPUNIT_ASSERT_EQUAL(Index32(8), tree.leafCount());
    // Verify that pruneInactive() has no effect, since the values are active.
    tree.pruneInactive(background);
    CPPUNIT_ASSERT_EQUAL(Index64(8), tree.activeVoxelCount());
    CPPUNIT_ASSERT_EQUAL(Index32(8), tree.leafCount());

    // Make some of the active values inactive, without changing their values.
    tree.setValueOff(Coord(-5, 10, 20));
    tree.setValueOff(Coord(-5,-10, 20));
    tree.setValueOff(Coord(-5, 10,-20));
    tree.setValueOff(Coord(-5,-10,-20));
    CPPUNIT_ASSERT_EQUAL(Index64(4), tree.activeVoxelCount());
    CPPUNIT_ASSERT_EQUAL(Index32(8), tree.leafCount());
    // Verify that prune() has no effect, since the values are still different.
    tree.prune();
    CPPUNIT_ASSERT_EQUAL(Index64(4), tree.activeVoxelCount());
    CPPUNIT_ASSERT_EQUAL(Index32(8), tree.leafCount());
    // Verify that pruneInactive() prunes the nodes containing only inactive voxels.
    tree.pruneInactive(background);
    CPPUNIT_ASSERT_EQUAL(Index64(4), tree.activeVoxelCount());
    CPPUNIT_ASSERT_EQUAL(Index32(4), tree.leafCount());

    // Make all of the active values inactive, without changing their values.
    tree.setValueOff(Coord( 5, 10, 20));
    tree.setValueOff(Coord( 5,-10, 20));
    tree.setValueOff(Coord( 5,-10,-20));
    tree.setValueOff(Coord( 5, 10,-20));
    CPPUNIT_ASSERT_EQUAL(Index64(0), tree.activeVoxelCount());
    CPPUNIT_ASSERT_EQUAL(Index32(4), tree.leafCount());
    // Verify that prune() has no effect, since the values are still different.
    tree.prune();
    CPPUNIT_ASSERT_EQUAL(Index64(0), tree.activeVoxelCount());
    CPPUNIT_ASSERT_EQUAL(Index32(4), tree.leafCount());
    // Verify that pruneInactive() prunes all of the remaining leaf nodes.
    tree.pruneInactive(background);
    CPPUNIT_ASSERT(tree.empty());
}

void
TestTree::testPruneLevelSet()
{
    const float background=10.0f, R=5.6f;
    const openvdb::Vec3f C(12.3f, 15.5f, 10.0f);
    const openvdb::Coord dim(32, 32, 32);
    openvdb::FloatGrid grid(background);
    unittest_util::makeSphere<openvdb::FloatGrid>(dim, C, R, grid,
                                                  1.0f, unittest_util::SPHERE_SPARSE_NARROW_BAND);
    openvdb::FloatTree& tree = grid.tree();

    openvdb::Index64 count = 0;
    openvdb::Coord xyz;
    for (xyz[0]=0; xyz[0]<dim[0]; ++xyz[0]) {
        for (xyz[1]=0; xyz[1]<dim[1]; ++xyz[1]) {
            for (xyz[2]=0; xyz[2]<dim[2]; ++xyz[2]) {
                if (fabs(tree.getValue(xyz))<background) ++count;
            }
        }
    }

    const openvdb::Index32 leafCount = tree.leafCount();
    CPPUNIT_ASSERT_EQUAL(tree.activeVoxelCount(), count);
    CPPUNIT_ASSERT_EQUAL(tree.activeLeafVoxelCount(), count);

    openvdb::Index64 removed = 0;
    const float new_width = background - 9.0f;

    // This version is fast since it only visits voxel and avoids
    // random access to set the voxels off.
    typedef openvdb::FloatTree::LeafNodeType::ValueOnIter VoxelOnIter;
    for (openvdb::FloatTree::LeafIter lIter = tree.beginLeaf(); lIter; ++lIter) {
        for (VoxelOnIter vIter = lIter->beginValueOn(); vIter; ++vIter) {
            if (fabs(*vIter)<new_width) continue;
            lIter->setValueOff(vIter.pos(), *vIter > 0.0f ? background : -background);
            ++removed;
        }
    }
    // The following version is slower since it employs
    // FloatTree::ValueOnIter that visits both tiles and voxels and
    // also uses random acceess to set the voxels off.
    /*
    for (openvdb::FloatTree::ValueOnIter i = tree.beginValueOn(); i; ++i) {
        if (fabs(*i)<new_width) continue;
        tree.setValueOff(i.getCoord(), *i > 0.0f ? background : -background);
        ++removed2;
    }
    */

    CPPUNIT_ASSERT_EQUAL(leafCount, tree.leafCount());
    //std::cerr << "Leaf count=" << tree.leafCount() << std::endl;
    CPPUNIT_ASSERT_EQUAL(tree.activeVoxelCount(), count-removed);
    CPPUNIT_ASSERT_EQUAL(tree.activeLeafVoxelCount(), count-removed);
    
    tree.pruneLevelSet();

    CPPUNIT_ASSERT(tree.leafCount() < leafCount);
    //std::cerr << "Leaf count=" << tree.leafCount() << std::endl;
    CPPUNIT_ASSERT_EQUAL(tree.activeVoxelCount(), count-removed);
    CPPUNIT_ASSERT_EQUAL(tree.activeLeafVoxelCount(), count-removed);

    openvdb::FloatTree::ValueOnCIter i = tree.cbeginValueOn();
    for (; i; ++i) CPPUNIT_ASSERT( *i < new_width);

    for (xyz[0]=0; xyz[0]<dim[0]; ++xyz[0]) {
        for (xyz[1]=0; xyz[1]<dim[1]; ++xyz[1]) {
            for (xyz[2]=0; xyz[2]<dim[2]; ++xyz[2]) {
                const float val = tree.getValue(xyz);
                if (fabs(val)<new_width)
                    CPPUNIT_ASSERT(tree.isValueOn(xyz));
                else if (val < 0.0f) {
                    CPPUNIT_ASSERT(tree.isValueOff(xyz));
                    ASSERT_DOUBLES_EXACTLY_EQUAL( -background, val );
                } else {
                    CPPUNIT_ASSERT(tree.isValueOff(xyz));
                    ASSERT_DOUBLES_EXACTLY_EQUAL(  background, val );
                }
            }
        }
    }
}

void
TestTree::testTouchLeaf()
{
    const float background=10.0f;
    const openvdb::Coord xyz(-20,30,10);
    {// test tree
        openvdb::FloatTree::Ptr tree(new openvdb::FloatTree(background));
        CPPUNIT_ASSERT_EQUAL(-1, tree->getValueDepth(xyz));
        CPPUNIT_ASSERT_EQUAL( 0, int(tree->leafCount()));
        CPPUNIT_ASSERT(tree->touchLeaf(xyz)!=NULL);
        CPPUNIT_ASSERT_EQUAL( 3, tree->getValueDepth(xyz));
        CPPUNIT_ASSERT_EQUAL( 1, int(tree->leafCount()));
        CPPUNIT_ASSERT(!tree->isValueOn(xyz));
        ASSERT_DOUBLES_EXACTLY_EQUAL(background, tree->getValue(xyz));
    }
    {// test accessor
        openvdb::FloatTree::Ptr tree(new openvdb::FloatTree(background));
        openvdb::tree::ValueAccessor<openvdb::FloatTree> acc(*tree);
        CPPUNIT_ASSERT_EQUAL(-1, acc.getValueDepth(xyz));
        CPPUNIT_ASSERT_EQUAL( 0, int(tree->leafCount()));
        CPPUNIT_ASSERT(acc.touchLeaf(xyz)!=NULL);
        CPPUNIT_ASSERT_EQUAL( 3, tree->getValueDepth(xyz));
        CPPUNIT_ASSERT_EQUAL( 1, int(tree->leafCount()));
        CPPUNIT_ASSERT(!acc.isValueOn(xyz));
        ASSERT_DOUBLES_EXACTLY_EQUAL(background, acc.getValue(xyz));
    }
}

void
TestTree::testProbeLeaf()
{
    const float background=10.0f, value = 2.0f;
    const openvdb::Coord xyz(-20,30,10);
    {// test Tree::probeLeaf
        openvdb::FloatTree::Ptr tree(new openvdb::FloatTree(background));
        CPPUNIT_ASSERT_EQUAL(-1, tree->getValueDepth(xyz));
        CPPUNIT_ASSERT_EQUAL( 0, int(tree->leafCount()));
        CPPUNIT_ASSERT(tree->probeLeaf(xyz)==NULL);
        CPPUNIT_ASSERT_EQUAL(-1, tree->getValueDepth(xyz));
        CPPUNIT_ASSERT_EQUAL( 0, int(tree->leafCount()));
        tree->setValue(xyz, value);
        CPPUNIT_ASSERT_EQUAL( 3, tree->getValueDepth(xyz));
        CPPUNIT_ASSERT_EQUAL( 1, int(tree->leafCount()));
        CPPUNIT_ASSERT(tree->probeLeaf(xyz)!=NULL);
        CPPUNIT_ASSERT_EQUAL( 3, tree->getValueDepth(xyz));
        CPPUNIT_ASSERT_EQUAL( 1, int(tree->leafCount()));
        CPPUNIT_ASSERT(tree->isValueOn(xyz));
        ASSERT_DOUBLES_EXACTLY_EQUAL(value, tree->getValue(xyz));
    }
    {// test Tree::probeConstLeaf
        const openvdb::FloatTree tree1(background);
        CPPUNIT_ASSERT_EQUAL(-1, tree1.getValueDepth(xyz));
        CPPUNIT_ASSERT_EQUAL( 0, int(tree1.leafCount()));
        CPPUNIT_ASSERT(tree1.probeConstLeaf(xyz)==NULL);
        CPPUNIT_ASSERT_EQUAL(-1, tree1.getValueDepth(xyz));
        CPPUNIT_ASSERT_EQUAL( 0, int(tree1.leafCount()));
        openvdb::FloatTree tmp(tree1);
        tmp.setValue(xyz, value);
        const openvdb::FloatTree tree2(tmp);
        CPPUNIT_ASSERT_EQUAL( 3, tree2.getValueDepth(xyz));
        CPPUNIT_ASSERT_EQUAL( 1, int(tree2.leafCount()));
        CPPUNIT_ASSERT(tree2.probeConstLeaf(xyz)!=NULL);
        CPPUNIT_ASSERT_EQUAL( 3, tree2.getValueDepth(xyz));
        CPPUNIT_ASSERT_EQUAL( 1, int(tree2.leafCount()));
        CPPUNIT_ASSERT(tree2.isValueOn(xyz));
        ASSERT_DOUBLES_EXACTLY_EQUAL(value, tree2.getValue(xyz));
    }
    {// test ValueAccessor::probeLeaf
        openvdb::FloatTree::Ptr tree(new openvdb::FloatTree(background));
        openvdb::tree::ValueAccessor<openvdb::FloatTree> acc(*tree);
        CPPUNIT_ASSERT_EQUAL(-1, acc.getValueDepth(xyz));
        CPPUNIT_ASSERT_EQUAL( 0, int(tree->leafCount()));
        CPPUNIT_ASSERT(acc.probeLeaf(xyz)==NULL);
        CPPUNIT_ASSERT_EQUAL(-1, acc.getValueDepth(xyz));
        CPPUNIT_ASSERT_EQUAL( 0, int(tree->leafCount()));
        acc.setValue(xyz, value);
        CPPUNIT_ASSERT_EQUAL( 3, acc.getValueDepth(xyz));
        CPPUNIT_ASSERT_EQUAL( 1, int(tree->leafCount()));
        CPPUNIT_ASSERT(acc.probeLeaf(xyz)!=NULL);
        CPPUNIT_ASSERT_EQUAL( 3, acc.getValueDepth(xyz));
        CPPUNIT_ASSERT_EQUAL( 1, int(tree->leafCount()));
        CPPUNIT_ASSERT(acc.isValueOn(xyz));
        ASSERT_DOUBLES_EXACTLY_EQUAL(value, acc.getValue(xyz));
    }
    {// test ValueAccessor::probeConstLeaf
        const openvdb::FloatTree tree1(background);
        openvdb::tree::ValueAccessor<const openvdb::FloatTree> acc1(tree1);
        CPPUNIT_ASSERT_EQUAL(-1, acc1.getValueDepth(xyz));
        CPPUNIT_ASSERT_EQUAL( 0, int(tree1.leafCount()));
        CPPUNIT_ASSERT(acc1.probeConstLeaf(xyz)==NULL);
        CPPUNIT_ASSERT_EQUAL(-1, acc1.getValueDepth(xyz));
        CPPUNIT_ASSERT_EQUAL( 0, int(tree1.leafCount()));
        openvdb::FloatTree tmp(tree1);
        tmp.setValue(xyz, value);
        const openvdb::FloatTree tree2(tmp);
        openvdb::tree::ValueAccessor<const openvdb::FloatTree> acc2(tree2);
        CPPUNIT_ASSERT_EQUAL( 3, acc2.getValueDepth(xyz));
        CPPUNIT_ASSERT_EQUAL( 1, int(tree2.leafCount()));
        CPPUNIT_ASSERT(acc2.probeConstLeaf(xyz)!=NULL);
        CPPUNIT_ASSERT_EQUAL( 3, acc2.getValueDepth(xyz));
        CPPUNIT_ASSERT_EQUAL( 1, int(tree2.leafCount()));
        CPPUNIT_ASSERT(acc2.isValueOn(xyz));
        ASSERT_DOUBLES_EXACTLY_EQUAL(value, acc2.getValue(xyz));
    }
}

struct BBoxOp
{
    std::vector<openvdb::CoordBBox> bbox;
    std::vector<openvdb::Index> level;

    // This method is required by Tree::visitActiveBBox
    // Since it will return false if LEVEL==0 it will never descent to
    // the active voxels. In other words the smallest BBoxes
    // correspond to LeafNodes or active tiles at LEVEL=1
    template<openvdb::Index LEVEL>
    inline bool descent() { return LEVEL>0; }

    // This method is required by Tree::visitActiveBBox
    template<openvdb::Index LEVEL>
    inline void operator()(const openvdb::CoordBBox &_bbox) {
        bbox.push_back(_bbox);
        level.push_back(LEVEL);
    }
};
void
TestTree::testProcessBBox()
{
    using openvdb::Coord;
    using openvdb::CoordBBox;
    //check two leaf nodes and two tiles at each level 1, 2 and 3
    const int size[4]={1<<3, 1<<3, 1<<(3+4), 1<<(3+4+5)};
    for (int level=0; level<=3; ++level) {
        openvdb::FloatTree tree;
        const int n = size[level];
        const CoordBBox bbox[]={CoordBBox::createCube(Coord(-n,-n,-n), n),
                                CoordBBox::createCube(Coord( 0, 0, 0), n)};
        if (level==0) {
            tree.setValue(Coord(-1,-2,-3), 1.0f);
            tree.setValue(Coord( 1, 2, 3), 1.0f);
        } else {
            tree.fill(bbox[0], 1.0f, true);
            tree.fill(bbox[1], 1.0f, true);
        }
        BBoxOp op;
        tree.visitActiveBBox(op);
        CPPUNIT_ASSERT_EQUAL(2, int(op.bbox.size()));

        for (int i=0; i<2; ++i) {
            //std::cerr <<"\nLevel="<<level<<" op.bbox["<<i<<"]="<<op.bbox[i]
            //          <<" op.level["<<i<<"]= "<<op.level[i]<<std::endl;
            CPPUNIT_ASSERT_EQUAL(level,int(op.level[i]));
            CPPUNIT_ASSERT(op.bbox[i] == bbox[i]);
        }
    }
}

void
TestTree::testLeafManager()
{
    using openvdb::CoordBBox;
    using openvdb::Coord;
    using openvdb::Vec3f;
    using openvdb::FloatGrid;
    using openvdb::FloatTree;

    const Vec3f center(0.35f, 0.35f, 0.35f);
    const float radius = 0.15;
    const int dim = 128, half_width = 5;
    const float voxel_size = 1.0f/dim;

    FloatGrid::Ptr grid = FloatGrid::create(/*background=*/half_width*voxel_size);
    FloatTree& tree = grid->tree();
    grid->setTransform(openvdb::math::Transform::createLinearTransform(/*voxel size=*/voxel_size));

    unittest_util::makeSphere<FloatGrid>(
        Coord(dim), center, radius, *grid, unittest_util::SPHERE_SPARSE_NARROW_BAND);
    const size_t leafCount = tree.leafCount();

    //grid->print(std::cout, 3);
    {// test with no aux buffers
        openvdb::tree::LeafManager<FloatTree> r(tree);
        CPPUNIT_ASSERT_EQUAL(leafCount, r.leafCount());
        CPPUNIT_ASSERT_EQUAL(size_t(0), r.auxBufferCount());
        CPPUNIT_ASSERT_EQUAL(size_t(0), r.auxBuffersPerLeaf());
        size_t n = 0;
        for (FloatTree::LeafCIter iter=tree.cbeginLeaf(); iter; ++iter, ++n) {
            CPPUNIT_ASSERT(r.leaf(n) == *iter);
            CPPUNIT_ASSERT(r.getBuffer(n,0) == iter->buffer());
        }
        CPPUNIT_ASSERT_EQUAL(r.leafCount(), n);
        CPPUNIT_ASSERT(!r.swapBuffer(0,0));

        r.rebuildAuxBuffers(2);

        CPPUNIT_ASSERT_EQUAL(leafCount, r.leafCount());
        CPPUNIT_ASSERT_EQUAL(size_t(2), r.auxBuffersPerLeaf());
        CPPUNIT_ASSERT_EQUAL(size_t(2*leafCount),r.auxBufferCount());

         for (n=0; n<leafCount; ++n) {
            CPPUNIT_ASSERT(r.getBuffer(n,0) == r.getBuffer(n,1));
            CPPUNIT_ASSERT(r.getBuffer(n,1) == r.getBuffer(n,2));
            CPPUNIT_ASSERT(r.getBuffer(n,0) == r.getBuffer(n,2));
        }
    }
    {// test with 2 aux buffers
        openvdb::tree::LeafManager<FloatTree> r(tree, 2);
        CPPUNIT_ASSERT_EQUAL(leafCount, r.leafCount());
        CPPUNIT_ASSERT_EQUAL(size_t(2), r.auxBuffersPerLeaf());
        CPPUNIT_ASSERT_EQUAL(size_t(2*leafCount),r.auxBufferCount());
        size_t n = 0;
        for (FloatTree::LeafCIter iter=tree.cbeginLeaf(); iter; ++iter, ++n) {
            CPPUNIT_ASSERT(r.leaf(n) == *iter);
            CPPUNIT_ASSERT(r.getBuffer(n,0) == iter->buffer());

            CPPUNIT_ASSERT(r.getBuffer(n,0) == r.getBuffer(n,1));
            CPPUNIT_ASSERT(r.getBuffer(n,1) == r.getBuffer(n,2));
            CPPUNIT_ASSERT(r.getBuffer(n,0) == r.getBuffer(n,2));
        }
        CPPUNIT_ASSERT_EQUAL(r.leafCount(), n);
        for (n=0; n<leafCount; ++n) r.leaf(n).buffer().setValue(4,2.4f);
        for (n=0; n<leafCount; ++n) {
            CPPUNIT_ASSERT(r.getBuffer(n,0) != r.getBuffer(n,1));
            CPPUNIT_ASSERT(r.getBuffer(n,1) == r.getBuffer(n,2));
            CPPUNIT_ASSERT(r.getBuffer(n,0) != r.getBuffer(n,2));
        }
        r.syncAllBuffers();
        for (n=0; n<leafCount; ++n) {
            CPPUNIT_ASSERT(r.getBuffer(n,0) == r.getBuffer(n,1));
            CPPUNIT_ASSERT(r.getBuffer(n,1) == r.getBuffer(n,2));
            CPPUNIT_ASSERT(r.getBuffer(n,0) == r.getBuffer(n,2));
        }
        for (n=0; n<leafCount; ++n) r.getBuffer(n,1).setValue(4,5.4f);
        for (n=0; n<leafCount; ++n) {
            CPPUNIT_ASSERT(r.getBuffer(n,0) != r.getBuffer(n,1));
            CPPUNIT_ASSERT(r.getBuffer(n,1) != r.getBuffer(n,2));
            CPPUNIT_ASSERT(r.getBuffer(n,0) == r.getBuffer(n,2));
        }
        CPPUNIT_ASSERT(r.swapLeafBuffer(1));
        for (n=0; n<leafCount; ++n) {
            CPPUNIT_ASSERT(r.getBuffer(n,0) != r.getBuffer(n,1));
            CPPUNIT_ASSERT(r.getBuffer(n,1) == r.getBuffer(n,2));
            CPPUNIT_ASSERT(r.getBuffer(n,0) != r.getBuffer(n,2));
        }
        r.syncAuxBuffer(1);
        for (n=0; n<leafCount; ++n) {
            CPPUNIT_ASSERT(r.getBuffer(n,0) == r.getBuffer(n,1));
            CPPUNIT_ASSERT(r.getBuffer(n,1) != r.getBuffer(n,2));
            CPPUNIT_ASSERT(r.getBuffer(n,0) != r.getBuffer(n,2));
        }
        r.syncAuxBuffer(2);
        for (n=0; n<leafCount; ++n) {
            CPPUNIT_ASSERT(r.getBuffer(n,0) == r.getBuffer(n,1));
            CPPUNIT_ASSERT(r.getBuffer(n,1) == r.getBuffer(n,2));
        }
    }
    {// test with const tree (buffers are not swappable)
        openvdb::tree::LeafManager<const FloatTree> r(tree);

        for (size_t numAuxBuffers = 0; numAuxBuffers <= 2; ++numAuxBuffers += 2) {
            r.rebuildAuxBuffers(numAuxBuffers);

            CPPUNIT_ASSERT_EQUAL(leafCount, r.leafCount());
            CPPUNIT_ASSERT_EQUAL(int(numAuxBuffers * leafCount), int(r.auxBufferCount()));
            CPPUNIT_ASSERT_EQUAL(numAuxBuffers, r.auxBuffersPerLeaf());

            size_t n = 0;
            for (FloatTree::LeafCIter iter = tree.cbeginLeaf(); iter; ++iter, ++n) {
                CPPUNIT_ASSERT(r.leaf(n) == *iter);
                // Verify that each aux buffer was initialized with a copy of the leaf buffer.
                for (size_t bufIdx = 0; bufIdx < numAuxBuffers; ++bufIdx) {
                    CPPUNIT_ASSERT(r.getBuffer(n, bufIdx) == iter->buffer());
                }
            }
            CPPUNIT_ASSERT_EQUAL(r.leafCount(), n);

            for (size_t i = 0; i < numAuxBuffers; ++i) {
                for (size_t j = 0; j < numAuxBuffers; ++j) {
                    // Verify that swapping buffers with themselves and swapping
                    // leaf buffers with aux buffers have no effect.
                    const bool canSwap = (i != j && i != 0 && j != 0);
                    CPPUNIT_ASSERT_EQUAL(canSwap, r.swapBuffer(i, j));
                }
            }
        }
    }
}

// Copyright (c) 2012-2013 DreamWorks Animation LLC
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
