// GrassBuffers.cpp
#include "GrassBuffers.h"
#include "injectemesh.h"
#undef byte
#include <cstdint>







bool UploadGrassToGPU(IDirect3DDevice9* device,
    const std::vector<GrassVertex>& vertices,
    const std::vector<uint16_t>& indices,
    IDirect3DVertexBuffer9** outVB,
    IDirect3DIndexBuffer9** outIB)
{
    HRESULT hr;

    size_t vertSize = sizeof(GrassVertex) * vertices.size();
    hr = device->CreateVertexBuffer(vertSize, 0, D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1, D3DPOOL_MANAGED, outVB, nullptr);
    if (FAILED(hr)) return false;

    void* vbPtr = nullptr;
    (*outVB)->Lock(0, vertSize, &vbPtr, 0);
    memcpy(vbPtr, vertices.data(), vertSize);
    (*outVB)->Unlock();

    size_t idxSize = sizeof(uint16_t) * indices.size();
    hr = device->CreateIndexBuffer(idxSize, 0, D3DFMT_INDEX16, D3DPOOL_MANAGED, outIB, nullptr);
    if (FAILED(hr)) return false;

    void* ibPtr = nullptr;
    (*outIB)->Lock(0, idxSize, &ibPtr, 0);
    memcpy(ibPtr, indices.data(), idxSize);
    (*outIB)->Unlock();

    return true;
}
