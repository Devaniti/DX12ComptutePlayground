#include "stdafx.h"

#include "D3D12Wrappers/HeapAllocator.h"

static const size_t g_TextureWidth = 1920;
static const size_t g_TextureHeight = 1080;
static const size_t g_HeapSize = 1024 * 1024 * 128;
static const size_t g_ResourcesCount = 2;

ComPtr<ID3D12Debug>               g_DebugInterface;
ComPtr<IDXGIFactory4>             g_DXGIFactory;
ComPtr<IDXGIAdapter4>             g_Adapter;
DXGI_ADAPTER_DESC1                g_AdapterDesc;
ComPtr<ID3D12Device2>             g_Device;
ComPtr<ID3D12InfoQueue>           g_InfoQueue;
ComPtr<ID3D12RootSignature>       g_RootSignature;
ComPtr<ID3D12CommandQueue>        g_CommandQueue;
ComPtr<ID3D12Resource>            g_TextureIn;
ComPtr<ID3D12Resource>            g_TextureOut;
ComPtr<ID3D12GraphicsCommandList> g_GraphicsCommandList;
ComPtr<ID3D12DescriptorHeap>      g_SRVDescriptorHeap;
ComPtr<ID3D12CommandAllocator>    g_CommandAllocator;
ComPtr<ID3D12CommandList>         g_CommandList;
ComPtr<ID3D12Fence>               g_Fence;
ComPtr<ID3D12PipelineState>       g_PipelineState;
HANDLE g_FenceEvent;

HeapAllocator* g_HeapAllocator;

void ResetDevice()
{
    //ComPtr<ID3D12DebugDevice> debugDevice;
    //WIN_CALL(g_Device.As(&debugDevice));
    g_Device.Reset();
    //debugDevice->ReportLiveDeviceObjects(D3D12_RLDO_IGNORE_INTERNAL);
}

void Cleanup()
{
    g_DebugInterface.Reset();
    g_DXGIFactory.Reset();
    g_InfoQueue.Reset();
    g_CommandQueue.Reset();
    g_TextureIn.Reset();
    g_TextureOut.Reset();
    g_GraphicsCommandList.Reset();
    g_SRVDescriptorHeap.Reset();
    g_CommandAllocator.Reset();
    g_CommandList.Reset();
    g_Fence.Reset();
    ResetDevice();
}

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
#ifndef USE_WARP_DEVICE
    WIN_CALL(g_DXGIFactory->EnumAdapters1(0, &adapter1));
#else
    WIN_CALL(g_DXGIFactory->EnumWarpAdapter(IID_PPV_ARGS(&adapter1)));
#endif
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
#if defined(_DEBUG)
    WIN_CALL(g_Device.As(&g_InfoQueue));
    WIN_CALL(g_InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE));
    WIN_CALL(g_InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE));
    WIN_CALL(g_InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE));
#endif
}

void InitRootSignature()
{
    D3D12_ROOT_PARAMETER1 rootParameters[2];
    for (size_t i = 0; i < 2; i++)
    {
        rootParameters[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
        rootParameters[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        rootParameters[i].Descriptor.ShaderRegister = i;
        rootParameters[i].Descriptor.RegisterSpace = 0;
        rootParameters[i].Descriptor.Flags =  i == 0 ? D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC : D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE;
    }

    D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSigDesc;
    rootSigDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
    rootSigDesc.Desc_1_1.NumParameters = 2;
    rootSigDesc.Desc_1_1.NumStaticSamplers = 0;
    rootSigDesc.Desc_1_1.pParameters = rootParameters;
    rootSigDesc.Desc_1_1.pStaticSamplers = nullptr;
    rootSigDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

    ComPtr<ID3DBlob> serializedSignature;
    WIN_CALL(D3D12SerializeVersionedRootSignature(&rootSigDesc, serializedSignature.GetAddressOf(), nullptr));
    g_Device->CreateRootSignature(0, serializedSignature->GetBufferPointer(), serializedSignature->GetBufferSize(), IID_PPV_ARGS(&g_RootSignature));
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
    desc.NumDescriptors = g_ResourcesCount;
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

    WIN_CALL(g_Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_SRVDescriptorHeap)));
}

void InitCommandAllocator()
{
    WIN_CALL(g_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_CommandAllocator)));
}

void InitHeapAllocator()
{
    g_HeapAllocator = new HeapAllocator(g_Device, D3D12_HEAP_TYPE_DEFAULT, g_HeapSize);
}

void InitTextures()
{
    D3D12_RESOURCE_DESC textureDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_B8G8R8A8_UNORM, g_TextureWidth, g_TextureHeight, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    g_TextureIn = g_HeapAllocator->Allocate(textureDesc);
    g_TextureOut = g_HeapAllocator->Allocate(textureDesc);
}

void InitTextureVies()
{
}

void DeleteHeapAllocator()
{
    delete g_HeapAllocator;
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

std::vector<char> ReadFile(std::string filename)
{
    std::ifstream file("TextureProcess.cso", std::ios::binary | std::ios::ate);
    assert(file);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    assert(file);

    std::vector<char> buffer(size);
    file.read(buffer.data(), size);
    assert(file);
    return buffer;
}

void CreatePSO()
{
    std::vector<char> csBytecode = ReadFile("TextureProcess.cso");
    D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc;
    psoDesc.pRootSignature = g_RootSignature.Get();
    psoDesc.CS.pShaderBytecode = csBytecode.data();
    psoDesc.CS.BytecodeLength = csBytecode.size();
    psoDesc.NodeMask = 0;
    psoDesc.CachedPSO.pCachedBlob = nullptr;
    psoDesc.CachedPSO.CachedBlobSizeInBytes = 0;
#ifndef USE_WARP_DEVICE
    psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
#else
    psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_TOOL_DEBUG;
#endif
    g_Device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&g_PipelineState));
}

void RunShader()
{
    g_GraphicsCommandList->SetComputeRootSignature(g_RootSignature.Get());
}

int main()
{
    EnableDebugLayer();
    InitFactory();
    InitAdapter();
    InitDevice();
    EnableDebugForDevice();
    InitRootSignature();
    InitCommandQueue();
    InitDescriptorHeap();
    InitCommandAllocator();
    InitHeapAllocator();
    InitTextures();
    InitTextureVies();
    CreatePSO();
    CreateCommandList();
    CreateFence();
    for (int i = 0; i < 10; i++)
    {
        RunShader();
        ExecuteCommandList();
        DeviceFlush();
        ResetCommandList();
    }
    DeleteHeapAllocator();
    Cleanup();

    return 0;
}