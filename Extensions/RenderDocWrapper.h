#pragma once

#include "stdafx.h"

void InitializeRenderDocExtension();
void StartRenderDocCapture(ComPtr<ID3D12Device> device);
void EndRenderDocCapture(ComPtr<ID3D12Device> device);