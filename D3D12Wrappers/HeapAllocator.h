#pragma once
#include "stdafx.h"

class HeapAllocator
{
public:
    HeapAllocator(ComPtr<ID3D12Device> device, D3D12_HEAP_TYPE type, size_t size, D3D12_HEAP_FLAGS heapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES);

    ComPtr<ID3D12Resource> Allocate(D3D12_RESOURCE_DESC resourceDesc, D3D12_RESOURCE_STATES resourceState = D3D12_RESOURCE_STATE_COMMON);
    void Deallocate(ComPtr<ID3D12Resource>& resource);
    void Reset();
private:
    struct FreeListEntry
    {
        size_t offset;
        size_t size;
    };

    ComPtr<ID3D12Device> m_Device;
    ComPtr<ID3D12Heap> m_Heap;
    size_t m_Size;
    std::list<FreeListEntry> m_FreeList;
};
