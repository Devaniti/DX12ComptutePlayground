#include "stdafx.h"

#include "RenderDocWrapper.h"
#include "External/renderdoc_app.h"

#ifndef _DEBUG

void InitializeRenderDocExtension()
{
}

void StartRenderDocCapture()
{
}

void EndRenderDocCapture()
{
}
#else

RENDERDOC_API_1_1_2* g_API;

void InitializeRenderDocExtension()
{
    if (HMODULE mod = GetModuleHandleA("renderdoc.dll"))
    {
        pRENDERDOC_GetAPI RENDERDOC_GetAPI =
            (pRENDERDOC_GetAPI)GetProcAddress(mod, "RENDERDOC_GetAPI");
        CHECKED_CALL(RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_1_2, (void**)&g_API));
    }
    else
    {
        DEBUG_PRINT("Couldn't load renderdoc.dll");
    }
}

void StartRenderDocCapture(ComPtr<ID3D12Device> device)
{
    if (!g_API) return;
    g_API->StartFrameCapture(device.Get(), 0);
}

void EndRenderDocCapture(ComPtr<ID3D12Device> device)
{
    if (!g_API) return;
    CHECKED_CALL(g_API->EndFrameCapture(device.Get(), 0));
}
#endif