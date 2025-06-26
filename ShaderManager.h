// ShaderManager.h
#pragma once

#include <d3dx9.h>

// === Grass Shader ===
extern LPD3DXEFFECT g_vegetationEffect;
extern D3DXHANDLE g_timeHandle;
extern D3DXHANDLE g_wvpHandle;
extern D3DXHANDLE g_baseTextureHandle;

void LoadVegetationShader(IDirect3DDevice9* device);

// === Leaves Shader ===
extern LPD3DXEFFECT g_leafEffect;
extern D3DXHANDLE g_leavesTimeHandle;
extern D3DXHANDLE g_leavesWvpHandle;
extern D3DXHANDLE g_leavesBaseTextureHandle;

void LoadLeavesShader(IDirect3DDevice9* device);

// === Global Init ===
namespace ShaderManager
{
    void Init();
}
