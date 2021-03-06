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
/// @file RootNode.h
///
/// @brief The root node of an OpenVDB tree

#ifndef OPENVDB_TREE_ROOTNODE_HAS_BEEN_INCLUDED
#define OPENVDB_TREE_ROOTNODE_HAS_BEEN_INCLUDED

#include <map>
#include <set>
#include <sstream>
#include <boost/type_traits/remove_const.hpp>
#include <openvdb/Exceptions.h>
#include <openvdb/Types.h>
#include <openvdb/io/Compression.h> // for truncateRealToHalf()
#include <openvdb/math/Math.h> // for isZero(), isExactlyEqual(), etc.
#include <openvdb/math/BBox.h>
#include <openvdb/util/NodeMasks.h> // for backward compatibility only (see readTopology())
#include <openvdb/version.h>
#include "Util.h"


namespace openvdb {
OPENVDB_USE_VERSION_NAMESPACE
namespace OPENVDB_VERSION_NAME {
namespace tree {

template<typename ChildType>
class RootNode
{
public:
    typedef ChildType                         ChildNodeType;
    typedef typename ChildType::LeafNodeType  LeafNodeType;
    typedef typename ChildType::ValueType     ValueType;

    static const Index LEVEL = 1 + ChildType::LEVEL; // level 0 = leaf

    /// @brief ValueConverter<T>::Type is the type of a RootNode having the same
    /// child hierarchy as this node but a different value type, T.
    template<typename OtherValueType>
    struct ValueConverter {
        typedef RootNode<typename ChildType::template ValueConverter<OtherValueType>::Type> Type;
    };


    /// Construct a new tree with a background value of 0.
    RootNode();

    /// Construct a new tree with the given background value.
    explicit RootNode(const ValueType& background);

    RootNode(const RootNode& other) { *this = other; }

    /// @brief Topology copy constructor that guarantees the
    /// configuration of the constructed tree is topologically
    /// identical to the other tree
    ///
    /// @details Reproduce the topology and active states of the other tree
    /// (which may have a different ValueType), but don't copy values.
    /// All values that are active in the other tree are set to the foreground value
    /// and all other values to the background value.
    /// @param other       the root node of a tree having (possibly) a different ValueType
    /// @param background  the value to which inactive tiles and voxels are initialized
    /// @param foreground  the value to which active tiles and voxels are initialized
    template<typename OtherChildType>
    RootNode(const RootNode<OtherChildType>& other,
        const ValueType& background, const ValueType& foreground,
             TopologyCopy);

    /// @brief Topology copy constructor that guarantees the
    /// configuration of the constructed tree is topologically
    /// identical to the other tree
    ///
    /// @note this copy constructor is generally faster then the one
    /// above that takes both a forground and a background value. Its
    /// main application is multithreading where the topology of
    /// the output tree exactly matches the input tree.
    ///
    /// @details Reproduce the topology and active states of the other node
    /// (which may have a different ValueType), but don't copy values.
    /// All values in the constructed tree are set to the background value
    /// regardless of their active states.
    /// @param other       the root node of a tree having (possibly) a different ValueType
    /// @param background  the value to which inactive tiles and voxels are initialized
    template<typename OtherChildType>
    RootNode(const RootNode<OtherChildType>& other, const ValueType& background,
             TopologyCopy);

    RootNode& operator=(const RootNode& other);

    ~RootNode() { this->clearTable(); }

private:
    struct Tile {
        Tile(): value(zeroVal<ValueType>()), active(false) {}
        Tile(const ValueType& v, bool b): value(v), active(b) {}
        ValueType value;
        bool      active;
    };

    // This lightweight struct pairs child pointers and tiles
    struct NodeStruct {
        ChildType* child;
        Tile       tile;

        NodeStruct(): child(NULL) {}
        NodeStruct(ChildType& c): child(&c) {}
        NodeStruct(const Tile& t): child(NULL), tile(t) {}
        ~NodeStruct() {} ///< @note doesn't delete child

        bool isChild() const { return child != NULL; }
        bool isTile() const { return child == NULL; }
        bool isTileOff() const { return isTile() && !tile.active; }
        bool isTileOn() const { return isTile() && tile.active; }

        void set(ChildType& c) { delete child; child = &c; }
        void set(const Tile& t) { delete child; child = NULL; tile = t; }
        ChildType& steal(const Tile& t) { ChildType* c = child; child = NULL; tile = t; return *c; }
    };

    typedef std::map<Coord, NodeStruct>      MapType;
    typedef typename MapType::iterator       MapIter;
    typedef typename MapType::const_iterator MapCIter;

    typedef std::set<Coord>                   CoordSet;
    typedef typename CoordSet::iterator       CoordSetIter;
    typedef typename CoordSet::const_iterator CoordSetCIter;

    static void             setTile(const MapIter& i, const Tile& t) { i->second.set(t); }
    static void             setChild(const MapIter& i, ChildType& c) { i->second.set(c); }
    static Tile&            getTile(const MapIter& i) { return i->second.tile; }
    static const Tile&      getTile(const MapCIter& i) { return i->second.tile; }
    static ChildType&       getChild(const MapIter& i) { return *(i->second.child); }
    static const ChildType& getChild(const MapCIter& i) { return *(i->second.child); }
    static ChildType&       stealChild(const MapIter& i, const Tile& t) {return i->second.steal(t);}
    static const ChildType& stealChild(const MapCIter& i,const Tile& t) {return i->second.steal(t);}

    static bool isChild(const MapCIter& i)   { return i->second.isChild(); }
    static bool isChild(const MapIter& i)    { return i->second.isChild(); }
    static bool isTile(const MapCIter& i)    { return i->second.isTile(); }
    static bool isTile(const MapIter& i)     { return i->second.isTile(); }
    static bool isTileOff(const MapCIter& i) { return i->second.isTileOff(); }
    static bool isTileOff(const MapIter& i)  { return i->second.isTileOff(); }
    static bool isTileOn(const MapCIter& i)  { return i->second.isTileOn(); }
    static bool isTileOn(const MapIter& i)   { return i->second.isTileOn(); }

    struct NullPred {
        static inline bool test(const MapIter&) { return true; }
        static inline bool test(const MapCIter&) { return true; }
    };
    struct ValueOnPred {
        static inline bool test(const MapIter& i) { return isTileOn(i); }
        static inline bool test(const MapCIter& i) { return isTileOn(i); }
    };
    struct ValueOffPred {
        static inline bool test(const MapIter& i) { return isTileOff(i); }
        static inline bool test(const MapCIter& i) { return isTileOff(i); }
    };
    struct ValueAllPred {
        static inline bool test(const MapIter& i) { return isTile(i); }
        static inline bool test(const MapCIter& i) { return isTile(i); }
    };
    struct ChildOnPred {
        static inline bool test(const MapIter& i) { return isChild(i); }
        static inline bool test(const MapCIter& i) { return isChild(i); }
    };
    struct ChildOffPred {
        static inline bool test(const MapIter& i) { return isTile(i); }
        static inline bool test(const MapCIter& i) { return isTile(i); }
    };

    template<typename _RootNodeT, typename _MapIterT, typename FilterPredT>
    class BaseIter
    {
    public:
        typedef _RootNodeT RootNodeT;
        typedef _MapIterT MapIterT; // either MapIter or MapCIter

        bool operator==(const BaseIter& other) const
        {
            return (mParentNode == other.mParentNode) && (mIter == other.mIter);
        }
        bool operator!=(const BaseIter& other) const { return !(*this == other); }

        RootNodeT* getParentNode() const { return mParentNode; }
        /// Return a reference to the node over which this iterator iterates.
        RootNodeT& parent() const
        {
            if (!mParentNode) OPENVDB_THROW(ValueError, "iterator references a null parent node");
            return *mParentNode;
        }

        bool test() const { assert(mParentNode); return mIter != mParentNode->mTable.end(); }
        operator bool() const { return this->test(); }

        void increment() { ++mIter; this->skip(); }
        bool next() { this->increment(); return this->test(); }
        void increment(Index n) { for (int i = 0; i < n && this->next(); ++i) {} }

        /// @brief Return this iterator's position as an offset from
        /// the beginning of the parent node's map.
        Index pos() const
        {
            return !mParentNode ? 0U : Index(std::distance(mParentNode->mTable.begin(), mIter));
        }

        bool isValueOn() const { return RootNodeT::isTileOn(mIter); }
        bool isValueOff() const { return RootNodeT::isTileOff(mIter); }
        void setValueOn(bool on = true) const { mIter->second.tile.active = on; }
        void setValueOff() const { mIter->second.tile.active = false; }

        /// Return the coordinates of the item to which this iterator is pointing.
        Coord getCoord() const { return mIter->first; }
        /// Return in @a xyz the coordinates of the item to which this iterator is pointing.
        void getCoord(Coord& xyz) const { xyz = this->getCoord(); }

    protected:
        BaseIter(): mParentNode(NULL) {}
        BaseIter(RootNodeT& parent, const MapIterT& iter): mParentNode(&parent), mIter(iter) {}

        void skip() { while (this->test() && !FilterPredT::test(mIter)) ++mIter; }

        RootNodeT* mParentNode;
        MapIterT mIter;
    }; // BaseIter

    template<typename RootNodeT, typename MapIterT, typename FilterPredT, typename ChildNodeT>
    class ChildIter: public BaseIter<RootNodeT, MapIterT, FilterPredT>
    {
    public:
        typedef BaseIter<RootNodeT, MapIterT, FilterPredT> BaseT;
        typedef RootNodeT NodeType;
        typedef NodeType ValueType;
        typedef ChildNodeT ChildNodeType;
        typedef typename boost::remove_const<NodeType>::type NonConstNodeType;
        typedef typename boost::remove_const<ValueType>::type NonConstValueType;
        typedef typename boost::remove_const<ChildNodeType>::type NonConstChildNodeType;
        using BaseT::mIter;

        ChildIter() {}
        ChildIter(RootNodeT& parent, const MapIterT& iter): BaseT(parent, iter) { BaseT::skip(); }

        ChildIter& operator++() { BaseT::increment(); return *this; }

        ChildNodeT& getValue() const { return getChild(mIter); }
        ChildNodeT& operator*() const { return this->getValue(); }
        ChildNodeT* operator->() const { return &this->getValue(); }
    }; // ChildIter

    template<typename RootNodeT, typename MapIterT, typename FilterPredT, typename ValueT>
    class ValueIter: public BaseIter<RootNodeT, MapIterT, FilterPredT>
    {
    public:
        typedef BaseIter<RootNodeT, MapIterT, FilterPredT> BaseT;
        typedef RootNodeT NodeType;
        typedef ValueT ValueType;
        typedef typename boost::remove_const<NodeType>::type NonConstNodeType;
        typedef typename boost::remove_const<ValueT>::type NonConstValueType;
        using BaseT::mIter;

        ValueIter() {}
        ValueIter(RootNodeT& parent, const MapIterT& iter): BaseT(parent, iter) { BaseT::skip(); }

        ValueIter& operator++() { BaseT::increment(); return *this; }

        ValueT& getValue() const { return getTile(mIter).value; }
        ValueT& operator*() const { return this->getValue(); }
        ValueT* operator->() const { return &(this->getValue()); }

        void setValue(const ValueT& v) const { assert(isTile(mIter)); getTile(mIter).value = v; }
    }; // ValueIter

    template<typename RootNodeT, typename MapIterT, typename ChildNodeT, typename ValueT>
    class DenseIter: public BaseIter<RootNodeT, MapIterT, NullPred>
    {
    public:
        typedef BaseIter<RootNodeT, MapIterT, NullPred> BaseT;
        typedef RootNodeT NodeType;
        typedef ValueT ValueType;
        typedef ChildNodeT ChildNodeType;
        typedef typename boost::remove_const<NodeType>::type NonConstNodeType;
        typedef typename boost::remove_const<ValueT>::type NonConstValueType;
        typedef typename boost::remove_const<ChildNodeT>::type NonConstChildNodeType;
        using BaseT::mIter;

        DenseIter() {}
        DenseIter(RootNodeT& parent, const MapIterT& iter): BaseT(parent, iter) {}

        DenseIter& operator++() { BaseT::increment(); return *this; }

        bool isChildNode() const { return isChild(mIter); }

        ChildNodeT* probeChild(NonConstValueType& value) const
        {
            if (isChild(mIter)) return &getChild(mIter);
            value = getTile(mIter).value;
            return NULL;
        }
        bool probeChild(ChildNodeT*& child, NonConstValueType& value) const
        {
            child = this->probeChild(value);
            return child != NULL;
        }
        bool probeValue(NonConstValueType& value) const { return !this->probeChild(value); }

        void setChild(ChildNodeT& c) const { RootNodeT::setChild(mIter, c); }
        void setChild(ChildNodeT* c) const { assert(c != NULL); RootNodeT::setChild(mIter, *c); }
        void setValue(const ValueT& v) const
        {
            if (isTile(mIter)) getTile(mIter).value = v;
            /// @internal For consistency with iterators for other node types
            /// (see, e.g., InternalNode::DenseIter::unsetItem()), we don't call
            /// setTile() here, because that would also delete the child.
            else stealChild(mIter, Tile(v, /*active=*/true));
        }
    }; // DenseIter

public:
    typedef ChildIter<RootNode, MapIter, ChildOnPred, ChildType>                  ChildOnIter;
    typedef ChildIter<const RootNode, MapCIter, ChildOnPred, const ChildType>     ChildOnCIter;
    typedef ValueIter<RootNode, MapIter, ChildOffPred, const ValueType>           ChildOffIter;
    typedef ValueIter<const RootNode, MapCIter, ChildOffPred, ValueType>          ChildOffCIter;
    typedef DenseIter<RootNode, MapIter, ChildType, ValueType>                    ChildAllIter;
    typedef DenseIter<const RootNode, MapCIter, const ChildType, const ValueType> ChildAllCIter;

    typedef ValueIter<RootNode, MapIter, ValueOnPred, ValueType>                  ValueOnIter;
    typedef ValueIter<const RootNode, MapCIter, ValueOnPred, const ValueType>     ValueOnCIter;
    typedef ValueIter<RootNode, MapIter, ValueOffPred, ValueType>                 ValueOffIter;
    typedef ValueIter<const RootNode, MapCIter, ValueOffPred, const ValueType>    ValueOffCIter;
    typedef ValueIter<RootNode, MapIter, ValueAllPred, ValueType>                 ValueAllIter;
    typedef ValueIter<const RootNode, MapCIter, ValueAllPred, const ValueType>    ValueAllCIter;


    ChildOnCIter  cbeginChildOn()  const { return ChildOnCIter(*this, mTable.begin()); }
    ChildOffCIter cbeginChildOff() const { return ChildOffCIter(*this, mTable.begin()); }
    ChildAllCIter cbeginChildAll() const { return ChildAllCIter(*this, mTable.begin()); }
    ChildOnCIter   beginChildOn()  const { return cbeginChildOn(); }
    ChildOffCIter  beginChildOff() const { return cbeginChildOff(); }
    ChildAllCIter  beginChildAll() const { return cbeginChildAll(); }
    ChildOnIter    beginChildOn()  { return ChildOnIter(*this, mTable.begin()); }
    ChildOffIter   beginChildOff() { return ChildOffIter(*this, mTable.begin()); }
    ChildAllIter   beginChildAll() { return ChildAllIter(*this, mTable.begin()); }

    ValueOnCIter  cbeginValueOn()  const { return ValueOnCIter(*this, mTable.begin()); }
    ValueOffCIter cbeginValueOff() const { return ValueOffCIter(*this, mTable.begin()); }
    ValueAllCIter cbeginValueAll() const { return ValueAllCIter(*this, mTable.begin()); }
    ValueOnCIter   beginValueOn()  const { return cbeginValueOn(); }
    ValueOffCIter  beginValueOff() const { return cbeginValueOff(); }
    ValueAllCIter  beginValueAll() const { return cbeginValueAll(); }
    ValueOnIter    beginValueOn()  { return ValueOnIter(*this, mTable.begin()); }
    ValueOffIter   beginValueOff() { return ValueOffIter(*this, mTable.begin()); }
    ValueAllIter   beginValueAll() { return ValueAllIter(*this, mTable.begin()); }

    /// Return the total amount of memory in bytes occupied by this node and its children.
    Index64 memUsage() const;

    /// @brief Expand the specified bbox so it includes the active tiles of
    /// this root node as well as all the active values in its child nodes.
    void evalActiveVoxelBoundingBox(CoordBBox& bbox) const;

    /// Return the bounding box of this RootNode, i.e., an infinite bounding box.
    static CoordBBox getNodeBoundingBox() { return CoordBBox::inf(); }

    /// @brief Change inactive tiles or voxels with a value equal to +/- the
    /// old background to the specified value (with the same sign). Active values
    /// are unchanged.
    void setBackground(const ValueType& value);
    /// Return the background value
    const ValueType& background() const { return mBackground; }
    /// @deprecated Use background()
    OPENVDB_DEPRECATED ValueType getBackground() const { return mBackground; }

    /// Return @c true if the given tile is inactive and has the background value.
    bool isBackgroundTile(const Tile&) const;
    //@{
    /// Return @c true if the given iterator points to an inactive tile with the background value.
    bool isBackgroundTile(const MapIter&) const;
    bool isBackgroundTile(const MapCIter&) const;
    //@}

    /// Return the number of background tiles.
    size_t numBackgroundTiles() const;
    /// @brief Remove all background tiles.
    /// @return the number of tiles removed.
    size_t eraseBackgroundTiles();
    void clear() { this->clearTable(); }

    /// Return @c true if this node's table is either empty or contains only background tiles.
    bool empty() const { return mTable.size() == numBackgroundTiles(); }

    /// @brief Expand this node's table so that (x, y, z) is included in the index range.
    /// @return @c true if an expansion was performed (i.e., if (x, y, z) was not already
    /// included in the index range).
    bool expand(const Coord& xyz);

    static Index getLevel() { return LEVEL; }
    static void getNodeLog2Dims(std::vector<Index>& dims);
    static Index getChildDim() { return ChildType::DIM; }

    /// Return the number of entries in this node's table.
    Index getTableSize() const { return mTable.size(); }

    Index getWidth() const { return this->getMaxIndex()[0] - this->getMinIndex()[0]; }
    Index getHeight() const { return this->getMaxIndex()[1] - this->getMinIndex()[1]; }
    Index getDepth() const { return this->getMaxIndex()[2] - this->getMinIndex()[2]; }

    /// Return the smallest index of the current tree.
    Coord getMinIndex() const;
    /// Return the largest index of the current tree.
    Coord getMaxIndex() const;
    /// Return the current index range.  Both min and max are inclusive.
    void getIndexRange(CoordBBox& bbox) const;

     /// @brief Return @c true if the given tree has the same node and active value
    /// topology as this tree (but possibly a different @c ValueType).
    template<typename OtherChildType>
    bool hasSameTopology(const RootNode<OtherChildType>& other) const;

    /// Return @c false if the other node's dimensions don't match this node's.
    template<typename OtherChildType>
    static bool hasSameConfiguration(const RootNode<OtherChildType>& other);

    Index32 leafCount() const;
    Index32 nonLeafCount() const;
    Index64 onVoxelCount() const;
    Index64 offVoxelCount() const;
    Index64 onLeafVoxelCount() const;
    Index64 offLeafVoxelCount() const;

    bool isValueOn(const Coord& xyz) const;

    bool hasActiveTiles() const;

    const ValueType& getValue(const Coord& xyz) const;
    bool probeValue(const Coord& xyz, ValueType& value) const;

    /// @brief Return the tree depth (0 = root) at which the value of voxel (x, y, z) resides.
    /// @details If (x, y, z) isn't explicitly represented in the tree (i.e.,
    /// it is implicitly a background voxel), return -1.
    int getValueDepth(const Coord& xyz) const;

    /// Set the active state of the voxel at the given coordinates, but don't change its value.
    void setActiveState(const Coord& xyz, bool on);

    /// Mark the voxel at the given coordinates as inactive, but don't change its value.
    void setValueOff(const Coord& xyz);
    /// Change the value of the voxel at the given coordinates and mark the voxel as inactive.
    void setValueOff(const Coord& xyz, const ValueType& value);

    void setValueOn(const Coord& xyz, const ValueType& value);
    void setValueOnly(const Coord& xyz, const ValueType& value);
    void setValueOnMin(const Coord& xyz, const ValueType& value);
    void setValueOnMax(const Coord& xyz, const ValueType& value);
    void setValueOnSum(const Coord& xyz, const ValueType& value);

    /// @brief Set all voxels within a given box to a constant value, if necessary
    /// subdividing tiles that intersect the box.
    /// @param bbox           inclusive coordinates of opposite corners of an axis-aligned box
    /// @param value          the value to which to set voxels within the box
    /// @param active         if true, mark voxels within the box as active,
    ///                       otherwise mark them as inactive
    void fill(const CoordBBox& bbox, const ValueType& value, bool active = true);


    //
    // I/O
    //
    bool writeTopology(std::ostream&, bool toHalf = false) const;
    bool readTopology(std::istream&, bool fromHalf = false);

    void writeBuffers(std::ostream&, bool toHalf = false) const;
    void readBuffers(std::istream&, bool fromHalf = false);

    /// Return the value of the voxel at the given coordinates and, if necessary, update
    /// the accessor with pointers to the nodes along the path from the root node to
    /// the node containing the voxel.
    /// @note Used internally by ValueAccessor.
    template<typename AccessorT>
    const ValueType& getValueAndCache(const Coord& xyz, AccessorT&) const;
    /// Return @c true if the voxel at the given coordinates is active and, if necessary,
    /// update the accessor with pointers to the nodes along the path from the root node
    /// to the node containing the voxel.
    /// @note Used internally by ValueAccessor.
    template<typename AccessorT>
    bool isValueOnAndCache(const Coord& xyz, AccessorT&) const;

    /// Change the value of the voxel at the given coordinates and mark it as active.
    /// If necessary, update the accessor with pointers to the nodes along the path
    /// from the root node to the node containing the voxel.
    /// @note Used internally by ValueAccessor.
    template<typename AccessorT>
    void setValueAndCache(const Coord& xyz, const ValueType& value, AccessorT&);

    /// Set the value of the voxel at the given coordinate but preserves its active state.
    /// If necessary, update the accessor with pointers to the nodes along the path
    /// from the root node to the node containing the voxel.
    /// @note Used internally by ValueAccessor.
    template<typename AccessorT>
    void setValueOnlyAndCache(const Coord& xyz, const ValueType& value, AccessorT&);

    /// Set the value of the voxel at the given coordinates to the sum of its current
    /// value and the given value, and mark the voxel as active.
    /// If necessary, update the accessor with pointers to the nodes along the path
    /// from the root node to the node containing the voxel.
    /// @note Used internally by ValueAccessor.
    template<typename AccessorT>
    void setValueOnSumAndCache(const Coord& xyz, const ValueType& value, AccessorT&);

    /// Change the value of the voxel at the given coordinates and mark it as inactive.
    /// If necessary, update the accessor with pointers to the nodes along the path
    /// from the root node to the node containing the voxel.
    /// @note Used internally by ValueAccessor.
    template<typename AccessorT>
    void setValueOffAndCache(const Coord& xyz, const ValueType& value, AccessorT&);

    /// Set the active state of the voxel at the given coordinates without changing its value.
    /// If necessary, update the accessor with pointers to the nodes along the path
    /// from the root node to the node containing the voxel.
    /// @note Used internally by ValueAccessor.
    template<typename AccessorT>
    void setActiveStateAndCache(const Coord& xyz, bool on, AccessorT&);

    /// Return @c true if the voxel at the given coordinates is active, assigns the voxel
    /// value, and, if necessary, update the accessor with pointers to the nodes along
    /// the path from the root node to the node containing the voxel.
    /// @note Used internally by ValueAccessor.
    template<typename AccessorT>
    bool probeValueAndCache(const Coord& xyz, ValueType& value, AccessorT&) const;

    /// Return the tree depth (0 = root) at which the value of voxel (x, y, z) resides.
    /// If (x, y, z) isn't explicitly represented in the tree (i.e., it is implicitly
    /// a background voxel), return -1. If necessary, update the accessor with pointers
    /// to the nodes along the path from the root node to the node containing the voxel.
    /// @note Used internally by ValueAccessor.
    template<typename AccessorT>
    int getValueDepthAndCache(const Coord& xyz, AccessorT&) const;

    /// Call the @c PruneOp functor for each child node and, if the functor
    /// returns @c true, prune the node and replace it with a tile.
    ///
    /// This method is used to implement all of the various pruning algorithms
    /// (prune(), pruneInactive(), etc.).  It should rarely be called directly.
    /// @see openvdb/tree/Util.h for the definition of the @c PruneOp functor
    template<typename PruneOp> void pruneOp(PruneOp&);

    /// @brief Reduce the memory footprint of this tree by replacing with tiles
    /// any nodes whose values are all the same (optionally to within a tolerance)
    /// and have the same active state.
    void prune(const ValueType& tolerance = zeroVal<ValueType>());

    /// @brief Reduce the memory footprint of this tree by replacing with
    /// tiles of the given value any nodes whose values are all inactive.
    void pruneInactive(const ValueType&);

    /// @brief Reduce the memory footprint of this tree by replacing with
    /// background tiles any nodes whose values are all inactive.
    void pruneInactive();

    /// @brief Return the leaf node that contains voxel (x, y, z).
    /// If no such node exists, create one, but preserve the values and
    /// active states of all voxels.
    ///
    /// @details Use this method to preallocate a static tree topology
    /// over which to safely perform multithreaded processing.
    LeafNodeType* touchLeaf(const Coord& xyz);

    /// @brief Return a pointer to the leaf node that contains voxel (x, y, z).
    /// If no such node exists, return NULL.
    LeafNodeType* probeLeaf(const Coord& xyz);
    /// @brief Return a const pointer to the leaf node that contains voxel (x, y, z).
    /// If no such node exists, return NULL.
    const LeafNodeType* probeConstLeaf(const Coord& xyz) const;

    /// @brief Same as touchLeaf except, if necessary, it update the accessor with pointers
    /// to the nodes along the path from the root node to the node containing the coordinate.
    template<typename AccessorT>
    LeafNodeType* touchLeafAndCache(const Coord& xyz, AccessorT&);

    /// @brief Same as probeLeaf except, if necessary, it update the accessor with pointers
    /// to the nodes along the path from the root node to the node containing the coordinate.
    template<typename AccessorT>
    LeafNodeType* probeLeafAndCache(const Coord& xyz, AccessorT&);
    /// @brief Same as probeConstLeaf except, if necessary, it update the accessor with pointers
    /// to the nodes along the path from the root node to the node containing the coordinate.
    template<typename AccessorT>
    const LeafNodeType* probeConstLeafAndCache(const Coord& xyz, AccessorT&) const;

    /// @brief Set the values of all inactive voxels and tiles of a narrow-band
    /// level set from the signs of the active voxels, setting outside values to
    /// +background and inside values to -background.
    ///
    /// @note This method should only be used on closed, narrow-band level sets!
    void signedFloodFill();

    /// @brief Set the values of all inactive voxels and tiles of a narrow-band
    /// level set from the signs of the active voxels, setting outside values to
    /// "outside" and inside values to "inside. The background value of the grid
    /// will be set to the "outside" value.
    ///
    /// @note This method should only be used on closed, narrow-band level sets!
    void signedFloodFill(const ValueType& outside, const ValueType& inside);

    /// Move child nodes from the other tree into this tree wherever those nodes
    /// correspond to constant-value tiles in this tree, and replace leaf-level
    /// inactive voxels in this tree with corresponding voxels in the other tree
    /// that are active.
    /// @note This operation always empties the other tree, whether or not any nodes were moved.
    void merge(RootNode& other);

    /// Turn active tiles into dense voxels, i.e., into leaf nodes that are entirely active.
    void voxelizeActiveTiles();

    /// @brief Union this tree's set of active values with the active values
    /// of the other tree, whose @c ValueType may be different.
    /// @details The resulting state of a value is active if the corresponding value
    /// was already active OR if it is active in the other tree.  Also, a resulting
    /// value maps to a voxel if the corresponding value already mapped to a voxel
    /// OR if it is a voxel in the other tree.  Thus, a resulting value can only
    /// map to a tile if the corresponding value already mapped to a tile
    /// AND if it is a tile value in other tree.
    ///
    /// @note This operation modifies only active states, not values.
    /// Specifically, active tiles and voxels in this tree are not changed, and
    /// tiles or voxels that were inactive in this tree but active in the other tree
    /// are marked as active in this tree but left with their original values.
    template<typename OtherChildType>
    void topologyUnion(const RootNode<OtherChildType>& other);

    template<typename CombineOp>
    void combine(RootNode& other, CombineOp&, bool prune = false);

    template<typename CombineOp>
    void combine2(const RootNode& other0, const RootNode& other1,
                  CombineOp& op, bool prune = false);

    /// @brief Calls the templated functor BBoxOp with bounding box
    /// information for all active tiles and leaf nodes in the tree.
    /// An additional level argument is provided for each callback.
    ///
    /// @note The bounding boxes are guarenteed to be non-overlapping.
    template<typename BBoxOp> void visitActiveBBox(BBoxOp&) const;

    template<typename VisitorOp> void visit(VisitorOp&);
    template<typename VisitorOp> void visit(VisitorOp&) const;

    template<typename OtherRootNodeType, typename VisitorOp>
    void visit2(OtherRootNodeType& other, VisitorOp&);
    template<typename OtherRootNodeType, typename VisitorOp>
    void visit2(OtherRootNodeType& other, VisitorOp&) const;

private:
    /// During topology-only construction, access is needed
    /// to protected/private members of other template instances.
    template<typename> friend class RootNode;

    /// Currently no-op, but can be used to define empty and delete keys for mTable
    void initTable() {}
    inline void clearTable();
    //@{
    /// @internal Used by doVisit2().
    void resetTable(MapType& table) { mTable.swap(table); table.clear(); }
    void resetTable(const MapType&) const {}
    //@}

    Index getChildCount() const;
    Index getTileCount() const;
    Index getActiveTileCount() const;
    Index getInactiveTileCount() const;

    /// Return a MapType key for the given coordinates.
    static Coord coordToKey(const Coord& xyz) { return xyz & ~(ChildType::DIM - 1); }

    /// Insert this node's mTable keys into the given set.
    void insertKeys(CoordSet&) const;

    /// Return @c true if this node's mTable contains the given key.
    bool hasKey(const Coord& key) const { return mTable.find(key) != mTable.end(); }
    //@{
    /// @brief Look up the given key in this node's mTable.
    /// @return an iterator pointing to the matching mTable entry or to mTable.end().
    MapIter findKey(const Coord& key) { return mTable.find(key); }
    MapCIter findKey(const Coord& key) const { return mTable.find(key); }
    //@}
    //@{
    /// @brief Convert the given coordinates to a key and look the key up in this node's mTable.
    /// @return an iterator pointing to the matching mTable entry or to mTable.end().
    MapIter findCoord(const Coord& xyz) { return mTable.find(coordToKey(xyz)); }
    MapCIter findCoord(const Coord& xyz) const { return mTable.find(coordToKey(xyz)); }
    //@}
    /// @brief Convert the given coordinates to a key and look the key up in this node's mTable.
    /// @details If the key is not found, insert a background tile with that key.
    /// @return an iterator pointing to the matching mTable entry.
    MapIter findOrAddCoord(const Coord& xyz);

    /// @throw TypeError if the other node's dimensions don't match this node's.
    template<typename OtherChildType>
    static void enforceSameConfiguration(const RootNode<OtherChildType>& other);

    template<typename RootNodeT, typename VisitorOp, typename ChildAllIterT>
    static inline void doVisit(RootNodeT&, VisitorOp&);

    template<typename RootNodeT, typename OtherRootNodeT, typename VisitorOp,
        typename ChildAllIterT, typename OtherChildAllIterT>
    static inline void doVisit2(RootNodeT&, OtherRootNodeT&, VisitorOp&);


    MapType mTable;
    ValueType mBackground;
}; // end of RootNode class


////////////////////////////////////////


template<typename ChildT>
inline
RootNode<ChildT>::RootNode(): mBackground(zeroVal<ValueType>())
{
    this->initTable();
}


template<typename ChildT>
inline
RootNode<ChildT>::RootNode(const ValueType& background) : mBackground(background)
{
    this->initTable();
}


template<typename ChildT>
template<typename OtherChildType>
inline
RootNode<ChildT>::RootNode(const RootNode<OtherChildType>& other,
    const ValueType& backgd, const ValueType& foregd, TopologyCopy):
    mBackground(backgd)
{
    typedef RootNode<OtherChildType> OtherRootT;

    /// @todo Can this be avoided with partial specialization?
    enforceSameConfiguration(other);

    const Tile bgTile(backgd, /*active=*/false), fgTile(foregd, true);
    this->initTable();

    for (typename OtherRootT::MapCIter i=other.mTable.begin(), e=other.mTable.end(); i != e; ++i) {
        mTable[i->first] = OtherRootT::isTile(i)
            ? NodeStruct(OtherRootT::isTileOn(i) ? fgTile : bgTile)
            : NodeStruct(*(new ChildT(OtherRootT::getChild(i), backgd, foregd, TopologyCopy())));
    }
}


template<typename ChildT>
template<typename OtherChildType>
inline
RootNode<ChildT>::RootNode(const RootNode<OtherChildType>& other,
    const ValueType& backgd, TopologyCopy):
    mBackground(backgd)
{
    typedef RootNode<OtherChildType> OtherRootT;

    /// @todo Can this be avoided with partial specialization?
    enforceSameConfiguration(other);

    const Tile bgTile(backgd, /*active=*/false), fgTile(backgd, true);
    this->initTable();
    for (typename OtherRootT::MapCIter i=other.mTable.begin(), e=other.mTable.end(); i != e; ++i) {
        mTable[i->first] = OtherRootT::isTile(i)
            ? NodeStruct(OtherRootT::isTileOn(i) ? fgTile : bgTile)
            : NodeStruct(*(new ChildT(OtherRootT::getChild(i), backgd, TopologyCopy())));
    }
}


template<typename ChildT>
inline RootNode<ChildT>&
RootNode<ChildT>::operator=(const RootNode& other)
{
    mBackground = other.mBackground;

    this->clearTable();
    this->initTable();

    for (MapCIter i = other.mTable.begin(), e = other.mTable.end(); i != e; ++i) {
        mTable[i->first] =
            isTile(i) ? NodeStruct(getTile(i)) : NodeStruct(*(new ChildT(getChild(i))));
    }
    return *this;
}


////////////////////////////////////////


template<typename ChildT>
inline void
RootNode<ChildT>::setBackground(const ValueType& background)
{
    if (math::isExactlyEqual(background, mBackground)) return;

    // Traverse the tree, replacing occurrences of mBackground with background
    // and -mBackground with -background.
    for (MapIter iter=mTable.begin(); iter!=mTable.end(); ++iter) {
        ChildT *child = iter->second.child;
        if (child) {
            child->resetBackground(/*old=*/mBackground, /*new=*/background);
        } else {
            Tile& tile = getTile(iter);
            if (tile.active) continue;//only change inactive tiles
            if (math::isApproxEqual(tile.value, mBackground)) {
                tile.value = background;
            } else if (math::isApproxEqual(tile.value, negative(mBackground))) {
                tile.value = negative(background);
            }
        }
    }
    mBackground = background;
}


template<typename ChildT>
inline bool
RootNode<ChildT>::isBackgroundTile(const Tile& tile) const
{
    return !tile.active && math::isApproxEqual(tile.value, mBackground);
}

template<typename ChildT>
inline bool
RootNode<ChildT>::isBackgroundTile(const MapIter& iter) const
{
    return isTileOff(iter) && math::isApproxEqual(getTile(iter).value, mBackground);
}

template<typename ChildT>
inline bool
RootNode<ChildT>::isBackgroundTile(const MapCIter& iter) const
{
    return isTileOff(iter) && math::isApproxEqual(getTile(iter).value, mBackground);
}


template<typename ChildT>
inline size_t
RootNode<ChildT>::numBackgroundTiles() const
{
    size_t count = 0;
    for (MapCIter i = mTable.begin(), e = mTable.end(); i != e; ++i) {
        if (this->isBackgroundTile(i)) ++count;
    }
    return count;
}


template<typename ChildT>
inline size_t
RootNode<ChildT>::eraseBackgroundTiles()
{
    std::set<Coord> keysToErase;
    for (MapCIter i = mTable.begin(), e = mTable.end(); i != e; ++i) {
        if (this->isBackgroundTile(i)) keysToErase.insert(i->first);
    }
    for (std::set<Coord>::iterator i = keysToErase.begin(), e = keysToErase.end(); i != e; ++i) {
        mTable.erase(*i);
    }
    return keysToErase.size();
}


////////////////////////////////////////


template<typename ChildT>
inline void
RootNode<ChildT>::insertKeys(CoordSet& keys) const
{
    for (MapCIter i = mTable.begin(), e = mTable.end(); i != e; ++i) {
        keys.insert(i->first);
    }
}


template<typename ChildT>
inline typename RootNode<ChildT>::MapIter
RootNode<ChildT>::findOrAddCoord(const Coord& xyz)
{
    const Coord key = coordToKey(xyz);
    std::pair<MapIter, bool> result = mTable.insert(
        typename MapType::value_type(key, NodeStruct(Tile(mBackground, /*active=*/false))));
    return result.first;
}


template<typename ChildT>
inline bool
RootNode<ChildT>::expand(const Coord& xyz)
{
    const Coord key = coordToKey(xyz);
    std::pair<MapIter, bool> result = mTable.insert(
        typename MapType::value_type(key, NodeStruct(Tile(mBackground, /*active=*/false))));
    return result.second; // return true if the key did not already exist
}


////////////////////////////////////////


template<typename ChildT>
inline void
RootNode<ChildT>::getNodeLog2Dims(std::vector<Index>& dims)
{
    dims.push_back(0); // magic number; RootNode has no Log2Dim
    ChildT::getNodeLog2Dims(dims);
}


template<typename ChildT>
inline Coord
RootNode<ChildT>::getMinIndex() const
{
    return mTable.empty() ? Coord(0) : mTable.begin()->first;
}

template<typename ChildT>
inline Coord
RootNode<ChildT>::getMaxIndex() const
{
    return mTable.empty() ? Coord(0) : mTable.rbegin()->first + Coord(ChildT::DIM - 1);
}


template<typename ChildT>
inline void
RootNode<ChildT>::getIndexRange(CoordBBox& bbox) const
{
    bbox.min() = this->getMinIndex();
    bbox.max() = this->getMaxIndex();
}


////////////////////////////////////////


template<typename ChildT>
template<typename OtherChildType>
inline bool
RootNode<ChildT>::hasSameTopology(const RootNode<OtherChildType>& other) const
{
    typedef RootNode<OtherChildType> OtherRootT;
    typedef typename OtherRootT::MapType OtherMapT;
    typedef typename OtherRootT::MapIter OtherIterT;
    typedef typename OtherRootT::MapCIter OtherCIterT;

    if (!hasSameConfiguration(other)) return false;

    // Create a local copy of the other node's table.
    OtherMapT copyOfOtherTable = other.mTable;

    // For each entry in this node's table...
    for (MapCIter thisIter = mTable.begin(); thisIter != mTable.end(); ++thisIter) {
        if (this->isBackgroundTile(thisIter)) continue; // ignore background tiles

        // Fail if there is no corresponding entry in the other node's table.
        OtherCIterT otherIter = other.findKey(thisIter->first);
        if (otherIter == other.mTable.end()) return false;

        // Fail if this entry is a tile and the other is a child or vice-versa.
        if (isChild(thisIter)) {//thisIter points to a child
            if (OtherRootT::isTile(otherIter)) return false;
            // Fail if both entries are children, but the children have different topology.
            if (!getChild(thisIter).hasSameTopology(&OtherRootT::getChild(otherIter))) return false;
        } else {//thisIter points to a tile
            if (OtherRootT::isChild(otherIter)) return false;
            if (getTile(thisIter).active != OtherRootT::getTile(otherIter).active) return false;
        }

        // Remove tiles and child nodes with matching topology from
        // the copy of the other node's table. This is required since
        // the two root tables can include an arbitrary number of
        // background tiles and still have the same topology!
        copyOfOtherTable.erase(otherIter->first);
    }
    // Fail if the remaining entries in copyOfOtherTable are not all background tiles.
    for (OtherIterT i = copyOfOtherTable.begin(), e = copyOfOtherTable.end(); i != e; ++i) {
        if (!other.isBackgroundTile(i)) return false;
    }
    return true;
}


template<typename ChildT>
template<typename OtherChildType>
inline bool
RootNode<ChildT>::hasSameConfiguration(const RootNode<OtherChildType>&)
{
    std::vector<Index> thisDims, otherDims;
    RootNode::getNodeLog2Dims(thisDims);
    RootNode<OtherChildType>::getNodeLog2Dims(otherDims);
    return (thisDims == otherDims);
}


template<typename ChildT>
template<typename OtherChildType>
inline void
RootNode<ChildT>::enforceSameConfiguration(const RootNode<OtherChildType>&)
{
    std::vector<Index> thisDims, otherDims;
    RootNode::getNodeLog2Dims(thisDims);
    RootNode<OtherChildType>::getNodeLog2Dims(otherDims);
    if (thisDims != otherDims) {
        std::ostringstream ostr;
        ostr << "grids have incompatible configurations (" << thisDims[0];
        for (size_t i = 1, N = thisDims.size(); i < N; ++i) ostr << " x " << thisDims[i];
        ostr << " vs. " << otherDims[0];
        for (size_t i = 1, N = otherDims.size(); i < N; ++i) ostr << " x " << otherDims[i];
        ostr << ")";
        OPENVDB_THROW(TypeError, ostr.str());
    }
}


////////////////////////////////////////


template<typename ChildT>
inline Index64
RootNode<ChildT>::memUsage() const
{
    Index64 sum = sizeof(*this);
    for (MapCIter iter=mTable.begin(); iter!=mTable.end(); ++iter) {
        if (const ChildT *child = iter->second.child) {
            sum += child->memUsage();
        }
    }
    return sum;
}

template<typename ChildT>
inline void
RootNode<ChildT>::evalActiveVoxelBoundingBox(CoordBBox& bbox) const
{
    for (MapCIter iter=mTable.begin(); iter!=mTable.end(); ++iter) {
        if (const ChildT *child = iter->second.child) {
            child->evalActiveVoxelBoundingBox(bbox);
        } else if (isTileOn(iter)) {
            bbox.expand(iter->first, ChildT::DIM);
        }
    }
}


template<typename ChildT>
inline void
RootNode<ChildT>::clearTable()
{
    for (MapIter i = mTable.begin(), e = mTable.end(); i != e; ++i) {
        delete i->second.child;
    }
    mTable.clear();
}


template<typename ChildT>
inline Index
RootNode<ChildT>::getChildCount() const {
    Index sum = 0;
    for (MapCIter i = mTable.begin(), e = mTable.end(); i != e; ++i) {
        if (isChild(i)) ++sum;
    }
    return sum;
}


template<typename ChildT>
inline Index
RootNode<ChildT>::getTileCount() const
{
    Index sum = 0;
    for (MapCIter i = mTable.begin(), e = mTable.end(); i != e; ++i) {
        if (isTile(i)) ++sum;
    }
    return sum;
}


template<typename ChildT>
inline Index
RootNode<ChildT>::getActiveTileCount() const
{
    Index sum = 0;
    for (MapCIter i = mTable.begin(), e = mTable.end(); i != e; ++i) {
        if (isTileOn(i)) ++sum;
    }
    return sum;
}


template<typename ChildT>
inline Index
RootNode<ChildT>::getInactiveTileCount() const
{
    Index sum = 0;
    for (MapCIter i = mTable.begin(), e = mTable.end(); i != e; ++i) {
        if (isTileOff(i)) ++sum;
    }
    return sum;
}


template<typename ChildT>
inline Index32
RootNode<ChildT>::leafCount() const
{
    Index32 sum = 0;
    for (MapCIter i = mTable.begin(), e = mTable.end(); i != e; ++i) {
        if (isChild(i)) sum += getChild(i).leafCount();
    }
    return sum;
}


template<typename ChildT>
inline Index32
RootNode<ChildT>::nonLeafCount() const
{
    Index32 sum = 1;
    if (ChildT::LEVEL != 0) {
        for (MapCIter i = mTable.begin(), e = mTable.end(); i != e; ++i) {
            if (isChild(i)) sum += getChild(i).nonLeafCount();
        }
    }
    return sum;
}


template<typename ChildT>
inline Index64
RootNode<ChildT>::onVoxelCount() const
{
    Index64 sum = 0;
    for (MapCIter i = mTable.begin(), e = mTable.end(); i != e; ++i) {
        if (isChild(i)) {
            sum += getChild(i).onVoxelCount();
        } else if (isTileOn(i)) {
            sum += ChildT::NUM_VOXELS;
        }
    }
    return sum;
}


template<typename ChildT>
inline Index64
RootNode<ChildT>::offVoxelCount() const
{
    Index64 sum = 0;
    for (MapCIter i = mTable.begin(), e = mTable.end(); i != e; ++i) {
        if (isChild(i)) {
            sum += getChild(i).offVoxelCount();
        } else if (isTileOff(i) && !this->isBackgroundTile(i)) {
            sum += ChildT::NUM_VOXELS;
        }
    }
    return sum;
}


template<typename ChildT>
inline Index64
RootNode<ChildT>::onLeafVoxelCount() const
{
    Index64 sum = 0;
    for (MapCIter i = mTable.begin(), e = mTable.end(); i != e; ++i) {
        if (isChild(i)) sum += getChild(i).onLeafVoxelCount();
    }
    return sum;
}


template<typename ChildT>
inline Index64
RootNode<ChildT>::offLeafVoxelCount() const
{
    Index64 sum = 0;
    for (MapCIter i = mTable.begin(), e = mTable.end(); i != e; ++i) {
        if (isChild(i)) sum += getChild(i).offLeafVoxelCount();
    }
    return sum;
}


////////////////////////////////////////


template<typename ChildT>
inline bool
RootNode<ChildT>::isValueOn(const Coord& xyz) const
{
    MapCIter iter = this->findCoord(xyz);
    if (iter == mTable.end() || isTileOff(iter)) return false;
    return isTileOn(iter) ? true : getChild(iter).isValueOn(xyz);
}

template<typename ChildT>
inline bool
RootNode<ChildT>::hasActiveTiles() const
{
    for (MapCIter i = mTable.begin(), e = mTable.end(); i != e; ++i) {
        if (isChild(i) ? getChild(i).hasActiveTiles() : getTile(i).active) return true;
    }
    return false;
}

template<typename ChildT>
template<typename AccessorT>
inline bool
RootNode<ChildT>::isValueOnAndCache(const Coord& xyz, AccessorT& acc) const
{
    MapCIter iter = this->findCoord(xyz);
    if (iter == mTable.end() || isTileOff(iter)) return false;
    if (isTileOn(iter)) return true;
    acc.insert(xyz, &getChild(iter));
    return getChild(iter).isValueOnAndCache(xyz, acc);
}


template<typename ChildT>
inline const typename ChildT::ValueType&
RootNode<ChildT>::getValue(const Coord& xyz) const
{
    MapCIter iter = this->findCoord(xyz);
    return iter == mTable.end() ? mBackground
        : (isTile(iter) ? getTile(iter).value : getChild(iter).getValue(xyz));
}

template<typename ChildT>
template<typename AccessorT>
inline const typename ChildT::ValueType&
RootNode<ChildT>::getValueAndCache(const Coord& xyz, AccessorT& acc) const
{
    MapCIter iter = this->findCoord(xyz);
    if (iter == mTable.end()) return mBackground;
    if (isChild(iter)) {
        acc.insert(xyz, &getChild(iter));
        return getChild(iter).getValueAndCache(xyz, acc);
    }
    return getTile(iter).value;
}


template<typename ChildT>
inline int
RootNode<ChildT>::getValueDepth(const Coord& xyz) const
{
    MapCIter iter = this->findCoord(xyz);
    return iter == mTable.end() ? -1
        : (isTile(iter) ? 0 : int(LEVEL) - int(getChild(iter).getValueLevel(xyz)));
}

template<typename ChildT>
template<typename AccessorT>
inline int
RootNode<ChildT>::getValueDepthAndCache(const Coord& xyz, AccessorT& acc) const
{
    MapCIter iter = this->findCoord(xyz);
    if (iter == mTable.end()) return -1;
    if (isTile(iter)) return 0;
    acc.insert(xyz, &getChild(iter));
    return int(LEVEL) - int(getChild(iter).getValueLevelAndCache(xyz, acc));
}


template<typename ChildT>
inline void
RootNode<ChildT>::setValueOff(const Coord& xyz)
{
    MapIter iter = this->findCoord(xyz);
    if (iter != mTable.end() && !isTileOff(iter)) {
        if (isTileOn(iter)) {
            setChild(iter, *new ChildT(xyz, getTile(iter).value, /*active=*/true));
        }
        getChild(iter).setValueOff(xyz);
    }
}


template<typename ChildT>
inline void
RootNode<ChildT>::setActiveState(const Coord& xyz, bool on)
{
    ChildT* child = NULL;
    MapIter iter = this->findCoord(xyz);
    if (iter == mTable.end()) {
        if (on) {
            child = new ChildT(xyz, mBackground);
            mTable[this->coordToKey(xyz)] = NodeStruct(*child);
        } else {
            // Nothing to do; (x, y, z) is background and therefore already inactive.
        }
    } else if (isChild(iter)) {
        child = &getChild(iter);
    } else if (on != getTile(iter).active) {
        child = new ChildT(xyz, getTile(iter).value, !on);
        setChild(iter, *child);
    }
    if (child) child->setActiveState(xyz, on);
}

template<typename ChildT>
template<typename AccessorT>
inline void
RootNode<ChildT>::setActiveStateAndCache(const Coord& xyz, bool on, AccessorT& acc)
{
    ChildT* child = NULL;
    MapIter iter = this->findCoord(xyz);
    if (iter == mTable.end()) {
        if (on) {
            child = new ChildT(xyz, mBackground);
            mTable[this->coordToKey(xyz)] = NodeStruct(*child);
        } else {
            // Nothing to do; (x, y, z) is background and therefore already inactive.
        }
    } else if (isChild(iter)) {
        child = &getChild(iter);
    } else if (on != getTile(iter).active) {
        child = new ChildT(xyz, getTile(iter).value, !on);
        setChild(iter, *child);
    }
    if (child) {
        acc.insert(xyz, child);
        child->setActiveStateAndCache(xyz, on, acc);
    }
}


template<typename ChildT>
inline void
RootNode<ChildT>::setValueOff(const Coord& xyz, const ValueType& value)
{
    ChildT* child = NULL;
    MapIter iter = this->findCoord(xyz);
    if (iter == mTable.end()) {
        if (!math::isExactlyEqual(mBackground, value)) {
            child = new ChildT(xyz, mBackground);
            mTable[this->coordToKey(xyz)] = NodeStruct(*child);
        }
    } else if (isChild(iter)) {
        child = &getChild(iter);
    } else if (isTileOn(iter) || !math::isExactlyEqual(getTile(iter).value, value)) {
        child = new ChildT(xyz, getTile(iter).value, isTileOn(iter));
        setChild(iter, *child);
    }
    if (child) child->setValueOff(xyz, value);
}

template<typename ChildT>
template<typename AccessorT>
inline void
RootNode<ChildT>::setValueOffAndCache(const Coord& xyz, const ValueType& value, AccessorT& acc)
{
    ChildT* child = NULL;
    MapIter iter = this->findCoord(xyz);
    if (iter == mTable.end()) {
        if (!math::isExactlyEqual(mBackground, value)) {
            child = new ChildT(xyz, mBackground);
            mTable[this->coordToKey(xyz)] = NodeStruct(*child);
        }
    } else if (isChild(iter)) {
        child = &getChild(iter);
    } else if (isTileOn(iter) || !math::isExactlyEqual(getTile(iter).value, value)) {
        child = new ChildT(xyz, getTile(iter).value, isTileOn(iter));
        setChild(iter, *child);
    }
    if (child) {
        acc.insert(xyz, child);
        child->setValueOffAndCache(xyz, value, acc);
    }
}


template<typename ChildT>
inline void
RootNode<ChildT>::setValueOn(const Coord& xyz, const ValueType& value)
{
    ChildT* child = NULL;
    MapIter iter = this->findCoord(xyz);
    if (iter == mTable.end()) {
        child = new ChildT(xyz, mBackground);
        mTable[this->coordToKey(xyz)] = NodeStruct(*child);
    } else if (isChild(iter)) {
        child = &getChild(iter);
    } else if (isTileOff(iter) || !math::isExactlyEqual(getTile(iter).value, value)) {
        child = new ChildT(xyz, getTile(iter).value, isTileOn(iter));
        setChild(iter, *child);
    }
    if (child) child->setValueOn(xyz, value);
}

template<typename ChildT>
template<typename AccessorT>
inline void
RootNode<ChildT>::setValueAndCache(const Coord& xyz, const ValueType& value, AccessorT& acc)
{
    ChildT* child = NULL;
    MapIter iter = this->findCoord(xyz);
    if (iter == mTable.end()) {
        child = new ChildT(xyz, mBackground);
        mTable[this->coordToKey(xyz)] = NodeStruct(*child);
    } else if (isChild(iter)) {
        child = &getChild(iter);
    } else if (isTileOff(iter) || !math::isExactlyEqual(getTile(iter).value, value)) {
        child = new ChildT(xyz, getTile(iter).value, isTileOn(iter));
        setChild(iter, *child);
    }
    if (child) {
        acc.insert(xyz, child);
        child->setValueAndCache(xyz, value, acc);
    }
}


template<typename ChildT>
inline void
RootNode<ChildT>::setValueOnly(const Coord& xyz, const ValueType& value)
{
    ChildT* child = NULL;
    MapIter iter = this->findCoord(xyz);
    if (iter == mTable.end()) {
        child = new ChildT(xyz, mBackground);
        mTable[this->coordToKey(xyz)] = NodeStruct(*child);
    } else if (isChild(iter)) {
        child = &getChild(iter);
    } else if (!math::isExactlyEqual(getTile(iter).value, value)) {
        child = new ChildT(xyz, getTile(iter).value, isTileOn(iter));
        setChild(iter, *child);
    }
    if (child) child->setValueOnly(xyz, value);
}

template<typename ChildT>
template<typename AccessorT>
inline void
RootNode<ChildT>::setValueOnlyAndCache(const Coord& xyz, const ValueType& value, AccessorT& acc)
{
    ChildT* child = NULL;
    MapIter iter = this->findCoord(xyz);
    if (iter == mTable.end()) {
        child = new ChildT(xyz, mBackground);
        mTable[this->coordToKey(xyz)] = NodeStruct(*child);
    } else if (isChild(iter)) {
        child = &getChild(iter);
    } else if (!math::isExactlyEqual(getTile(iter).value, value)) {
        child = new ChildT(xyz, getTile(iter).value, isTileOn(iter));
        setChild(iter, *child);
    }
    if (child) {
        acc.insert(xyz, child);
        child->setValueOnlyAndCache(xyz, value, acc);
    }
}


template<typename ChildT>
inline void
RootNode<ChildT>::setValueOnMin(const Coord& xyz, const ValueType& value)
{
    ChildT* child = NULL;
    MapIter iter = this->findCoord(xyz);
    if (iter == mTable.end()) {
        child = new ChildT(xyz, mBackground);
        mTable[this->coordToKey(xyz)] = NodeStruct(*child);
    } else if (isChild(iter)) {
        child = &getChild(iter);
    } else if (isTileOff(iter) || getTile(iter).value > value) {
        child = new ChildT(xyz, getTile(iter).value, isTileOn(iter));
        setChild(iter, *child);
    }
    if (child) child->setValueOnMin(xyz, value);
}


template<typename ChildT>
inline void
RootNode<ChildT>::setValueOnMax(const Coord& xyz, const ValueType& value)
{
    ChildT* child = NULL;
    MapIter iter = this->findCoord(xyz);
    if (iter == mTable.end()) {
        child = new ChildT(xyz, mBackground);
        mTable[this->coordToKey(xyz)] = NodeStruct(*child);
    } else if (isChild(iter)) {
        child = &getChild(iter);
    } else if (isTileOff(iter) || getTile(iter).value < value) {
        child = new ChildT(xyz, getTile(iter).value, isTileOn(iter));
        setChild(iter, *child);
    }
    if (child) child->setValueOnMax(xyz, value);
}


template<typename ChildT>
inline void
RootNode<ChildT>::setValueOnSum(const Coord& xyz, const ValueType& addend)
{
    ChildT* child = NULL;
    MapIter iter = this->findCoord(xyz);
    if (iter == mTable.end()) {
        child = new ChildT(xyz, mBackground);
        mTable[this->coordToKey(xyz)] = NodeStruct(*child);
    } else if (isChild(iter)) {
        child = &getChild(iter);
    } else if (isTileOff(iter) || !math::isZero(addend)) {
        child = new ChildT(xyz, getTile(iter).value, isTileOn(iter));
        setChild(iter, *child);
    }
    if (child) child->setValueOnSum(xyz, addend);
}

template<typename ChildT>
template<typename AccessorT>
inline void
RootNode<ChildT>::setValueOnSumAndCache(const Coord& xyz,
    const ValueType& addend, AccessorT& acc)
{
    ChildT* child = NULL;
    MapIter iter = this->findCoord(xyz);
    if (iter == mTable.end()) {
        child = new ChildT(xyz, mBackground);
        mTable[this->coordToKey(xyz)] = NodeStruct(*child);
    } else if (isChild(iter)) {
        child = &getChild(iter);
    } else if (isTileOff(iter) || !math::isZero(addend)) {
        child = new ChildT(xyz, getTile(iter).value, isTileOn(iter));
        setChild(iter, *child);
    }
    if (child) {
        acc.insert(xyz, child);
        child->setValueOnSumAndCache(xyz, addend, acc);
    }
}


template<typename ChildT>
inline bool
RootNode<ChildT>::probeValue(const Coord& xyz, ValueType& value) const
{
    MapCIter iter = this->findCoord(xyz);
    if (iter == mTable.end()) {
        value = mBackground;
        return false;
    } else if (isChild(iter)) {
        return getChild(iter).probeValue(xyz, value);
    }
    value = getTile(iter).value;
    return isTileOn(iter);
}

template<typename ChildT>
template<typename AccessorT>
inline bool
RootNode<ChildT>::probeValueAndCache(const Coord& xyz, ValueType& value, AccessorT& acc) const
{
    MapCIter iter = this->findCoord(xyz);
    if (iter == mTable.end()) {
        value = mBackground;
        return false;
    } else if (isChild(iter)) {
        acc.insert(xyz, &getChild(iter));
        return getChild(iter).probeValueAndCache(xyz, value, acc);
    }
    value = getTile(iter).value;
    return isTileOn(iter);
}


////////////////////////////////////////


template<typename ChildT>
inline void
RootNode<ChildT>::fill(const CoordBBox& bbox, const ValueType& value, bool active)
{
    if (bbox.empty()) return;

    Coord xyz, tileMax;
    for (int x = bbox.min().x(); x <= bbox.max().x(); x = tileMax.x() + 1) {
        xyz.setX(x);
        for (int y = bbox.min().y(); y <= bbox.max().y(); y = tileMax.y() + 1) {
            xyz.setY(y);
            for (int z = bbox.min().z(); z <= bbox.max().z(); z = tileMax.z() + 1) {
                xyz.setZ(z);

                // Get the bounds of the tile that contains voxel (x, y, z).
                Coord tileMin = coordToKey(xyz);
                tileMax = tileMin.offsetBy(ChildT::DIM - 1);

                if (xyz != tileMin || Coord::lessThan(bbox.max(), tileMax)) {
                    // If the box defined by (xyz, bbox.max()) doesn't completely enclose
                    // the tile to which xyz belongs, create a child node (or retrieve
                    // the existing one).
                    ChildT* child = NULL;
                    MapIter iter = this->findKey(tileMin);
                    if (iter == mTable.end()) {
                        // No child or tile exists.  Create a child and initialize it
                        // with the background value.
                        child = new ChildT(xyz, mBackground);
                        mTable[tileMin] = NodeStruct(*child);
                    } else if (isTile(iter)) {
                        // Replace the tile with a newly-created child that is initialized
                        // with the tile's value and active state.
                        const Tile& tile = getTile(iter);
                        child = new ChildT(xyz, tile.value, tile.active);
                        mTable[tileMin] = NodeStruct(*child);
                    } else if (isChild(iter)) {
                        child = &getChild(iter);
                    }
                    // Forward the fill request to the child.
                    if (child) {
                        child->fill(CoordBBox(xyz, Coord::minComponent(bbox.max(), tileMax)),
                            value, active);
                    }
                } else {
                    // If the box given by (xyz, bbox.max()) completely encloses
                    // the tile to which xyz belongs, create the tile (if it
                    // doesn't already exist) and give it the fill value.
                    MapIter iter = this->findOrAddCoord(tileMin);
                    setTile(iter, Tile(value, active));
                }
            }
        }
    }
}


////////////////////////////////////////


template<typename ChildT>
inline bool
RootNode<ChildT>::writeTopology(std::ostream& os, bool toHalf) const
{
    if (!toHalf) {
        os.write(reinterpret_cast<const char*>(&mBackground), sizeof(ValueType));
    } else {
        ValueType truncatedVal = io::truncateRealToHalf(mBackground);
        os.write(reinterpret_cast<const char*>(&truncatedVal), sizeof(ValueType));
    }
    io::setGridBackgroundValuePtr(os, &mBackground);

    const Index numTiles = this->getTileCount(), numChildren = this->getChildCount();
    os.write(reinterpret_cast<const char*>(&numTiles), sizeof(Index));
    os.write(reinterpret_cast<const char*>(&numChildren), sizeof(Index));

    if (numTiles == 0 && numChildren == 0) return false;

    // Write tiles.
    for (MapCIter i = mTable.begin(), e = mTable.end(); i != e; ++i) {
        if (isChild(i)) continue;
        os.write(reinterpret_cast<const char*>(i->first.asPointer()), 3 * sizeof(Int32));
        os.write(reinterpret_cast<const char*>(&getTile(i).value), sizeof(ValueType));
        os.write(reinterpret_cast<const char*>(&getTile(i).active), sizeof(bool));
    }
    // Write child nodes.
    for (MapCIter i = mTable.begin(), e = mTable.end(); i != e; ++i) {
        if (isTile(i)) continue;
        os.write(reinterpret_cast<const char*>(i->first.asPointer()), 3 * sizeof(Int32));
        getChild(i).writeTopology(os, toHalf);
    }

    return true; // not empty
}


template<typename ChildT>
inline bool
RootNode<ChildT>::readTopology(std::istream& is, bool fromHalf)
{
    // Delete the existing tree.
    this->clearTable();

    if (io::getFormatVersion(is) < OPENVDB_FILE_VERSION_ROOTNODE_MAP) {
        // Read and convert an older-format RootNode.

        // For backward compatibility with older file formats, read both
        // outside and inside background values.
        is.read(reinterpret_cast<char*>(&mBackground), sizeof(ValueType));
        ValueType inside;
        is.read(reinterpret_cast<char*>(&inside), sizeof(ValueType));

        io::setGridBackgroundValuePtr(is, &mBackground);

        // Read the index range.
        Coord rangeMin, rangeMax;
        is.read(reinterpret_cast<char*>(rangeMin.asPointer()), 3 * sizeof(Int32));
        is.read(reinterpret_cast<char*>(rangeMax.asPointer()), 3 * sizeof(Int32));

        this->initTable();
        Index tableSize = 0, log2Dim[4] = { 0, 0, 0, 0 };
        Int32 offset[3];
        for (int i = 0; i < 3; ++i) {
            offset[i] = rangeMin[i] >> ChildT::TOTAL;
            rangeMin[i] = offset[i] << ChildT::TOTAL;
            log2Dim[i] = 1 + util::FindHighestOn((rangeMax[i] >> ChildT::TOTAL) - offset[i]);
            tableSize += log2Dim[i];
            rangeMax[i] = (((1 << log2Dim[i]) + offset[i]) << ChildT::TOTAL) - 1;
        }
        log2Dim[3] = log2Dim[1] + log2Dim[2];
        tableSize = 1U << tableSize;

        // Read masks.
        util::RootNodeMask childMask(tableSize), valueMask(tableSize);
        childMask.load(is);
        valueMask.load(is);

        // Read child nodes/values.
        for (Index i = 0; i < tableSize; ++i) {
            // Compute origin = offset2coord(i).
            Index n = i;
            Coord origin;
            origin[0] = (n >> log2Dim[3]) + offset[0];
            n &= (1U << log2Dim[3]) - 1;
            origin[1] = (n >> log2Dim[2]) + offset[1];
            origin[2] = (n & ((1U << log2Dim[2]) - 1)) + offset[1];
            origin <<= ChildT::TOTAL;

            if (childMask.isOn(i)) {
                // Read in and insert a child node.
                ChildT* child = new ChildT(origin, mBackground);
                child->readTopology(is);
                mTable[origin] = NodeStruct(*child);
            } else {
                // Read in a tile value and insert a tile, but only if the value
                // is either active or non-background.
                ValueType value;
                is.read(reinterpret_cast<char*>(&value), sizeof(ValueType));
                if (valueMask.isOn(i) || (!math::isApproxEqual(value, mBackground))) {
                    mTable[origin] = NodeStruct(Tile(value, valueMask.isOn(i)));
                }
            }
        }
        return true;
    }

    // Read a RootNode that was stored in the current format.

    is.read(reinterpret_cast<char*>(&mBackground), sizeof(ValueType));
    io::setGridBackgroundValuePtr(is, &mBackground);

    Index numTiles = 0, numChildren = 0;
    is.read(reinterpret_cast<char*>(&numTiles), sizeof(Index));
    is.read(reinterpret_cast<char*>(&numChildren), sizeof(Index));

    if (numTiles == 0 && numChildren == 0) return false;

    Int32 vec[3];
    ValueType value;
    bool active;

    // Read tiles.
    for (Index n = 0; n < numTiles; ++n) {
        is.read(reinterpret_cast<char*>(vec), 3 * sizeof(Int32));
        is.read(reinterpret_cast<char*>(&value), sizeof(ValueType));
        is.read(reinterpret_cast<char*>(&active), sizeof(bool));
        mTable[Coord(vec)] = NodeStruct(Tile(value, active));
    }

    // Read child nodes.
    for (Index n = 0; n < numChildren; ++n) {
        is.read(reinterpret_cast<char*>(vec), 3 * sizeof(Int32));
        Coord origin(vec);
        ChildT* child = new ChildT(origin, mBackground);
        child->readTopology(is, fromHalf);
        mTable[Coord(vec)] = NodeStruct(*child);
    }

    return true; // not empty
}


template<typename ChildT>
inline void
RootNode<ChildT>::writeBuffers(std::ostream& os, bool toHalf) const
{
    for (MapCIter i = mTable.begin(), e = mTable.end(); i != e; ++i) {
        if (isChild(i)) getChild(i).writeBuffers(os, toHalf);
    }
}


template<typename ChildT>
inline void
RootNode<ChildT>::readBuffers(std::istream& is, bool fromHalf)
{
    for (MapIter i = mTable.begin(), e = mTable.end(); i != e; ++i) {
        if (isChild(i)) getChild(i).readBuffers(is, fromHalf);
    }
}


////////////////////////////////////////


template<typename ChildT>
template<typename PruneOp>
inline void
RootNode<ChildT>::pruneOp(PruneOp& op)
{
    for (MapIter i = mTable.begin(), e = mTable.end(); i != e; ++i) {
        if (this->isTile(i)|| !op(this->getChild(i))) continue;
        this->setTile(i, Tile(op.value, op.state));
    }
    this->eraseBackgroundTiles();
}


template<typename ChildT>
inline void
RootNode<ChildT>::prune(const ValueType& tolerance)
{
    TolerancePrune<ValueType> op(tolerance);
    this->pruneOp(op);
}


template<typename ChildT>
inline void
RootNode<ChildT>::pruneInactive(const ValueType& bg)
{
    InactivePrune<ValueType> op(bg);
    this->pruneOp(op);
}


template<typename ChildT>
inline void
RootNode<ChildT>::pruneInactive()
{
    this->pruneInactive(mBackground);
}


////////////////////////////////////////


template<typename ChildT>
inline typename ChildT::LeafNodeType*
RootNode<ChildT>::touchLeaf(const Coord& xyz)
{
    ChildT* child = NULL;
    MapIter iter = this->findCoord(xyz);
    if (iter == mTable.end()) {
        child = new ChildT(xyz, mBackground, false);
        mTable[this->coordToKey(xyz)] = NodeStruct(*child);
    } else if (isChild(iter)) {
        child = &getChild(iter);
    } else {
        child = new ChildT(xyz, getTile(iter).value, isTileOn(iter));
        setChild(iter, *child);
    }
    return child->touchLeaf(xyz);
}


template<typename ChildT>
template<typename AccessorT>
inline typename ChildT::LeafNodeType*
RootNode<ChildT>::touchLeafAndCache(const Coord& xyz, AccessorT& acc)
{
    ChildT* child = NULL;
    MapIter iter = this->findCoord(xyz);
    if (iter == mTable.end()) {
        child = new ChildT(xyz, mBackground, false);
        mTable[this->coordToKey(xyz)] = NodeStruct(*child);
    } else if (isChild(iter)) {
        child = &getChild(iter);
    } else {
        child = new ChildT(xyz, getTile(iter).value, isTileOn(iter));
        setChild(iter, *child);
    }
    acc.insert(xyz, child);
    return child->touchLeafAndCache(xyz, acc);
}


////////////////////////////////////////


template<typename ChildT>
inline typename ChildT::LeafNodeType*
RootNode<ChildT>::probeLeaf(const Coord& xyz)
{
    MapIter iter = this->findCoord(xyz);
    if (iter == mTable.end() || isTile(iter)) return NULL;
    return getChild(iter).probeLeaf(xyz);
}


template<typename ChildT>
inline const typename ChildT::LeafNodeType*
RootNode<ChildT>::probeConstLeaf(const Coord& xyz) const
{
    MapCIter iter = this->findCoord(xyz);
    if (iter == mTable.end() || isTile(iter)) return NULL;
    return getChild(iter).probeConstLeaf(xyz);
}


template<typename ChildT>
template<typename AccessorT>
inline typename ChildT::LeafNodeType*
RootNode<ChildT>::probeLeafAndCache(const Coord& xyz, AccessorT& acc)
{
    MapIter iter = this->findCoord(xyz);
    if (iter == mTable.end() || isTile(iter)) return NULL;
    ChildT* child = &getChild(iter);
    acc.insert(xyz, child);
    return child->probeLeafAndCache(xyz, acc);
}


template<typename ChildT>
template<typename AccessorT>
inline const typename ChildT::LeafNodeType*
RootNode<ChildT>::probeConstLeafAndCache(const Coord& xyz, AccessorT& acc) const
{
    MapCIter iter = this->findCoord(xyz);
    if (iter == mTable.end() || isTile(iter)) return NULL;
    const ChildT* child = &getChild(iter);
    acc.insert(xyz, child);
    return child->probeConstLeafAndCache(xyz, acc);
}


////////////////////////////////////////


template<typename ChildT>
inline void
RootNode<ChildT>::signedFloodFill()
{
    this->signedFloodFill(mBackground, negative(mBackground));
}


template<typename ChildT>
inline void
RootNode<ChildT>::signedFloodFill(const ValueType& outside, const ValueType& inside)
{
    const ValueType zero = zeroVal<ValueType>();

    mBackground = outside;

    // First, flood fill all child nodes and put child-keys into a sorted set
    CoordSet nodeKeys;
    for (MapIter i = mTable.begin(), e = mTable.end(); i != e; ++i) {
        if (this->isTile(i)) continue;
        getChild(i).signedFloodFill(outside, inside);
        nodeKeys.insert(i->first);//only add inactive tiles!
    }

    // We employ a simple z-scanline algorithm that inserts inactive
    // tiles with the inside value if they are sandwiched
    // between inside child nodes only!
    const Tile insideTile(inside, /*on=*/false);
    CoordSetCIter b = nodeKeys.begin(), e = nodeKeys.end();
    if ( b == e ) return;
    for (CoordSetCIter a = b++; b != e; ++a, ++b) {
        Coord d = *b - *a; // delta of neighboring keys
        if (d[0]!=0 || d[1]!=0 || d[2]==Int32(ChildT::DIM)) continue;//not z-scanline or neighbors
        MapIter i = mTable.find(*a), j = mTable.find(*b);
        const ValueType fill[] = { getChild(i).getLastValue(), getChild(j).getFirstValue() };
        if (!(fill[0] < zero) || !(fill[1] < zero)) continue; // scanline isn't inside
        for (Coord c = *a + Coord(0u,0u,ChildT::DIM); c[2] != (*b)[2]; c[2] += ChildT::DIM) {
            mTable[c] = insideTile;
        }
    }
}


////////////////////////////////////////


template<typename ChildT>
inline void
RootNode<ChildT>::voxelizeActiveTiles()
{
    for (MapIter i = mTable.begin(), e = mTable.end(); i != e; ++i) {
        if (this->isTileOff(i)) continue;
        ChildT* child = i->second.child;
        if (child==NULL) {
            child = new ChildT(i->first, this->getTile(i).value, true);
            i->second.child = child;
        }
        child->voxelizeActiveTiles();
    }
}


////////////////////////////////////////


template<typename ChildT>
inline void
RootNode<ChildT>::merge(RootNode& other)
{
    for (MapIter i = other.mTable.begin(), e = other.mTable.end(); i != e; ++i) {
        MapIter j = mTable.find(i->first);
        if (other.isChild(i)) {
            if (j == mTable.end()) { // insert other child node
                mTable[i->first]=NodeStruct(stealChild(i, Tile(other.mBackground, /*on=*/false)));
            } else if (isTile(j)) { // replace tile with other child node
                setChild(j, stealChild(i, Tile(other.mBackground, /*on=*/false)));
            } else { // merge both child nodes
                getChild(j).merge(getChild(i),other.mBackground, mBackground);
            }
        } else { // other is a tile
            if (j == mTable.end()) { // insert other tile
                mTable[i->first] = i->second;
            } else continue; // ignore other tile
        }
    }
    // Empty the other tree so as not to leave it in a partially cannibalized state.
    other.clear();
}


////////////////////////////////////////


template<typename ChildT>
template<typename OtherChildType>
inline void
RootNode<ChildT>::topologyUnion(const RootNode<OtherChildType>& other)
{
    typedef RootNode<OtherChildType> OtherRootT;
    typedef typename OtherRootT::MapCIter OtherCIterT;

    enforceSameConfiguration(other);

    for (OtherCIterT i = other.mTable.begin(), e = other.mTable.end(); i != e; ++i) {
        MapIter j = mTable.find(i->first);
        if (other.isChild(i)) {
            if (j == mTable.end()) { // create child branch with identical topology
                mTable[i->first] = NodeStruct(
                    *(new ChildT(other.getChild(i), mBackground, TopologyCopy())));
            } else if (this->isChild(j)) { // union with child branch
                this->getChild(j).topologyUnion(other.getChild(i));
            } else {// this is a tile so replace it with a child branch with identical topology
                ChildT* child = new ChildT(
                    other.getChild(i), this->getTile(j).value, TopologyCopy());
                if (this->isTileOn(j)) child->setValuesOn();//this is an active tile
                this->setChild(j, *child);
            }
        } else if (other.isTileOn(i)) { // other is an active tile
            if (j == mTable.end()) { // insert an active tile
                mTable[i->first] = NodeStruct(Tile(mBackground, true));
            } else if (this->isChild(j)) {
                this->getChild(j).setValuesOn();
            } else if (this->isTileOff(j)) {
                this->setTile(j, Tile(this->getTile(j).value, true));
            }
        }
    }
}


////////////////////////////////////////


template<typename ChildT>
template<typename CombineOp>
inline void
RootNode<ChildT>::combine(RootNode& other, CombineOp& op, bool prune)
{
    CombineArgs<ValueType> args;

    CoordSet keys;
    this->insertKeys(keys);
    other.insertKeys(keys);

    for (CoordSetCIter i = keys.begin(), e = keys.end(); i != e; ++i) {
        MapIter iter = findOrAddCoord(*i), otherIter = other.findOrAddCoord(*i);
        if (isTile(iter) && isTile(otherIter)) {
            // Both this node and the other node have constant values (tiles).
            // Combine the two values and store the result as this node's new tile value.
            op(args.setARef(getTile(iter).value)
                .setAIsActive(isTileOn(iter))
                .setBRef(getTile(otherIter).value)
                .setBIsActive(isTileOn(otherIter)));
            setTile(iter, Tile(args.result(), args.resultIsActive()));

        } else if (isChild(iter) && isTile(otherIter)) {
            // Combine this node's child with the other node's constant value.
            ChildT& child = getChild(iter);
            child.combine(getTile(otherIter).value, isTileOn(otherIter), op);

        } else if (isTile(iter) && isChild(otherIter)) {
            // Combine this node's constant value with the other node's child,
            // but use a new functor in which the A and B values are swapped,
            // since the constant value is the A value, not the B value.
            SwappedCombineOp<ValueType, CombineOp> swappedOp(op);
            ChildT& child = getChild(otherIter);
            child.combine(getTile(iter).value, isTileOn(iter), swappedOp);

            // Steal the other node's child.
            setChild(iter, stealChild(otherIter, Tile()));

        } else /*if (isChild(iter) && isChild(otherIter))*/ {
            // Combine this node's child with the other node's child.
            ChildT &child = getChild(iter), &otherChild = getChild(otherIter);
            child.combine(otherChild, op);
        }
        if (prune && isChild(iter)) getChild(iter).prune();
    }

    // Combine background values.
    op(args.setARef(mBackground).setBRef(other.mBackground));
    mBackground = args.result();

    // Empty the other tree so as not to leave it in a partially cannibalized state.
    other.clear();
}


////////////////////////////////////////


template<typename ChildT>
template<typename CombineOp>
inline void
RootNode<ChildT>::combine2(const RootNode& other0, const RootNode& other1,
    CombineOp& op, bool prune)
{
    CombineArgs<ValueType> args;

    CoordSet keys;
    other0.insertKeys(keys);
    other1.insertKeys(keys);

    const NodeStruct
        bg0(Tile(other0.mBackground, /*active=*/false)),
        bg1(Tile(other1.mBackground, /*active=*/false));

    for (CoordSetCIter i = keys.begin(), e = keys.end(); i != e; ++i) {
        MapIter thisIter = this->findOrAddCoord(*i);
        MapCIter iter0 = other0.findKey(*i), iter1 = other1.findKey(*i);
        const NodeStruct
            &ns0 = (iter0 != other0.mTable.end()) ? iter0->second : bg0,
            &ns1 = (iter1 != other1.mTable.end()) ? iter1->second : bg1;
        if (ns0.isTile() && ns1.isTile()) {
            // Both input nodes have constant values (tiles).
            // Combine the two values and add a new tile to this node with the result.
            op(args.setARef(ns0.tile.value)
                .setAIsActive(ns0.isTileOn())
                .setBRef(ns1.tile.value)
                .setBIsActive(ns1.isTileOn()));
            setTile(thisIter, Tile(args.result(), args.resultIsActive()));
        } else {
            ChildT& otherChild = ns0.isChild() ? *ns0.child : *ns1.child;
            if (!isChild(thisIter)) {
                // Add a new child with the same coordinates, etc. as the other node's child.
                setChild(thisIter,
                    *(new ChildT(otherChild.getOrigin(), getTile(thisIter).value)));
            }
            ChildT& child = getChild(thisIter);

            if (ns0.isTile()) {
                // Combine node1's child with node0's constant value
                // and write the result into this node's child.
                child.combine2(ns0.tile.value, *ns1.child, ns0.isTileOn(), op);
            } else if (ns1.isTile()) {
                // Combine node0's child with node1's constant value
                // and write the result into this node's child.
                child.combine2(*ns0.child, ns1.tile.value, ns1.isTileOn(), op);
            } else {
                // Combine node0's child with node1's child
                // and write the result into this node's child.
                child.combine2(*ns0.child, *ns1.child, op);
            }
        }
        if (prune && isChild(thisIter)) getChild(thisIter).prune();
    }

    // Combine background values.
    op(args.setARef(other0.mBackground).setBRef(other1.mBackground));
    mBackground = args.result();
}


////////////////////////////////////////


template<typename ChildT>
template<typename BBoxOp>
inline void
RootNode<ChildT>::visitActiveBBox(BBoxOp& op) const
{
    const bool descent = op.template descent<LEVEL>();
    for (MapCIter i = mTable.begin(), e = mTable.end(); i != e; ++i) {
        if (this->isTileOff(i)) continue;
        if (this->isChild(i) && descent) {
            this->getChild(i).visitActiveBBox(op);
        } else {
#ifdef _MSC_VER
            op.operator()<LEVEL>(CoordBBox::createCube(i->first, ChildT::DIM));
#else
            op.template operator()<LEVEL>(CoordBBox::createCube(i->first, ChildT::DIM));
#endif
        }
    }
}


template<typename ChildT>
template<typename VisitorOp>
inline void
RootNode<ChildT>::visit(VisitorOp& op)
{
    doVisit<RootNode, VisitorOp, ChildAllIter>(*this, op);
}


template<typename ChildT>
template<typename VisitorOp>
inline void
RootNode<ChildT>::visit(VisitorOp& op) const
{
    doVisit<const RootNode, VisitorOp, ChildAllCIter>(*this, op);
}


template<typename ChildT>
template<typename RootNodeT, typename VisitorOp, typename ChildAllIterT>
inline void
RootNode<ChildT>::doVisit(RootNodeT& self, VisitorOp& op)
{
    typename RootNodeT::ValueType val;
    for (ChildAllIterT iter = self.beginChildAll(); iter; ++iter) {
        if (op(iter)) continue;
        if (typename ChildAllIterT::ChildNodeType* child = iter.probeChild(val)) {
            child->visit(op);
        }
    }
}


////////////////////////////////////////


template<typename ChildT>
template<typename OtherRootNodeType, typename VisitorOp>
inline void
RootNode<ChildT>::visit2(OtherRootNodeType& other, VisitorOp& op)
{
    doVisit2<RootNode, OtherRootNodeType, VisitorOp, ChildAllIter,
        typename OtherRootNodeType::ChildAllIter>(*this, other, op);
}


template<typename ChildT>
template<typename OtherRootNodeType, typename VisitorOp>
inline void
RootNode<ChildT>::visit2(OtherRootNodeType& other, VisitorOp& op) const
{
    doVisit2<const RootNode, OtherRootNodeType, VisitorOp, ChildAllCIter,
        typename OtherRootNodeType::ChildAllCIter>(*this, other, op);
}


template<typename ChildT>
template<
    typename RootNodeT,
    typename OtherRootNodeT,
    typename VisitorOp,
    typename ChildAllIterT,
    typename OtherChildAllIterT>
inline void
RootNode<ChildT>::doVisit2(RootNodeT& self, OtherRootNodeT& other, VisitorOp& op)
{
    /// @todo Allow the two nodes to have different ValueTypes, but not
    /// different fan-out factors or different index space bounds.
    enforceSameConfiguration(other);

    typename RootNodeT::ValueType val;
    typename OtherRootNodeT::ValueType otherVal;

    // The two nodes are required to have corresponding table entries,
    // but since that might require background tiles to be added to one or both,
    // and the nodes might be const, we operate on shallow copies of the nodes instead.
    RootNodeT copyOfSelf(self.mBackground);
    copyOfSelf.mTable = self.mTable;
    OtherRootNodeT copyOfOther(other.mBackground);
    copyOfOther.mTable = other.mTable;

    // Add background tiles to both nodes as needed.
    CoordSet keys;
    self.insertKeys(keys);
    other.insertKeys(keys);
    for (CoordSetCIter i = keys.begin(), e = keys.end(); i != e; ++i) {
        copyOfSelf.findOrAddCoord(*i);
        copyOfOther.findOrAddCoord(*i);
    }

    ChildAllIterT iter = copyOfSelf.beginChildAll();
    OtherChildAllIterT otherIter = copyOfOther.beginChildAll();

    for ( ; iter && otherIter; ++iter, ++otherIter)
    {
        const size_t skipBranch = static_cast<size_t>(op(iter, otherIter));

        typename ChildAllIterT::ChildNodeType* child =
            (skipBranch & 1U) ? NULL : iter.probeChild(val);
        typename OtherChildAllIterT::ChildNodeType* otherChild =
            (skipBranch & 2U) ? NULL : otherIter.probeChild(otherVal);

        if (child != NULL && otherChild != NULL) {
            child->visit2Node(*otherChild, op);
        } else if (child != NULL) {
            child->visit2(otherIter, op);
        } else if (otherChild != NULL) {
            otherChild->visit2(iter, op, /*otherIsLHS=*/true);
        }
    }
    // Remove any background tiles that were added above,
    // as well as any that were created by the visitors.
    copyOfSelf.eraseBackgroundTiles();
    copyOfOther.eraseBackgroundTiles();

    // If either input node is non-const, replace its table with
    // the (possibly modified) copy.
    self.resetTable(copyOfSelf.mTable);
    other.resetTable(copyOfOther.mTable);
}

} // namespace tree
} // namespace OPENVDB_VERSION_NAME
} // namespace openvdb

#endif // OPENVDB_TREE_ROOTNODE_HAS_BEEN_INCLUDED

// Copyright (c) 2012-2013 DreamWorks Animation LLC
// All rights reserved. This software is distributed under the
// Mozilla Public License 2.0 ( http://www.mozilla.org/MPL/2.0/ )
