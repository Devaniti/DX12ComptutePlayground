#include "stdafx.h"

#include "D3D12Wrappers/HeapAllocator.h"
#include "D3D12Wrappers/DXGIFormatSizes.h"

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
ComPtr<ID3D12Resource>            g_Textures[2];
ComPtr<ID3D12Resource>            g_UploadBuffer;
ComPtr<ID3D12Resource>            g_ReadbackBuffer;
ComPtr<ID3D12GraphicsCommandList> g_GraphicsCommandList;
ComPtr<ID3D12DescriptorHeap>      g_SRVDescriptorHeap;
UINT64                            g_DescriptorHandleSizeIncrement[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
ComPtr<ID3D12CommandAllocator>    g_CommandAllocator;
ComPtr<ID3D12CommandList>         g_CommandList;
ComPtr<ID3D12Fence>               g_Fence;
ComPtr<ID3D12PipelineState>       g_PipelineState;
HANDLE g_FenceEvent;

HeapAllocator* g_DefaultHeapAllocator;
HeapAllocator* g_UploadHeapAllocator;
HeapAllocator* g_ReadbackHeapAllocator;

void ResetDevice()
{
    //ComPtr<ID3D12DebugDevice> debugDevice;
    //WIN_CALL(g_Device.As(&debugDevice));
    g_Device.Reset();
    //debugDevice->ReportLiveDeviceObjects(D3D12_RLDO_IGNORE_INTERNAL);
}

void DeleteHeapAllocators()
{
    delete g_DefaultHeapAllocator;
    delete g_UploadHeapAllocator;
    delete g_ReadbackHeapAllocator;
}

void Cleanup()
{
    DeleteHeapAllocators();
    g_DebugInterface.Reset();
    g_DXGIFactory.Reset();
    g_InfoQueue.Reset();
    g_CommandQueue.Reset();
    g_Textures[0].Reset();
    g_Textures[1].Reset();
    g_UploadBuffer.Reset();
    g_ReadbackBuffer.Reset();
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

void InitDeviceConstants()
{
    for (int i = 0 ; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES ; i++)
    {
        g_DescriptorHandleSizeIncrement[i] = g_Device->GetDescriptorHandleIncrementSize((D3D12_DESCRIPTOR_HEAP_TYPE)i);
    }
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
    D3D12_ROOT_PARAMETER1 descriptorTableParametersOld[2];
    for (size_t i = 0; i < 2; i++)
    {
        descriptorTableParametersOld[i].ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
        descriptorTableParametersOld[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        descriptorTableParametersOld[i].Descriptor.ShaderRegister = i;
        descriptorTableParametersOld[i].Descriptor.RegisterSpace = 0;
        descriptorTableParametersOld[i].Descriptor.Flags = i == 0 ? D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC : D3D12_ROOT_DESCRIPTOR_FLAG_DATA_VOLATILE;
    }
    D3D12_DESCRIPTOR_RANGE1 descriptorTableParameter;
    descriptorTableParameter.BaseShaderRegister = 0;
    descriptorTableParameter.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC;
    descriptorTableParameter.NumDescriptors = 2;
    descriptorTableParameter.OffsetInDescriptorsFromTableStart = 0;
    descriptorTableParameter.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    descriptorTableParameter.RegisterSpace = 0;

    D3D12_ROOT_PARAMETER1 rootParameters[1];
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
    rootParameters[0].DescriptorTable.pDescriptorRanges = &descriptorTableParameter;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSigDesc;
    rootSigDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
    rootSigDesc.Desc_1_1.NumParameters = 1;
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
    ComPtr<ID3DBlob> errors;
    if (FAILED(D3D12SerializeVersionedRootSignature(&rootSigDesc, serializedSignature.GetAddressOf(), &errors)))
    {
        OutputDebugStringA((char*)errors->GetBufferPointer());
        MyAssert(0);
    }
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
    desc.NodeMask = 0;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    WIN_CALL(g_Device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_SRVDescriptorHeap)));
}

void InitCommandAllocator()
{
    WIN_CALL(g_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_CommandAllocator)));
}

void InitHeapAllocators()
{
    g_DefaultHeapAllocator = new HeapAllocator(g_Device, D3D12_HEAP_TYPE_DEFAULT, g_HeapSize);
    g_UploadHeapAllocator = new HeapAllocator(g_Device, D3D12_HEAP_TYPE_UPLOAD, g_HeapSize, D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS);
    g_ReadbackHeapAllocator = new HeapAllocator(g_Device, D3D12_HEAP_TYPE_READBACK, g_HeapSize, D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS);
    
}

void InitTextures()
{
    D3D12_RESOURCE_DESC textureDesc;
    textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    textureDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    textureDesc.Width = g_TextureWidth;
    textureDesc.Height = g_TextureHeight;
    textureDesc.DepthOrArraySize = 1;
    textureDesc.MipLevels = 0;
    textureDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    g_Textures[0] = g_DefaultHeapAllocator->Allocate(textureDesc);
    g_Textures[1] = g_DefaultHeapAllocator->Allocate(textureDesc);

    D3D12_RESOURCE_DESC bufferDesc;
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    bufferDesc.Width = g_TextureWidth * g_TextureHeight * BitsPerPixel(DXGI_FORMAT_B8G8R8A8_UNORM) / 8;
    bufferDesc.Height = 1;
    bufferDesc.DepthOrArraySize = 1;
    bufferDesc.MipLevels = 1;
    bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
    bufferDesc.SampleDesc.Count = 1;
    bufferDesc.SampleDesc.Quality = 0;
    bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    g_UploadBuffer = g_UploadHeapAllocator->Allocate(bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ);
    g_ReadbackBuffer = g_ReadbackHeapAllocator->Allocate(bufferDesc, D3D12_RESOURCE_STATE_COPY_DEST);
}

D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandleAt(size_t i)
{
    MyAssert(i < g_ResourcesCount);
    D3D12_CPU_DESCRIPTOR_HANDLE  handle = g_SRVDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += UINT64(i) * g_DescriptorHandleSizeIncrement[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV];
    return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandleAt(size_t i)
{
    MyAssert(i < g_ResourcesCount);
    D3D12_GPU_DESCRIPTOR_HANDLE  handle = g_SRVDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
    handle.ptr += UINT64(i) * g_DescriptorHandleSizeIncrement[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV];
    return handle;
}

void InitTextureVies()
{
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavViewDesc;
    uavViewDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    uavViewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    uavViewDesc.Texture2D.MipSlice = 0;
    uavViewDesc.Texture2D.PlaneSlice = 0;

    g_Device->CreateUnorderedAccessView(g_Textures[0].Get(), nullptr, &uavViewDesc, GetCPUDescriptorHandleAt(0));
    g_Device->CreateUnorderedAccessView(g_Textures[1].Get(), nullptr, &uavViewDesc, GetCPUDescriptorHandleAt(1));
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
    MyAssert(g_FenceEvent);
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
    MyAssert(file);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    MyAssert(file);

    std::vector<char> buffer(size);
    file.read(buffer.data(), size);
    MyAssert(file);
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
    ID3D12DescriptorHeap* heaps[] = { g_SRVDescriptorHeap.Get() };
    g_GraphicsCommandList->SetDescriptorHeaps(1, heaps);
    g_GraphicsCommandList->SetPipelineState(g_PipelineState.Get());
    g_GraphicsCommandList->Dispatch(g_TextureWidth, g_TextureHeight, 1);
}

int main()
{
    EnableDebugLayer();
    InitFactory();
    InitAdapter();
    InitDevice();
    InitDeviceConstants();
    EnableDebugForDevice();
    InitRootSignature();
    InitCommandQueue();
    InitDescriptorHeap();
    InitCommandAllocator();
    InitHeapAllocators();
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
    Cleanup();

    return 0;
}