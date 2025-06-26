// VegetationShaderUtils.cpp

#include "VegetationShaderUtils.h"
#include <d3dx9.h>
#include <fstream>
#include <iomanip>
#include <unordered_set>
#include "KnownVegetation.h"
#include "injectemesh.h"
#undef byte
#include <cstdint>


extern std::ofstream shaderDebugLog;

uint32_t g_lastTextureHash = 0;

VegetationType DetectVegetationType(uint32_t hash) {
    if (knownVegetationHashes.count(hash)) return VegetationType::Grass;
    if (knownLeafTextures.count(hash)) return VegetationType::Leaf;
    return VegetationType::None;
}

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
    UINT PrimCount)
{
    shaderDebugLog << "=====================\n[FRAME] Shader appliqué\n";

    float timeSeconds = static_cast<float>(GetTickCount64() % 100000) / 1000.0f;

    D3DXMATRIX world, view, proj, wvp;
    device->GetTransform(D3DTS_WORLD, &world);
    device->GetTransform(D3DTS_VIEW, &view);
    device->GetTransform(D3DTS_PROJECTION, &proj);
    wvp = world * view * proj;

    OutputDebugStringA("[DEBUG] ApplyVegetationEffect: début\n");

    effect->SetFloat(hTime, timeSeconds);
    OutputDebugStringA("[DEBUG] SetFloat OK\n");
    effect->SetMatrix(hWVP, &wvp);
    OutputDebugStringA("[DEBUG] SetMatrix OK\n");

    IDirect3DBaseTexture9* tex = nullptr;
    device->GetTexture(0, &tex);
    OutputDebugStringA("[DEBUG] GetTexture OK\n");
    if (tex && hBaseTexture) {
        effect->SetTexture(hBaseTexture, tex);
        OutputDebugStringA("[DEBUG] SetTexture OK\n");
        device->SetTexture(0, tex);
    }

    HRESULT result = device->DrawIndexedPrimitive(
        PrimitiveType, BaseVertexIndex, MinVertexIndex,
        NumVertices, StartIndex, PrimCount);

    if (SUCCEEDED(effect->Begin(nullptr, 0))) {
        if (SUCCEEDED(effect->BeginPass(0))) {
            shaderDebugLog << "[SHADER] BeginPass réussi\n";
            result = device->DrawIndexedPrimitive(
                PrimitiveType, BaseVertexIndex, MinVertexIndex,
                NumVertices, StartIndex, PrimCount);
            effect->EndPass();
        }
        effect->End();
    }

    if (tex) tex->Release();
    return result;
}

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
    UINT PrimCount)
{
    shaderDebugLog << "=====================\n[FRAME] Leaves Shader appliqué\n";

    float timeSeconds = static_cast<float>(GetTickCount64() % 100000) / 1000.0f;

    D3DXMATRIX world, view, proj, wvp;
    device->GetTransform(D3DTS_WORLD, &world);
    device->GetTransform(D3DTS_VIEW, &view);
    device->GetTransform(D3DTS_PROJECTION, &proj);
    wvp = world * view * proj;

    effect->SetFloat(hTime, timeSeconds);
    effect->SetMatrix(hWVP, &wvp);

    IDirect3DBaseTexture9* tex = nullptr;
    device->GetTexture(0, &tex);
    if (tex && hBaseTexture) {
        effect->SetTexture(hBaseTexture, tex);
        device->SetTexture(0, tex);
    }

    HRESULT result = device->DrawIndexedPrimitive(
        PrimitiveType, BaseVertexIndex, MinVertexIndex,
        NumVertices, StartIndex, PrimCount);

    if (SUCCEEDED(effect->Begin(nullptr, 0))) {
        if (SUCCEEDED(effect->BeginPass(0))) {
            shaderDebugLog << "[SHADER] BeginPass FEUILLES réussi\n";
            result = device->DrawIndexedPrimitive(
                PrimitiveType, BaseVertexIndex, MinVertexIndex,
                NumVertices, StartIndex, PrimCount);
            effect->EndPass();
        }
        effect->End();
    }

    if (tex) tex->Release();
    return result;
}

void LogShaderDebug(
    IDirect3DDevice9* device,
    LPD3DXEFFECT effect,
    D3DXHANDLE hTime,
    D3DXHANDLE hWVP,
    D3DXHANDLE hBaseTexture,
    const char* shaderTag)
{
    shaderDebugLog << "=====================\n" << shaderTag << " Shader appliqué\n";

    float timeSeconds = static_cast<float>(GetTickCount64() % 100000) / 1000.0f;
    D3DXMATRIX world, view, proj, wvp;
    device->GetTransform(D3DTS_WORLD, &world);
    device->GetTransform(D3DTS_VIEW, &view);
    device->GetTransform(D3DTS_PROJECTION, &proj);
    wvp = world * view * proj;

    effect->SetFloat(hTime, timeSeconds);
    effect->SetMatrix(hWVP, &wvp);

    IDirect3DBaseTexture9* tex = nullptr;
    device->GetTexture(0, &tex);
    if (tex && hBaseTexture) {
        effect->SetTexture(hBaseTexture, tex);
        device->SetTexture(0, tex);
    }

    if (tex) tex->Release();

    shaderDebugLog << "[TIME] t = " << timeSeconds << " s\n";
    shaderDebugLog.flush();
}
