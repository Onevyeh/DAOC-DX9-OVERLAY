// GrassBuffers.h
#pragma once
#include <vector>
#include <d3d9.h>
#include "injectemesh.cpp" // pour GrassVertex
#include "GrassGlobals.h"

bool UploadGrassToGPU(IDirect3DDevice9* device,
    const std::vector<GrassVertex>& vertices,
    const std::vector<uint16_t>& indices,
    IDirect3DVertexBuffer9** outVB,
    IDirect3DIndexBuffer9** outIB);
