#pragma once

template <class NodeType>
class SegmentTree
{
public:
    SegmentTree(size_t size);
    ~SegmentTree();
private:
    static constexpr ct_NodesPerChunk = max((CACHELINE_SIZE / sizeof(NodeType)), 2);
    struct Chunk
    {
        NodeType nodes[ct_NodesPerChunk];
    };

    Chunk* m_Data;
    size_t m_Size;
};

template <class NodeType>
SegmentTree<NodeType>::SegmentTree(size_t size)
    : m_Size(size)
{
    // TODO move to algo.h
    size_t nodesCurrentStep = ct_NodesPerChunk;
    size_t neededChunks = 1;
    while (nodesCurrentStep < size)
    {
        neededChunks += divRoundUp(size, nodesCurrentStep);
    }

    m_Data = new Chunk[neededChunks];
}

template <class NodeType>
SegmentTree<NodeType>::~SegmentTree()
{
    delete[] m_Data;
}
