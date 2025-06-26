// GrassGlobals.h
#pragma once

#include <d3d9.h>
#include <cstdint>
#include <map>
#include <vector>
#include <string>

// Vertex struct D3D9 compatible
struct GrassVertex {
    float x, y, z;
    float nx, ny, nz;
    float u, v;
};

// Buffers : hash de texture -> (vertexbuffer, indexbuffer)
extern std::map<uint32_t, std::pair<IDirect3DVertexBuffer9*, IDirect3DIndexBuffer9*>> g_customGrassBuffers;
extern std::map<uint32_t, std::pair<size_t, size_t>> g_customGrassSizes;

// Fonctions utilitaires
bool LoadGrassNif(const std::string& nifFile, std::vector<GrassVertex>& outVertices, std::vector<uint16_t>& outIndices);
bool UploadGrassToGPU(IDirect3DDevice9* device, const std::vector<GrassVertex>& vertices, const std::vector<uint16_t>& indices, IDirect3DVertexBuffer9** outVB, IDirect3DIndexBuffer9** outIB);
void InitGrassModels(IDirect3DDevice9* device);
