#pragma once

#include <set>
#include <unordered_map>
#include <fstream>
#include <d3d9.h>
#include <string>
#include <cstdint>

extern std::set<IDirect3DBaseTexture9*> g_loggedTextures;
extern std::ofstream g_textureLog;
extern bool g_captureNextFrame;
extern std::ofstream g_frameLog;
extern int g_frameIndex;

void InitTextureLog();
void ExportTextureAsJPG(IDirect3DTexture9* tex, const std::string& filename);

extern std::ofstream logDrawIndexed;
extern std::ofstream logFVF;
extern std::ofstream logStream;
extern std::ofstream logVertexDecl;
extern std::ofstream logVertexBuffer;
extern std::ofstream logVertexShader;
extern std::ofstream logPixelShader;
void InitExtendedLogs();

// --- Nouvelles variables globales ---
extern std::set<uint32_t> g_detectedVegetationHashes;
extern std::unordered_map<IDirect3DBaseTexture9*, uint32_t> g_textureHashes;
extern uint32_t g_lastTextureHash;
