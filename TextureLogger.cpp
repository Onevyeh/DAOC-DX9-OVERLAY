#include "TextureLogger.h"
#include <Windows.h>
#include <filesystem>
#include <unordered_set>
#include "injectemesh.h"


bool g_triggerNextFrameCapture = false;
// Déclarations des fichiers de log supplémentaires
std::ofstream logDrawIndexed;
std::ofstream logFVF;
std::ofstream logStream;
std::ofstream logVertexDecl;
std::ofstream logVertexBuffer;
std::ofstream logVertexShader;
std::ofstream logPixelShader;
std::ofstream g_frameLog;
std::ofstream shaderDebugLog;

int g_frameIndex = 0;

void InitExtendedLogs()
{
    CreateDirectoryA("C:\\D3D9Proxy", NULL);

    logDrawIndexed.open("C:\\D3D9Proxy\\log_drawindexed.txt", std::ios::out);
    logFVF.open("C:\\D3D9Proxy\\log_fvf.txt", std::ios::out);
    logStream.open("C:\\D3D9Proxy\\log_stream.txt", std::ios::out);
    logVertexDecl.open("C:\\D3D9Proxy\\log_vertexdecl.txt", std::ios::out);
    logVertexBuffer.open("C:\\D3D9Proxy\\log_vertexbuffer.txt", std::ios::out);
    logVertexShader.open("C:\\D3D9Proxy\\log_vertexshader.txt", std::ios::out);
    logPixelShader.open("C:\\D3D9Proxy\\log_pixelshader.txt", std::ios::out);
    g_frameLog.open("C:\\D3D9Proxy\\frame_log.txt", std::ios::out);
    shaderDebugLog.open("C:\\D3D9Proxy\\shader_debug.log", std::ios::out);

    if (!logDrawIndexed) OutputDebugStringA("[ERROR] Failed to open log_drawindexed.txt\n");
    if (!logFVF) OutputDebugStringA("[ERROR] Failed to open log_fvf.txt\n");
    if (!logStream) OutputDebugStringA("[ERROR] Failed to open log_stream.txt\n");
    if (!logVertexDecl) OutputDebugStringA("[ERROR] Failed to open log_vertexdecl.txt\n");
    if (!logVertexBuffer) OutputDebugStringA("[ERROR] Failed to open log_vertexbuffer.txt\n");
    if (!logVertexShader) OutputDebugStringA("[ERROR] Failed to open log_vertexshader.txt\n");
    if (!logPixelShader) OutputDebugStringA("[ERROR] Failed to open log_pixelshader.txt\n");
    if (!g_frameLog) OutputDebugStringA("Failed to open frame_log.txt\n");
   
    if (!shaderDebugLog) OutputDebugStringA("[ERROR] Failed to open shader_debug.log\n");

}
bool g_captureNextFrame = false;

std::unordered_set<uint32_t> knownVegetationHashes = {
    0x101ECAB5 //atlas alb
};

std::set<IDirect3DBaseTexture9*> g_loggedTextures;
std::ofstream g_textureLog;



void InitTextureLog()
{
    CreateDirectoryA("C:\\D3D9Proxy\\jpg", NULL);

    CreateDirectoryA("C:\\D3D9Proxy", NULL); // Crée le dossier si pas existant

    g_textureLog.open("C:\\D3D9Proxy\\textures.log", std::ios::out);
    if (g_textureLog.is_open())
    {
        g_textureLog << "=== Logging started ===" << std::endl;
        g_textureLog.flush();
    }
    else
    {
        MessageBoxA(NULL, "Erreur : impossible d'ouvrir C:\\D3D9Proxy\\textures.log", "Log Fail", MB_OK);
    }
}
#include <d3dx9.h>

void ExportTextureAsJPG(IDirect3DTexture9* tex, const std::string& filename)
{
    if (!tex) return;

    std::string fullPath = "C:\\D3D9Proxy\\jpg\\" + filename + ".jpg";

    // Sauvegarde la texture au format JPG
    D3DXSaveTextureToFileA(
        fullPath.c_str(),
        D3DXIFF_JPG,
        tex,
        nullptr
    );
}
// --- Implémentation des nouvelles variables globales ---
std::set<uint32_t> g_detectedVegetationHashes;
std::unordered_map<IDirect3DBaseTexture9*, uint32_t> g_textureHashes;

