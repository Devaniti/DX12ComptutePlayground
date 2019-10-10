#include "stdafx.h"
#include "HeapAllocator.h"

HeapAllocator::HeapAllocator(ComPtr<ID3D12Device> device, D3D12_HEAP_TYPE type, size_t size) 
	: m_Device(device)
    , m_FreeList({ {0, size} })
{
	D3D12_HEAP_DESC desc;
	desc.SizeInBytes = size;
	desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	desc.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
	desc.Properties.Type = type;
	desc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	desc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	desc.Properties.CreationNodeMask = 0;
	desc.Properties.VisibleNodeMask = 0;

	WIN_CALL(m_Device->CreateHeap(&desc, IID_PPV_ARGS(&m_Heap)));
}

ComPtr<ID3D12Resource> HeapAllocator::Allocate(D3D12_RESOURCE_DESC resourceDesc)
{
    D3D12_RESOURCE_ALLOCATION_INFO allocationInfo = m_Device->GetResourceAllocationInfo(0, 1, &resourceDesc);
    size_t size = allocationInfo.SizeInBytes;
    size_t offset = -1;
    auto it = m_FreeList.begin();
    for (; it != m_FreeList.end(); ++it)
    {
        if (it->size >= size)
        {
            offset = it->offset;
            if (it->size > size)
            {
                it->size -= size;
                it->offset += size;
            }
            else
            {
                m_FreeList.erase(it);
            }
            break;
        }
    }

    if (offset == size_t(-1))
    {
        assert(0);
        return nullptr;
    }

    ComPtr<ID3D12Resource> resource;
    WIN_CALL(m_Device->CreatePlacedResource(m_Heap.Get(), offset, &resourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&resource)));
    return resource;
}

void HeapAllocator::Deallocate(ComPtr<ID3D12Resource>& resource)
{
    resource.Reset();
    // TODO add freed memory to freelist
}
