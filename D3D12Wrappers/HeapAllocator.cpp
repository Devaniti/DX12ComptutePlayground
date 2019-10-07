#include "stdafx.h"
#include "HeapAllocator.h"

HeapAllocator::HeapAllocator(ComPtr<ID3D12Device> device, D3D12_HEAP_TYPE type, size_t size) 
	: m_Device(device)
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
