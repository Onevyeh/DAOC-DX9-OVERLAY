#pragma once
#include <d3dx9.h>
#include <d3d9.h>
#include <set>
#include <cstdint>
#include <fstream>
#include <string>
#include <sstream>
#include "KnownVegetation.h"




enum class VegetationType {
    None,
    Grass,
    Leaf
};

// Déclarations des fonctions comme mentionné précédemment…


VegetationType DetectVegetationType(uint32_t hash);

HRESULT ApplyVegetationEffect(
    IDirect3DDevice9* device,
    LPD3DXEFFECT effect,
    D3DXHANDLE hTime,
    D3DXHANDLE hWVP,
    D3DXHANDLE hBaseTexture,
    D3DPRIMITIVETYPE PrimitiveType,
    INT BaseVertexIndex,
    UINT MinVertexIndex,
    UINT NumVertices,
    UINT StartIndex,
    UINT PrimCount
);

HRESULT ApplyLeavesEffect(
    IDirect3DDevice9* device,
    LPD3DXEFFECT effect,
    D3DXHANDLE hTime,
    D3DXHANDLE hWVP,
    D3DXHANDLE hBaseTexture,
    D3DPRIMITIVETYPE PrimitiveType,
    INT BaseVertexIndex,
    UINT MinVertexIndex,
    UINT NumVertices,
    UINT StartIndex,
    UINT PrimCount
);

// Handles to shader effects
extern LPD3DXEFFECT g_vegetationEffect;
extern LPD3DXEFFECT g_leafEffect;
extern D3DXHANDLE g_timeHandle;
extern D3DXHANDLE g_wvpHandle;
extern D3DXHANDLE g_baseTextureHandle;

extern uint32_t g_lastTextureHash;  // <- juste une déclaration ici


inline bool IsGrassVegetation(D3DPRIMITIVETYPE PrimitiveType, UINT PrimCount, DWORD fvf, UINT stride)
{
    return (PrimitiveType == D3DPT_TRIANGLELIST &&
        knownVegetationHashes.find(g_lastTextureHash) != knownVegetationHashes.end() &&
        PrimCount >= 10);
}

inline bool IsLeafVegetation(D3DPRIMITIVETYPE PrimitiveType, UINT PrimCount, DWORD fvf, UINT stride)
{
    return (PrimitiveType == D3DPT_TRIANGLELIST &&
        knownLeafTextures.find(g_lastTextureHash) != knownLeafTextures.end() &&
        PrimCount >= 6 && stride >= 20);
}

inline VegetationType DetectVegetationType(D3DPRIMITIVETYPE PrimitiveType, UINT PrimCount, DWORD fvf, UINT stride)
{
    if (IsGrassVegetation(PrimitiveType, PrimCount, fvf, stride))
        return VegetationType::Grass;
    if (IsLeafVegetation(PrimitiveType, PrimCount, fvf, stride))
        return VegetationType::Leaf;
    return VegetationType::None;
}
