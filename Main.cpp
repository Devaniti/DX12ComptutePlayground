#include "stdafx.h"

const uint32_t g_BufferCount = 256;
const uint32_t g_BufferSize = 16;

ComPtr<ID3D12Debug> g_DebugInterface;
ComPtr<IDXGIFactory4> g_DXGIFactory;
ComPtr<IDXGIAdapter4> g_Adapter;
DXGI_ADAPTER_DESC1 g_AdapterDesc;
ComPtr<ID3D12Device2> g_Device;
ComPtr<ID3D12InfoQueue> g_InfoQueue;
ComPtr<ID3D12CommandQueue> g_CommandQueue;
ComPtr<ID3D12Resource> g_DeviceLocalBuffers[g_BufferCount];
ComPtr<ID3D12GraphicsCommandList> g_GraphicsCommandList;
ComPtr<ID3D12DescriptorHeap> g_SRVDescriptorHeap;
ComPtr<ID3D12CommandAllocator> g_CommandAllocator;
ComPtr<ID3D12CommandList> g_CommandList;

ComPtr<ID3D12Heap> g_ResourceHeap;
ComPtr<ID3D12Heap> g_ReadbackHeap;
ComPtr<ID3D12Heap> g_UploadHeap;

void EnableDebugLayer()
{
#if defined(_DEBUG)
	PRINT("Enabling Debug Layer");
	WIN_CALL(D3D12GetDebugInterface(IID_PPV_ARGS(&g_DebugInterface)));
	g_DebugInterface->EnableDebugLayer();
#endif
}

void InitFactory()
{
	UINT createFactoryFlags = 0;
#if defined(_DEBUG)
	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

	WIN_CALL(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&g_DXGIFactory)));
}

void InitAdapter()
{
	ComPtr<IDXGIAdapter1> adapter1;
	WIN_CALL(g_DXGIFactory->EnumAdapters1(0, &adapter1));
	WIN_CALL(adapter1.As(&g_Adapter));

	g_Adapter->GetDesc1(&g_AdapterDesc);

	DEBUG_PRINTW(g_AdapterDesc.Description);
}

void InitDevice()
{
	WIN_CALL(D3D12CreateDevice(g_Adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&g_Device)));
}

void EnableDebugForDevice()
{
	WIN_CALL(g_Device.As(&g_InfoQueue));
	WIN_CALL(g_InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE));
	WIN_CALL(g_InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE));
	WIN_CALL(g_InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE));
}

void InitCommandQueue()
{
	D3D12_COMMAND_QUEUE_DESC desc = {};
	desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	desc.NodeMask = 0;

	WIN_CALL(g_Device->CreateCommandQueue(&desc, IID_PPV_ARGS(&g_CommandQueue)));
}

void InitDescriptorHeap()
{
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = g_BufferCount;
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	WIN_CALL(g_Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_SRVDescriptorHeap)));
}

void InitCommandAllocator()
{
	WIN_CALL(g_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_CommandAllocator)));
}

void CreateCommandList()
{
	g_CommandList.Reset();
	WIN_CALL(g_Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_CommandAllocator.Get(), nullptr, IID_PPV_ARGS(&g_CommandList)));
	WIN_CALL(g_CommandList.As(&g_GraphicsCommandList));
}

void DeviceFlush()
{
}

void CreateResourceHeap(ComPtr<ID3D12Heap>& heap, D3D12_HEAP_TYPE type)
{
	D3D12_HEAP_DESC desc;
	desc.SizeInBytes = g_BufferSize * g_BufferCount;
	desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	desc.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
	desc.Properties.Type = type;
	desc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	desc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	desc.Properties.CreationNodeMask = 0;
	desc.Properties.VisibleNodeMask = 0;
		
	WIN_CALL(g_Device->CreateHeap(&desc, IID_PPV_ARGS(&heap)));
}

void CreateBuffer(ComPtr<ID3D12Resource>& buffer, ComPtr<ID3D12Heap> heap, uint32_t heapOffset)
{
	CD3DX12_RESOURCE_DESC;
	D3D12_RESOURCE_DESC resourceDesc;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Alignment;
	resourceDesc.Width;
	resourceDesc.Height;
	resourceDesc.DepthOrArraySize;
	resourceDesc.MipLevels;
	resourceDesc.Format;
	resourceDesc.SampleDesc;
	resourceDesc.Layout;
	resourceDesc.Flags;
}

int main()
{
	EnableDebugLayer();
	InitFactory();
	InitAdapter();
	InitDevice();
	EnableDebugForDevice();
	InitCommandQueue();
	InitDescriptorHeap();
	InitCommandAllocator();

	CreateResourceHeap(g_ResourceHeap, D3D12_HEAP_TYPE_DEFAULT);
	CreateResourceHeap(g_ReadbackHeap, D3D12_HEAP_TYPE_READBACK);
	CreateResourceHeap(g_UploadHeap, D3D12_HEAP_TYPE_UPLOAD);
	return 0;
}