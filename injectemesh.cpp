#include "GrassGlobals.h"
#include <niflib.h>
#include <obj/NiTriShape.h>
#include <obj/NiTriShapeData.h>
#include <string>
#include <vector>

std::map<uint32_t, std::pair<IDirect3DVertexBuffer9*, IDirect3DIndexBuffer9*>> g_customGrassBuffers;
std::map<uint32_t, std::pair<size_t, size_t>> g_customGrassSizes;

bool LoadGrassNif(const std::string& nifFile, std::vector<GrassVertex>& outVertices, std::vector<uint16_t>& outIndices) {
    using namespace Niflib;
    NiObjectRef root = ReadNifTree(nifFile);
    if (!root) return false;

    // Cherche le premier NiTriShape
    std::vector<NiObjectRef> objs;
    objs.push_back(root);
    while (!objs.empty()) {
        NiObjectRef obj = objs.back();
        objs.pop_back();

        NiTriShapeRef triShape = DynamicCast<NiTriShape>(obj);
        if (triShape) {
            NiTriShapeDataRef data = triShape->GetData();
            if (!data) continue;

            auto verts = data->GetVertices();
            auto norms = data->GetNormals();
            auto uvs = data->GetUVSet(0);

            size_t vertCount = verts.size();
            outVertices.resize(vertCount);
            for (size_t i = 0; i < vertCount; ++i) {
                outVertices[i].x = verts[i].x;
                outVertices[i].y = verts[i].y;
                outVertices[i].z = verts[i].z;
                if (norms.size() == vertCount) {
                    outVertices[i].nx = norms[i].x;
                    outVertices[i].ny = norms[i].y;
                    outVertices[i].nz = norms[i].z;
                }
                else {
                    outVertices[i].nx = outVertices[i].ny = outVertices[i].nz = 0.f;
                }
                if (uvs.size() == vertCount) {
                    outVertices[i].u = uvs[i].u;
                    outVertices[i].v = uvs[i].v;
                }
                else {
                    outVertices[i].u = outVertices[i].v = 0.f;
                }
            }

            // Indices
            auto tris = data->GetTriangles();
            outIndices.resize(tris.size() * 3);
            for (size_t i = 0; i < tris.size(); ++i) {
                outIndices[i * 3 + 0] = tris[i].v1;
                outIndices[i * 3 + 1] = tris[i].v2;
                outIndices[i * 3 + 2] = tris[i].v3;
            }
            return true;
        }

        // Sinon, explore les enfants (si c'est un AVObject)
        NiAVObjectRef avObj = DynamicCast<NiAVObject>(obj);
        if (avObj) {
            for (const auto& child : avObj->GetChildren())
                objs.push_back(child);
        }
    }
    return false;
}

bool UploadGrassToGPU(IDirect3DDevice9* device, const std::vector<GrassVertex>& vertices, const std::vector<uint16_t>& indices, IDirect3DVertexBuffer9** outVB, IDirect3DIndexBuffer9** outIB) {
    HRESULT hr;
    size_t vertSize = sizeof(GrassVertex) * vertices.size();
    hr = device->CreateVertexBuffer((UINT)vertSize, 0, D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1, D3DPOOL_MANAGED, outVB, nullptr);
    if (FAILED(hr)) return false;
    void* vbPtr = nullptr;
    (*outVB)->Lock(0, (UINT)vertSize, &vbPtr, 0);
    memcpy(vbPtr, vertices.data(), vertSize);
    (*outVB)->Unlock();

    size_t idxSize = sizeof(uint16_t) * indices.size();
    hr = device->CreateIndexBuffer((UINT)idxSize, 0, D3DFMT_INDEX16, D3DPOOL_MANAGED, outIB, nullptr);
    if (FAILED(hr)) return false;
    void* ibPtr = nullptr;
    (*outIB)->Lock(0, (UINT)idxSize, &ibPtr, 0);
    memcpy(ibPtr, indices.data(), idxSize);
    (*outIB)->Unlock();
    return true;
}

void InitGrassModels(IDirect3DDevice9* device) {
    // Mets ici tous tes .nif à charger
    std::string nifPath = "C:\\chemin\\herbe_hd.nif"; // A changer par ton mesh
    std::vector<GrassVertex> verts;
    std::vector<uint16_t> inds;
    if (LoadGrassNif(nifPath, verts, inds)) {
        IDirect3DVertexBuffer9* vb = nullptr;
        IDirect3DIndexBuffer9* ib = nullptr;
        if (UploadGrassToGPU(device, verts, inds, &vb, &ib)) {
            uint32_t id = 0x101ECAB5; // Met ici le hash de ta texture associée
            g_customGrassBuffers[id] = { vb, ib };
            g_customGrassSizes[id] = { verts.size(), inds.size() / 3 };
        }
    }
}
