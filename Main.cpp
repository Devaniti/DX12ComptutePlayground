#include "stdafx.h"

const uint32_t g_NumBuffers = 256;

ComPtr<ID3D12Debug> g_DebugInterface;
ComPtr<IDXGIFactory4> g_DXGIFactory;
ComPtr<IDXGIAdapter4> g_Adapter;
DXGI_ADAPTER_DESC1 g_AdapterDesc;
ComPtr<ID3D12Device2> g_Device;
ComPtr<ID3D12InfoQueue> g_InfoQueue;
ComPtr<ID3D12CommandQueue> g_CommandQueue;
ComPtr<ID3D12Resource> g_Buffers[g_NumBuffers];
ComPtr<ID3D12GraphicsCommandList> g_GraphicsCommandList;
ComPtr<ID3D12DescriptorHeap> g_SRVDescriptorHeap;
ComPtr<ID3D12CommandAllocator> g_CommandAllocator;
ComPtr<ID3D12CommandList> g_CommandList;
ComPtr<ID3D12Fence> g_Fence;
HANDLE g_FenceEvent;

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
	desc.NumDescriptors = g_NumBuffers;
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

void CreateFence()
{
    WIN_CALL(g_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_Fence)));
    g_FenceEvent = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
    assert(g_FenceEvent);
}

void SignalFence(uint64_t value)
{
    WIN_CALL(g_CommandQueue->Signal(g_Fence.Get(), value));
}

void WaitFence(uint64_t value)
{
    if (g_Fence->GetCompletedValue() >= value)
        return;
    WIN_CALL(g_Fence->SetEventOnCompletion(value, g_FenceEvent));
    ::WaitForSingleObject(g_FenceEvent, INFINITE);
}

void DeviceFlush()
{
    static uint64_t fenceValue = 0;
    fenceValue++;
    SignalFence(fenceValue);
    WaitFence(fenceValue);
}

void ExecuteCommandList()
{
    WIN_CALL(g_GraphicsCommandList->Close());
    ID3D12CommandList* const commandLists[] = { g_CommandList.Get() };
    g_CommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
}

void ResetCommandList()
{
    WIN_CALL(g_CommandAllocator->Reset());
    WIN_CALL(g_GraphicsCommandList->Reset(g_CommandAllocator.Get(), nullptr));
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
    CreateCommandList();
    CreateFence();
    for (int i = 0; i < 10; i++)
    {
        ExecuteCommandList();
        DeviceFlush();
        ResetCommandList();
    }
	return 0;
}