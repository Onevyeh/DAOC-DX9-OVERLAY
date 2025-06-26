// ShaderManager.cpp

#include <d3dx9.h>
#include "ShaderManager.h"
#include "VegetationShaderUtils.h"
#include "KnownVegetation.h"
#include <vector>
#include <string>
#include "niflib_includes.h" 
#include "injectemesh.h"
#undef byte
#include <cstdint>




LPD3DXEFFECT g_vegetationEffect = nullptr;
LPD3DXEFFECT g_leafEffect = nullptr;

D3DXHANDLE g_timeHandle = nullptr;
D3DXHANDLE g_wvpHandle = nullptr;
D3DXHANDLE g_baseTextureHandle = nullptr;

D3DXHANDLE g_leavesTimeHandle = nullptr;
D3DXHANDLE g_leavesWvpHandle = nullptr;
D3DXHANDLE g_leavesBaseTextureHandle = nullptr;

std::vector<GrassVertex> g_grassVertices;
std::vector<uint16_t>    g_grassIndices;

void InitGrassModels() {
    if (LoadGrassNif("Data Files\\Meshes\\Grass\\Grass01.nif", g_grassVertices, g_grassIndices)) {
        // Tu peux faire un log ici
        printf("Grass model loaded: %zu vertices, %zu indices\n", g_grassVertices.size(), g_grassIndices.size());
    }
    else {
        printf("Failed to load grass model!\n");
    }
}


void ShaderManager::Init()
{
    LoadKnownLeafTextures();
}

void LoadVegetationShader(IDirect3DDevice9* device)
{
    ID3DXBuffer* errorBuffer = nullptr;

    HRESULT hr = D3DXCreateEffectFromFileA(
        device,
        "C:\\D3D9Proxy\\shaders\\vegetation.fx",
        nullptr, nullptr, D3DXSHADER_DEBUG, nullptr,
        &g_vegetationEffect,
        &errorBuffer
    );

    if (FAILED(hr)) {
        if (errorBuffer) {
            OutputDebugStringA((char*)errorBuffer->GetBufferPointer());
            errorBuffer->Release();
        }
        else {
            OutputDebugStringA("[ERROR] Failed to load vegetation.fx (no errorBuffer)\n");
        }
        return;
    }

    g_timeHandle = g_vegetationEffect->GetParameterByName(nullptr, "time");
    g_wvpHandle = g_vegetationEffect->GetParameterByName(nullptr, "WorldViewProjection");
    g_baseTextureHandle = g_vegetationEffect->GetParameterByName(nullptr, "baseTexture");

    if (!g_timeHandle)
        OutputDebugStringA("[WARN] Shader handle 'time' not found\n");
    if (!g_wvpHandle)
        OutputDebugStringA("[WARN] Shader handle 'WorldViewProjection' not found\n");
    if (!g_baseTextureHandle)
        OutputDebugStringA("[WARN] Shader handle 'baseTexture' not found\n");
}

void LoadLeavesShader(IDirect3DDevice9* device)
{
    ID3DXBuffer* errorBuffer = nullptr;

    HRESULT hr = D3DXCreateEffectFromFileA(
        device,
        "C:\\D3D9Proxy\\shaders\\Leaves.fx",
        nullptr, nullptr, D3DXSHADER_DEBUG, nullptr,
        &g_leafEffect,
        &errorBuffer
    );

    if (FAILED(hr)) {
        if (errorBuffer) {
            OutputDebugStringA((char*)errorBuffer->GetBufferPointer());
            errorBuffer->Release();
        }
        else {
            OutputDebugStringA("[ERROR] Failed to load leaves.fx (no errorBuffer)\n");
        }
        return;
    }

    g_leavesTimeHandle = g_leafEffect->GetParameterByName(nullptr, "time");
    g_leavesWvpHandle = g_leafEffect->GetParameterByName(nullptr, "WorldViewProjection");
    g_leavesBaseTextureHandle = g_leafEffect->GetParameterByName(nullptr, "baseTexture");

    if (!g_leavesTimeHandle)
        OutputDebugStringA("[WARN] leaves.fx: 'time' handle missing\n");
    if (!g_leavesWvpHandle)
        OutputDebugStringA("[WARN] leaves.fx: 'WorldViewProjection' handle missing\n");
    if (!g_leavesBaseTextureHandle)
        OutputDebugStringA("[WARN] leaves.fx: 'baseTexture' handle missing\n");
}
