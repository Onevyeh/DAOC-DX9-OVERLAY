#define NOMINMAX // pour �viter les conflits avec min/max d�finis dans Windows.h
#include <cstdint>      // uint32_t, uint8_t
#include <cstddef>      // size_t
#include <algorithm>    // std::min
#include "TextureHash.h"
#include "TextureLogger.h"
#include "IDirect3DDevice9Proxy.h"
#include "VegetationShaderUtils.h"
#include <d3dx9.h>
#include <filesystem>
#include "ShaderManager.h"
#include "KnownVegetation.h" // ou le bon header si les hashes sont stock�s ailleurs
#include <sstream>
#include <iomanip>
#include <fstream>
#include "injectemesh.h"

// IDirect3DDevice9Proxy_DrawIndexedPrimitive_refactor.cpp



extern std::ofstream logDrawIndexed;
extern std::ofstream g_frameLog;
extern std::set<uint32_t> g_detectedVegetationHashes;
extern bool g_captureNextFrame;

namespace {

	void LogPrimitiveCall(D3DPRIMITIVETYPE PrimitiveType, INT BaseVertexIndex, UINT MinVertexIndex,
		UINT NumVertices, UINT StartIndex, UINT PrimCount) {
		logDrawIndexed << "DrawIndexedPrimitive(Type=" << PrimitiveType
			<< ", BaseVertexIndex=" << BaseVertexIndex
			<< ", MinVertexIndex=" << MinVertexIndex
			<< ", NumVertices=" << NumVertices
			<< ", StartIndex=" << StartIndex
			<< ", PrimCount=" << PrimCount << ")\n";

		if (g_captureNextFrame) {
			g_frameLog << "DrawIndexedPrimitive(Type=" << PrimitiveType
				<< ", PrimCount=" << PrimCount << ")\n";
		}
	}

	bool IsVegetationPrimitive(D3DPRIMITIVETYPE PrimitiveType, UINT PrimCount) {
		return (PrimitiveType == D3DPT_TRIANGLELIST &&
			g_detectedVegetationHashes.find(g_lastTextureHash) != g_detectedVegetationHashes.end() &&
			PrimCount >= 10);
	}

} // end anon namespace


static std::ofstream logDrawIndexed("C:\\D3D9Proxy\\log_drawindexed.txt", std::ios::app);
static std::ofstream logFVF("C:\\D3D9Proxy\\log_fvf.txt", std::ios::app);
static std::ofstream logStream("C:\\D3D9Proxy\\log_stream.txt", std::ios::app);
static std::ofstream logVertexDecl("C:\\D3D9Proxy\\log_vertexdecl.txt", std::ios::app);
static std::ofstream logVertexBuffer("C:\\D3D9Proxy\\log_vertexbuffer.txt", std::ios::app);
static std::ofstream logVertexShader("C:\\D3D9Proxy\\log_vertexshader.txt", std::ios::app);
static std::ofstream logPixelShader("C:\\D3D9Proxy\\log_pixelshader.txt", std::ios::app);


// pour log vertex et injection
void DumpVertexBufferAndLog(
	IDirect3DDevice9* device,
	IDirect3DVertexBuffer9* vb,
	UINT stride,
	uint32_t texHash,
	const char* tag,
	std::ofstream& log)
{
	void* pVertices = nullptr;
	if (SUCCEEDED(vb->Lock(0, 0, &pVertices, D3DLOCK_READONLY))) {
		D3DVERTEXBUFFER_DESC desc;
		vb->GetDesc(&desc);

		// Fichier unique : tag + hash + timestamp
		char filename[256];
		sprintf_s(filename, "C:\\D3D9Proxy\\vb_%s_hash_%08X_%llu.bin", tag, texHash, GetTickCount64());
		FILE* f = nullptr;
		fopen_s(&f, filename, "wb");
		if (f) {
			fwrite(pVertices, 1, desc.Size, f);
			fclose(f);

			// Log dans le fichier principal
			log << "[DUMP VB] " << tag
				<< " | hash=0x" << std::hex << std::uppercase << texHash << std::dec
				<< " | stride=" << stride
				<< " | size=" << desc.Size
				<< " | file=" << filename << "\n";
			log.flush();
		}
		vb->Unlock();
	}
}



// Utilisation des structures s�curis�es pour �liminer les warnings
// Remplace les d�clarations directes de types D3DX sans initialisation par les versions s�curis�es suivantes :
#define D3DXVECTOR2_SAFE D3DXVECTOR2(0.0f, 0.0f)
#define D3DXVECTOR4_SAFE D3DXVECTOR4(0.0f, 0.0f, 0.0f, 0.0f)
#define D3DXCOLOR_SAFE D3DXCOLOR(0.0f, 0.0f, 0.0f, 0.0f)
#define D3DXPLANE_SAFE D3DXPLANE(0.0f, 0.0f, 0.0f, 0.0f)
#define D3DXQUATERNION_SAFE D3DXQUATERNION(0.0f, 0.0f, 0.0f, 0.0f)
#define D3DXFLOAT16_SAFE D3DXFLOAT16(0.0f)

// Initialisations globales s�res
D3DXVECTOR2 safeVec2 = D3DXVECTOR2(0.0f, 0.0f);
D3DXVECTOR4 safeVec4 = D3DXVECTOR4(0.0f, 0.0f, 0.0f, 0.0f);
D3DXCOLOR safeColor = D3DXCOLOR(0.0f, 0.0f, 0.0f, 0.0f);
D3DXPLANE safePlane = D3DXPLANE(0.0f, 0.0f, 0.0f, 0.0f);
D3DXQUATERNION safeQuat = D3DXQUATERNION(0.0f, 0.0f, 0.0f, 0.0f);
D3DXFLOAT16 safeFloat16 = D3DXFLOAT16(0.0f);


struct SimpleVertex
{
	float x, y, z;
	float u, v;
};

IDirect3DDevice9Proxy *IDirect3DDevice9Proxy::lastDevice = NULL;

void* IDirect3DDevice9Proxy::callbacks[D3D9_DEVICE_FUNC_COUNT] = {NULL};

extern std::set<uint32_t> knownVegetationHashes;

extern "C" UINT WINAPI D3D9DeviceFuncHook(UINT funcId, void* funcRef){
	//Check existence of device
	if (!IDirect3DDevice9Proxy::lastDevice)
		return D3D_DEVICE_PROXY_STATUS_NOTREADY;
	if(funcId>(D3D9_DEVICE_FUNC_COUNT-1))
		return D3D_DEVICE_PROXY_STATUS_WRONG_FUNC_ID;
	if(!funcRef)
		return D3D_DEVICE_PROXY_STATUS_WRONG_FUNC;
	//Mkay, set it up
	IDirect3DDevice9Proxy::callbacks[funcId] = funcRef;
	return 1;
}

extern "C" UINT WINAPI D3D9DeviceFuncUnHook(UINT funcId){
	//Check existence of device
	if (!IDirect3DDevice9Proxy::lastDevice)
		return D3D_DEVICE_PROXY_STATUS_NOTREADY;
	if(funcId>(D3D9_DEVICE_FUNC_COUNT-1))
		return D3D_DEVICE_PROXY_STATUS_WRONG_FUNC_ID;
	//Mkay, set it up
	IDirect3DDevice9Proxy::callbacks[funcId] = NULL;
	return 1;
}

IDirect3DDevice9Proxy::IDirect3DDevice9Proxy(IDirect3DDevice9* pOriginal){
	origIDirect3DDevice9 = pOriginal; // store the pointer to original object
	lastDevice = this;
}

IDirect3DDevice9Proxy::~IDirect3DDevice9Proxy(void){
	lastDevice = NULL;
}

HRESULT IDirect3DDevice9Proxy::QueryInterface(REFIID riid, void** ppvObj){
	// check if original dll can provide interface. then send *our* address
	*ppvObj = NULL;
	HRESULT hRes = origIDirect3DDevice9->QueryInterface(riid, ppvObj); 
	if (hRes == NOERROR)
		*ppvObj = this;
	return hRes;
}

ULONG IDirect3DDevice9Proxy::AddRef(void){
	return(origIDirect3DDevice9->AddRef());
}

ULONG IDirect3DDevice9Proxy::Release(void){
	// ATTENTION: This is a booby-trap ! Watch out !
	// If we create our own sprites, surfaces, etc. (thus increasing the ref counter
	// by external action), we need to delete that objects before calling the original
	// Release function	

	// release/delete own objects
	// .....

	// Calling original function now
	ULONG count = origIDirect3DDevice9->Release();
	// destructor will be called automatically
	if (count == 0){
		delete(this);
	}
	return (count);
}

HRESULT IDirect3DDevice9Proxy::TestCooperativeLevel(void){
	return(origIDirect3DDevice9->TestCooperativeLevel());
}

UINT IDirect3DDevice9Proxy::GetAvailableTextureMem(void){
	return(origIDirect3DDevice9->GetAvailableTextureMem());
}

HRESULT IDirect3DDevice9Proxy::EvictManagedResources(void){
	return(origIDirect3DDevice9->EvictManagedResources());
}

HRESULT IDirect3DDevice9Proxy::GetDirect3D(IDirect3D9** ppD3D9){
	return(origIDirect3DDevice9->GetDirect3D(ppD3D9));
}

HRESULT IDirect3DDevice9Proxy::GetDeviceCaps(D3DCAPS9* pCaps){
	return(origIDirect3DDevice9->GetDeviceCaps(pCaps));
}

HRESULT IDirect3DDevice9Proxy::GetDisplayMode(UINT iSwapChain, D3DDISPLAYMODE* pMode){
	return(origIDirect3DDevice9->GetDisplayMode(iSwapChain, pMode));
}

HRESULT IDirect3DDevice9Proxy::GetCreationParameters(D3DDEVICE_CREATION_PARAMETERS *pParameters){
	return(origIDirect3DDevice9->GetCreationParameters(pParameters));
}

HRESULT IDirect3DDevice9Proxy::SetCursorProperties(UINT XHotSpot, UINT YHotSpot, IDirect3DSurface9* pCursorBitmap){
	return(origIDirect3DDevice9->SetCursorProperties(XHotSpot, YHotSpot, pCursorBitmap));
}

void IDirect3DDevice9Proxy::SetCursorPosition(int X, int Y, DWORD Flags){
	return(origIDirect3DDevice9->SetCursorPosition(X, Y, Flags));
}

BOOL IDirect3DDevice9Proxy::ShowCursor(BOOL bShow){
	return(origIDirect3DDevice9->ShowCursor(bShow));
}

HRESULT IDirect3DDevice9Proxy::CreateAdditionalSwapChain(D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DSwapChain9** pSwapChain){
	return(origIDirect3DDevice9->CreateAdditionalSwapChain(pPresentationParameters, pSwapChain));
}

HRESULT IDirect3DDevice9Proxy::GetSwapChain(UINT iSwapChain, IDirect3DSwapChain9** pSwapChain){
	return(origIDirect3DDevice9->GetSwapChain(iSwapChain, pSwapChain));
}

UINT IDirect3DDevice9Proxy::GetNumberOfSwapChains(void){
	return(origIDirect3DDevice9->GetNumberOfSwapChains());
}

HRESULT IDirect3DDevice9Proxy::Reset(D3DPRESENT_PARAMETERS* pPresentationParameters){
	if (callbacks[PRERESET])
		((D3D9DevicePreResetFunc)callbacks[PRERESET])();
	HRESULT res = (origIDirect3DDevice9->Reset(pPresentationParameters));
	if (callbacks[POSTRESET])
		((D3D9DevicePostResetFunc)callbacks[POSTRESET])(this, res);
	return res;
}

HRESULT IDirect3DDevice9Proxy::Present(CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion){
	HRESULT res = (origIDirect3DDevice9->Present(pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion));
	if (callbacks[POSTPRESENT])
		((D3D9DevicePostPresentFunc)callbacks[POSTPRESENT])(this, res);
	return res;
}

HRESULT IDirect3DDevice9Proxy::GetBackBuffer(UINT iSwapChain, UINT iBackBuffer, D3DBACKBUFFER_TYPE Type, IDirect3DSurface9** ppBackBuffer){
	return(origIDirect3DDevice9->GetBackBuffer(iSwapChain, iBackBuffer, Type, ppBackBuffer));
}

HRESULT IDirect3DDevice9Proxy::GetRasterStatus(UINT iSwapChain, D3DRASTER_STATUS* pRasterStatus){
	return(origIDirect3DDevice9->GetRasterStatus(iSwapChain, pRasterStatus));
}

HRESULT IDirect3DDevice9Proxy::SetDialogBoxMode(BOOL bEnableDialogs){
	return(origIDirect3DDevice9->SetDialogBoxMode(bEnableDialogs));
}

void IDirect3DDevice9Proxy::SetGammaRamp(UINT iSwapChain, DWORD Flags, CONST D3DGAMMARAMP* pRamp){
	return(origIDirect3DDevice9->SetGammaRamp(iSwapChain, Flags, pRamp));
}

void IDirect3DDevice9Proxy::GetGammaRamp(UINT iSwapChain, D3DGAMMARAMP* pRamp){
	return(origIDirect3DDevice9->GetGammaRamp(iSwapChain, pRamp));
}

HRESULT IDirect3DDevice9Proxy::CreateTexture(UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture9** ppTexture, HANDLE* pSharedHandle){
	return(origIDirect3DDevice9->CreateTexture(Width, Height, Levels, Usage, Format, Pool, ppTexture, pSharedHandle));
}

HRESULT IDirect3DDevice9Proxy::CreateVolumeTexture(UINT Width, UINT Height, UINT Depth, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DVolumeTexture9** ppVolumeTexture, HANDLE* pSharedHandle){
	return(origIDirect3DDevice9->CreateVolumeTexture(Width, Height, Depth, Levels, Usage, Format, Pool, ppVolumeTexture, pSharedHandle));
}

HRESULT IDirect3DDevice9Proxy::CreateCubeTexture(UINT EdgeLength, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DCubeTexture9** ppCubeTexture, HANDLE* pSharedHandle){
	return(origIDirect3DDevice9->CreateCubeTexture(EdgeLength, Levels, Usage,Format, Pool, ppCubeTexture, pSharedHandle));
}

HRESULT IDirect3DDevice9Proxy::CreateVertexBuffer(UINT Length, DWORD Usage, DWORD FVF, D3DPOOL Pool, IDirect3DVertexBuffer9** ppVertexBuffer, HANDLE* pSharedHandle){
	return(origIDirect3DDevice9->CreateVertexBuffer(Length, Usage, FVF, Pool, ppVertexBuffer, pSharedHandle));
}

HRESULT IDirect3DDevice9Proxy::CreateIndexBuffer(UINT Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DIndexBuffer9** ppIndexBuffer, HANDLE* pSharedHandle){
	return(origIDirect3DDevice9->CreateIndexBuffer(Length, Usage, Format, Pool, ppIndexBuffer, pSharedHandle));
}

HRESULT IDirect3DDevice9Proxy::CreateRenderTarget(UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, BOOL Lockable, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle){
	return(origIDirect3DDevice9->CreateRenderTarget(Width,Height,Format,MultiSample,MultisampleQuality,Lockable,ppSurface,pSharedHandle));
}

HRESULT IDirect3DDevice9Proxy::CreateDepthStencilSurface(UINT Width,UINT Height,D3DFORMAT Format,D3DMULTISAMPLE_TYPE MultiSample,DWORD MultisampleQuality,BOOL Discard,IDirect3DSurface9** ppSurface,HANDLE* pSharedHandle){
	return(origIDirect3DDevice9->CreateDepthStencilSurface(Width,Height,Format,MultiSample,MultisampleQuality,Discard,ppSurface,pSharedHandle));
}

HRESULT IDirect3DDevice9Proxy::UpdateSurface(IDirect3DSurface9* pSourceSurface,CONST RECT* pSourceRect,IDirect3DSurface9* pDestinationSurface,CONST POINT* pDestPoint){
	return(origIDirect3DDevice9->UpdateSurface(pSourceSurface,pSourceRect,pDestinationSurface,pDestPoint));
}

HRESULT IDirect3DDevice9Proxy::UpdateTexture(IDirect3DBaseTexture9* pSourceTexture,IDirect3DBaseTexture9* pDestinationTexture){
	return(origIDirect3DDevice9->UpdateTexture(pSourceTexture,pDestinationTexture));
}

HRESULT IDirect3DDevice9Proxy::GetRenderTargetData(IDirect3DSurface9* pRenderTarget,IDirect3DSurface9* pDestSurface){
	return(origIDirect3DDevice9->GetRenderTargetData(pRenderTarget,pDestSurface));
}

HRESULT IDirect3DDevice9Proxy::GetFrontBufferData(UINT iSwapChain,IDirect3DSurface9* pDestSurface){
	return(origIDirect3DDevice9->GetFrontBufferData(iSwapChain,pDestSurface));
}

HRESULT IDirect3DDevice9Proxy::StretchRect(IDirect3DSurface9* pSourceSurface,CONST RECT* pSourceRect,IDirect3DSurface9* pDestSurface,CONST RECT* pDestRect,D3DTEXTUREFILTERTYPE Filter){
	return(origIDirect3DDevice9->StretchRect(pSourceSurface,pSourceRect,pDestSurface,pDestRect,Filter));
}

HRESULT IDirect3DDevice9Proxy::ColorFill(IDirect3DSurface9* pSurface,CONST RECT* pRect,D3DCOLOR color){
	return(origIDirect3DDevice9->ColorFill(pSurface,pRect,color));
}

HRESULT IDirect3DDevice9Proxy::CreateOffscreenPlainSurface(UINT Width,UINT Height,D3DFORMAT Format,D3DPOOL Pool,IDirect3DSurface9** ppSurface,HANDLE* pSharedHandle){
	return(origIDirect3DDevice9->CreateOffscreenPlainSurface(Width,Height,Format,Pool,ppSurface,pSharedHandle));
}

HRESULT IDirect3DDevice9Proxy::SetRenderTarget(DWORD RenderTargetIndex,IDirect3DSurface9* pRenderTarget){
	return(origIDirect3DDevice9->SetRenderTarget(RenderTargetIndex,pRenderTarget));
}

HRESULT IDirect3DDevice9Proxy::GetRenderTarget(DWORD RenderTargetIndex,IDirect3DSurface9** ppRenderTarget){
	return(origIDirect3DDevice9->GetRenderTarget(RenderTargetIndex,ppRenderTarget));
}

HRESULT IDirect3DDevice9Proxy::SetDepthStencilSurface(IDirect3DSurface9* pNewZStencil){
	return(origIDirect3DDevice9->SetDepthStencilSurface(pNewZStencil));
}

HRESULT IDirect3DDevice9Proxy::GetDepthStencilSurface(IDirect3DSurface9** ppZStencilSurface){
	return(origIDirect3DDevice9->GetDepthStencilSurface(ppZStencilSurface));
}

HRESULT IDirect3DDevice9Proxy::BeginScene()
{
	if (g_triggerNextFrameCapture)
	{
		g_triggerNextFrameCapture = true;
		g_captureNextFrame = false;
		OutputDebugStringA("[LOG] Capture d�clench�e automatiquement\n");
	}

	if (GetAsyncKeyState(VK_F12) & 1)  // d�clenchement unique
	{
		g_captureNextFrame = true;
		OutputDebugStringA("[LOG] Capture prochaine frame (F12)\n");
	}

	return origIDirect3DDevice9->BeginScene();
}
HRESULT IDirect3DDevice9Proxy::EndScene(void)
{
	if (g_captureNextFrame)
	{
		g_captureNextFrame = false;
		OutputDebugStringA("[LOG] Fin de capture de frame\n");
	}

	if (callbacks[ENDSCENE])
		((D3D9DeviceEndSceneFunc)callbacks[ENDSCENE])(this);

	// (Optionnel : update global shader time ici si utilis� ailleurs)
	if (g_vegetationEffect && g_timeHandle)
	{
		static float t = 0.0f;
		t += 0.01f;
		g_vegetationEffect->SetFloat(g_timeHandle, t);
	}
	g_frameLog << "\n[Frame " << g_frameIndex++ << "]\n";
	g_frameLog.flush();





	return origIDirect3DDevice9->EndScene();
}


HRESULT IDirect3DDevice9Proxy::Clear(DWORD Count,CONST D3DRECT* pRects,DWORD Flags,D3DCOLOR Color,float Z,DWORD Stencil){
	return(origIDirect3DDevice9->Clear(Count,pRects,Flags,Color,Z,Stencil));
}

HRESULT IDirect3DDevice9Proxy::SetTransform(D3DTRANSFORMSTATETYPE State,CONST D3DMATRIX* pMatrix){
	return(origIDirect3DDevice9->SetTransform(State,pMatrix));
}

HRESULT IDirect3DDevice9Proxy::GetTransform(D3DTRANSFORMSTATETYPE State,D3DMATRIX* pMatrix){
	return(origIDirect3DDevice9->GetTransform(State,pMatrix));
}

HRESULT IDirect3DDevice9Proxy::MultiplyTransform(D3DTRANSFORMSTATETYPE State,CONST D3DMATRIX* pMatrix){
	return(origIDirect3DDevice9->MultiplyTransform(State,pMatrix));
}

HRESULT IDirect3DDevice9Proxy::SetViewport(CONST D3DVIEWPORT9* pViewport){
	return(origIDirect3DDevice9->SetViewport(pViewport));
}

HRESULT IDirect3DDevice9Proxy::GetViewport(D3DVIEWPORT9* pViewport){
	return(origIDirect3DDevice9->GetViewport(pViewport));
}

HRESULT IDirect3DDevice9Proxy::SetMaterial(CONST D3DMATERIAL9* pMaterial){
	return(origIDirect3DDevice9->SetMaterial(pMaterial));
}

HRESULT IDirect3DDevice9Proxy::GetMaterial(D3DMATERIAL9* pMaterial){
	return(origIDirect3DDevice9->GetMaterial(pMaterial));
}

HRESULT IDirect3DDevice9Proxy::SetLight(DWORD Index,CONST D3DLIGHT9* pLight){
	return(origIDirect3DDevice9->SetLight(Index,pLight));
}

HRESULT IDirect3DDevice9Proxy::GetLight(DWORD Index,D3DLIGHT9* pLight){
	return(origIDirect3DDevice9->GetLight(Index,pLight));
}

HRESULT IDirect3DDevice9Proxy::LightEnable(DWORD Index,BOOL Enable){
	return(origIDirect3DDevice9->LightEnable(Index,Enable));
}

HRESULT IDirect3DDevice9Proxy::GetLightEnable(DWORD Index,BOOL* pEnable){
	return(origIDirect3DDevice9->GetLightEnable(Index, pEnable));
}

HRESULT IDirect3DDevice9Proxy::SetClipPlane(DWORD Index,CONST float* pPlane){
	return(origIDirect3DDevice9->SetClipPlane(Index, pPlane));
}

HRESULT IDirect3DDevice9Proxy::GetClipPlane(DWORD Index,float* pPlane){
	return(origIDirect3DDevice9->GetClipPlane(Index,pPlane));
}

HRESULT IDirect3DDevice9Proxy::SetRenderState(D3DRENDERSTATETYPE State,DWORD Value){
	return(origIDirect3DDevice9->SetRenderState(State, Value));
}

HRESULT IDirect3DDevice9Proxy::GetRenderState(D3DRENDERSTATETYPE State,DWORD* pValue){
	return(origIDirect3DDevice9->GetRenderState(State, pValue));
}

HRESULT IDirect3DDevice9Proxy::CreateStateBlock(D3DSTATEBLOCKTYPE Type,IDirect3DStateBlock9** ppSB){
	return(origIDirect3DDevice9->CreateStateBlock(Type,ppSB));
}

HRESULT IDirect3DDevice9Proxy::BeginStateBlock(void){
	return(origIDirect3DDevice9->BeginStateBlock());
}

HRESULT IDirect3DDevice9Proxy::EndStateBlock(IDirect3DStateBlock9** ppSB){
	return(origIDirect3DDevice9->EndStateBlock(ppSB));
}

HRESULT IDirect3DDevice9Proxy::SetClipStatus(CONST D3DCLIPSTATUS9* pClipStatus){
	return(origIDirect3DDevice9->SetClipStatus(pClipStatus));
}

HRESULT IDirect3DDevice9Proxy::GetClipStatus(D3DCLIPSTATUS9* pClipStatus){
	return(origIDirect3DDevice9->GetClipStatus( pClipStatus));
}

HRESULT IDirect3DDevice9Proxy::GetTexture(DWORD Stage,IDirect3DBaseTexture9** ppTexture){
	return(origIDirect3DDevice9->GetTexture(Stage,ppTexture));
}

HRESULT IDirect3DDevice9Proxy::SetTexture(DWORD Stage, IDirect3DBaseTexture9* pTexture)
{
	uint32_t texHash = 0;
	std::string typeStr = "Unknown";
	UINT width = 0, height = 0;
	D3DFORMAT format = D3DFMT_UNKNOWN;

	if (pTexture && pTexture->GetType() == D3DRTYPE_TEXTURE)
	{
		typeStr = "2D";
		IDirect3DTexture9* tex = static_cast<IDirect3DTexture9*>(pTexture);

		// Texture jamais vue
		bool isNew = g_loggedTextures.find(pTexture) == g_loggedTextures.end();
		if (isNew)
			g_loggedTextures.insert(pTexture);

		//Si hash d�j� connu, on le r�cup�re
		auto it = g_textureHashes.find(pTexture);
		if (it != g_textureHashes.end())
		{
			texHash = it->second;
		}
		else
		{
			// On calcule le hash une seule fois
			D3DSURFACE_DESC desc;
			if (SUCCEEDED(tex->GetLevelDesc(0, &desc)))
			{
				width = desc.Width;
				height = desc.Height;
				format = desc.Format;

				IDirect3DSurface9* surface = nullptr;
				if (SUCCEEDED(tex->GetSurfaceLevel(0, &surface)))
				{
					D3DLOCKED_RECT locked;
					if (SUCCEEDED(surface->LockRect(&locked, nullptr, D3DLOCK_READONLY)))
					{
						size_t sampleSize = std::min<size_t>(locked.Pitch * 16, locked.Pitch * desc.Height);
						texHash = HashTextureMemory(locked.pBits, sampleSize);
						g_textureHashes[pTexture] = texHash; // on l'enregistre
						surface->UnlockRect();
					}
					surface->Release();
				}
			}

			// Export JPG pour debug visuel
			char filename[256];
			sprintf_s(filename, "slot%d_Addr%p_%ux%u.jpg", Stage, pTexture, width, height);
			ExportTextureAsJPG(tex, filename);
		}

		// Mise � jour du hash courant (slot 0 seulement)
		if (Stage == 0)
		{
			g_lastTextureHash = texHash;

			// ------------------------------------------
			// D�BUT de l'ajout LOG/DUMP VB V�G�TATION/FEUILLE
			// ------------------------------------------
			auto DumpVertexBufferAndLog = [&](const char* type, const std::set<uint32_t>& refSet) {
				if (refSet.find(texHash) != refSet.end() && g_detectedVegetationHashes.find(texHash) == g_detectedVegetationHashes.end()) {
					g_detectedVegetationHashes.insert(texHash);

					g_captureNextFrame = true;
					g_triggerNextFrameCapture = false;

					g_frameLog << "[AUTO CAPTURE] Texture " << type << " d�tect�e, Hash=0x"
						<< std::hex << std::uppercase << texHash << std::dec << "\n";

					// Log FVF aussi
					DWORD fvf = 0;
					if (SUCCEEDED(origIDirect3DDevice9->GetFVF(&fvf))) {
						g_frameLog << "[AUTO CAPTURE] " << type << " Hash 0x" << std::hex << texHash
							<< " uses FVF 0x" << fvf << std::dec << "\n";
					}

					// ----------- Dump du VB -----------
					IDirect3DVertexBuffer9* vb = nullptr;
					UINT offset = 0, stride = 0;
					if (SUCCEEDED(origIDirect3DDevice9->GetStreamSource(0, &vb, &offset, &stride)) && vb) {
						void* pVertices = nullptr;
						if (SUCCEEDED(vb->Lock(0, 0, &pVertices, D3DLOCK_READONLY))) {
							D3DVERTEXBUFFER_DESC desc;
							vb->GetDesc(&desc);
							// Nom fichier unique
							char vbFilename[256];
							sprintf_s(vbFilename, "C:\\D3D9Proxy\\vb_%s_hash_%08X_%llu.bin", type, texHash, GetTickCount64());
							FILE* f = nullptr;
							fopen_s(&f, vbFilename, "wb");
							if (f) {
								fwrite(pVertices, 1, desc.Size, f);
								fclose(f);
								// LOG LIGNE DUMP
								g_frameLog << "[DUMP VB] " << type
									<< " | hash=0x" << std::hex << std::uppercase << texHash << std::dec
									<< " | stride=" << stride
									<< " | size=" << desc.Size
									<< " | file=" << vbFilename << "\n";
								g_frameLog.flush();
							}
							vb->Unlock();
						}
						vb->Release();
					}
					// -----------------------------------
				}
				};

			DumpVertexBufferAndLog("FEUILLE", knownLeafTextures);
			DumpVertexBufferAndLog("VEGETATION", knownVegetationHashes);
			// ------------------------------------------
			// FIN de l'ajout LOG/DUMP VB
			// ------------------------------------------
		}

		// Log si texture nouvelle
		if (isNew)
		{
			g_textureLog << "SetTexture(slot=" << Stage
				<< ") [" << typeStr << "] "
				<< "Addr=" << pTexture
				<< " Size=" << width << "x" << height
				<< " Format=" << (UINT)format
				<< " Hash=0x" << std::hex << std::uppercase << texHash << std::dec
				<< std::endl;
			g_textureLog.flush();
		}
	}

	// Log frame si capture active
	if (g_captureNextFrame)
	{
		g_frameLog << "SetTexture(slot=" << Stage << ") Hash=0x"
			<< std::hex << std::uppercase << texHash << std::dec << "\n";
		g_frameLog.flush();
	}

	return origIDirect3DDevice9->SetTexture(Stage, pTexture);
}








HRESULT IDirect3DDevice9Proxy::GetTextureStageState(DWORD Stage,D3DTEXTURESTAGESTATETYPE Type,DWORD* pValue)
{
	return(origIDirect3DDevice9->GetTextureStageState(Stage,Type, pValue));
}

HRESULT IDirect3DDevice9Proxy::SetTextureStageState(DWORD Stage,D3DTEXTURESTAGESTATETYPE Type,DWORD Value)
{
	return(origIDirect3DDevice9->SetTextureStageState(Stage,Type,Value));
}

HRESULT IDirect3DDevice9Proxy::GetSamplerState(DWORD Sampler,D3DSAMPLERSTATETYPE Type,DWORD* pValue)
{
	return(origIDirect3DDevice9->GetSamplerState(Sampler,Type, pValue));
}

HRESULT IDirect3DDevice9Proxy::SetSamplerState(DWORD Sampler,D3DSAMPLERSTATETYPE Type,DWORD Value)
{
	return(origIDirect3DDevice9->SetSamplerState(Sampler,Type,Value));
}

HRESULT IDirect3DDevice9Proxy::ValidateDevice(DWORD* pNumPasses)
{
	return(origIDirect3DDevice9->ValidateDevice( pNumPasses));
}

HRESULT IDirect3DDevice9Proxy::SetPaletteEntries(UINT PaletteNumber,CONST PALETTEENTRY* pEntries)
{
	return(origIDirect3DDevice9->SetPaletteEntries(PaletteNumber, pEntries));
}

HRESULT IDirect3DDevice9Proxy::GetPaletteEntries(UINT PaletteNumber,PALETTEENTRY* pEntries)
{
	return(origIDirect3DDevice9->GetPaletteEntries(PaletteNumber, pEntries));
}

HRESULT IDirect3DDevice9Proxy::SetCurrentTexturePalette(UINT PaletteNumber)
{
	return(origIDirect3DDevice9->SetCurrentTexturePalette(PaletteNumber));
}

HRESULT IDirect3DDevice9Proxy::GetCurrentTexturePalette(UINT *PaletteNumber)
{
	return(origIDirect3DDevice9->GetCurrentTexturePalette(PaletteNumber));
}

HRESULT IDirect3DDevice9Proxy::SetScissorRect(CONST RECT* pRect)
{
	return(origIDirect3DDevice9->SetScissorRect( pRect));
}

HRESULT IDirect3DDevice9Proxy::GetScissorRect( RECT* pRect)
{
	return(origIDirect3DDevice9->GetScissorRect( pRect));
}

HRESULT IDirect3DDevice9Proxy::SetSoftwareVertexProcessing(BOOL bSoftware)
{
	return(origIDirect3DDevice9->SetSoftwareVertexProcessing(bSoftware));
}

BOOL    IDirect3DDevice9Proxy::GetSoftwareVertexProcessing(void)
{
	return(origIDirect3DDevice9->GetSoftwareVertexProcessing());
}

HRESULT IDirect3DDevice9Proxy::SetNPatchMode(float nSegments)
{
	return(origIDirect3DDevice9->SetNPatchMode(nSegments));
}

float   IDirect3DDevice9Proxy::GetNPatchMode(void)
{
	return(origIDirect3DDevice9->GetNPatchMode());
}

HRESULT IDirect3DDevice9Proxy::DrawPrimitive(D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount)
{
	IDirect3DBaseTexture9* tex = nullptr;
	origIDirect3DDevice9->GetTexture(0, &tex);

	char buf[512];
	sprintf_s(buf, "[DrawPrimitive] Type=%d, StartVertex=%u, Count=%u, Tex0=%p\n",
		PrimitiveType, StartVertex, PrimitiveCount, tex);
	OutputDebugStringA(buf);

	if (tex) tex->Release();
	return origIDirect3DDevice9->DrawPrimitive(PrimitiveType, StartVertex, PrimitiveCount);
}




HRESULT IDirect3DDevice9Proxy::DrawIndexedPrimitive(
	D3DPRIMITIVETYPE PrimitiveType,
	INT BaseVertexIndex,
	UINT MinVertexIndex,
	UINT NumVertices,
	UINT StartIndex,
	UINT PrimCount)
{
	LogPrimitiveCall(PrimitiveType, BaseVertexIndex, MinVertexIndex, NumVertices, StartIndex, PrimCount);

	DWORD fvf = 0;
	origIDirect3DDevice9->GetFVF(&fvf);

	UINT stride = 0;
	IDirect3DVertexBuffer9* vb = nullptr;
	UINT offset = 0;
	if (SUCCEEDED(origIDirect3DDevice9->GetStreamSource(0, &vb, &offset, &stride))) {
		if (vb) vb->Release();
	}

	// Injection mesh HD
	if (g_customGrassBuffers.count(g_lastTextureHash)) {
		auto& buffers = g_customGrassBuffers[g_lastTextureHash];
		auto& sizes = g_customGrassSizes[g_lastTextureHash];
		origIDirect3DDevice9->SetStreamSource(0, buffers.first, 0, sizeof(GrassVertex));
		origIDirect3DDevice9->SetIndices(buffers.second);
		origIDirect3DDevice9->SetFVF(D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1);
		return origIDirect3DDevice9->DrawIndexedPrimitive(
			D3DPT_TRIANGLELIST,
			0, 0,
			static_cast<UINT>(sizes.first),
			0,
			static_cast<UINT>(sizes.second)
		);
	}

	VegetationType type = DetectVegetationType(g_lastTextureHash);
	switch (type)
	{
	case VegetationType::Grass:
		return ApplyVegetationEffect(
			origIDirect3DDevice9,
			g_vegetationEffect,
			g_timeHandle,
			g_wvpHandle,
			g_baseTextureHandle,
			PrimitiveType,
			BaseVertexIndex,
			MinVertexIndex,
			NumVertices,
			StartIndex,
			PrimCount);
	case VegetationType::Leaf:
		return ApplyLeavesEffect(
			origIDirect3DDevice9,
			g_leafEffect,
			g_timeHandle,
			g_wvpHandle,
			g_baseTextureHandle,
			PrimitiveType,
			BaseVertexIndex,
			MinVertexIndex,
			NumVertices,
			StartIndex,
			PrimCount);
	default:
		break;
	}
	return origIDirect3DDevice9->DrawIndexedPrimitive(
		PrimitiveType, BaseVertexIndex, MinVertexIndex,
		NumVertices, StartIndex, PrimCount);
}








HRESULT IDirect3DDevice9Proxy::DrawPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType,UINT PrimitiveCount,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride)
{
	return(origIDirect3DDevice9->DrawPrimitiveUP(PrimitiveType,PrimitiveCount,pVertexStreamZeroData,VertexStreamZeroStride));
}

HRESULT IDirect3DDevice9Proxy::DrawIndexedPrimitiveUP(D3DPRIMITIVETYPE PrimitiveType,UINT MinVertexIndex,UINT NumVertices,UINT PrimitiveCount,CONST void* pIndexData,D3DFORMAT IndexDataFormat,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride)
{
	return(origIDirect3DDevice9->DrawIndexedPrimitiveUP(PrimitiveType,MinVertexIndex,NumVertices,PrimitiveCount, pIndexData, IndexDataFormat, pVertexStreamZeroData,VertexStreamZeroStride));
}

HRESULT IDirect3DDevice9Proxy::ProcessVertices(UINT SrcStartIndex,UINT DestIndex,UINT VertexCount,IDirect3DVertexBuffer9* pDestBuffer,IDirect3DVertexDeclaration9* pVertexDecl,DWORD Flags)
{
	return(origIDirect3DDevice9->ProcessVertices( SrcStartIndex, DestIndex, VertexCount, pDestBuffer, pVertexDecl, Flags));
}

HRESULT IDirect3DDevice9Proxy::CreateVertexDeclaration(CONST D3DVERTEXELEMENT9* pVertexElements,IDirect3DVertexDeclaration9** ppDecl)
{
	return(origIDirect3DDevice9->CreateVertexDeclaration( pVertexElements,ppDecl));
}

HRESULT IDirect3DDevice9Proxy::SetVertexDeclaration(IDirect3DVertexDeclaration9* pDecl)
{
	logVertexDecl << "SetVertexDeclaration(Decl=" << pDecl << ")\n";

	return(origIDirect3DDevice9->SetVertexDeclaration(pDecl));
}

HRESULT IDirect3DDevice9Proxy::GetVertexDeclaration(IDirect3DVertexDeclaration9** ppDecl)
{
	return(origIDirect3DDevice9->GetVertexDeclaration(ppDecl));
}

HRESULT IDirect3DDevice9Proxy::SetFVF(DWORD FVF)
{
	logFVF << "SetFVF(FVF=0x" << std::hex << FVF << std::dec << ")\n";

	return(origIDirect3DDevice9->SetFVF(FVF));
}

HRESULT IDirect3DDevice9Proxy::GetFVF(DWORD* pFVF)
{
	return(origIDirect3DDevice9->GetFVF(pFVF));
}

HRESULT IDirect3DDevice9Proxy::CreateVertexShader(CONST DWORD* pFunction,IDirect3DVertexShader9** ppShader)
{
	return(origIDirect3DDevice9->CreateVertexShader(pFunction,ppShader));
}
HRESULT IDirect3DDevice9Proxy::SetVertexShader(IDirect3DVertexShader9* pShader)
{
	logVertexShader << "SetVertexShader(pShader=" << pShader << ")\n";
	logVertexShader.flush();

	return origIDirect3DDevice9->SetVertexShader(pShader);
}

HRESULT IDirect3DDevice9Proxy::GetVertexShader(IDirect3DVertexShader9** ppShader)
{
	return(origIDirect3DDevice9->GetVertexShader(ppShader));
}

HRESULT IDirect3DDevice9Proxy::SetVertexShaderConstantF(UINT StartRegister,CONST float* pConstantData,UINT Vector4fCount)
{
	return(origIDirect3DDevice9->SetVertexShaderConstantF(StartRegister,pConstantData, Vector4fCount));
}

HRESULT IDirect3DDevice9Proxy::GetVertexShaderConstantF(UINT StartRegister,float* pConstantData,UINT Vector4fCount)
{
	return(origIDirect3DDevice9->GetVertexShaderConstantF(StartRegister,pConstantData,Vector4fCount));
}

HRESULT IDirect3DDevice9Proxy::SetVertexShaderConstantI(UINT StartRegister,CONST int* pConstantData,UINT Vector4iCount)
{
	return(origIDirect3DDevice9->SetVertexShaderConstantI(StartRegister,pConstantData,Vector4iCount));
}

HRESULT IDirect3DDevice9Proxy::GetVertexShaderConstantI(UINT StartRegister,int* pConstantData,UINT Vector4iCount)
{
	return(origIDirect3DDevice9->GetVertexShaderConstantI(StartRegister,pConstantData,Vector4iCount));
}

HRESULT IDirect3DDevice9Proxy::SetVertexShaderConstantB(UINT StartRegister,CONST BOOL* pConstantData,UINT  BoolCount)
{
	return(origIDirect3DDevice9->SetVertexShaderConstantB(StartRegister,pConstantData,BoolCount));
}

HRESULT IDirect3DDevice9Proxy::GetVertexShaderConstantB(UINT StartRegister,BOOL* pConstantData,UINT BoolCount)
{
	return(origIDirect3DDevice9->GetVertexShaderConstantB(StartRegister,pConstantData,BoolCount));
}

HRESULT IDirect3DDevice9Proxy::SetStreamSource(UINT StreamNumber, IDirect3DVertexBuffer9* pStreamData, UINT OffsetInBytes, UINT Stride)
{
	logStream << "SetStreamSource(Stream=" << StreamNumber
		<< ", VB=" << pStreamData
		<< ", Offset=" << OffsetInBytes
		<< ", Stride=" << Stride << ")\n";

	if (StreamNumber == 0)
	{
		IDirect3DBaseTexture9* tex = nullptr;
		origIDirect3DDevice9->GetTexture(0, &tex);

		char buf[512];
		sprintf_s(buf, "[SetStreamSource] Stream=%u, Offset=%u, Stride=%u, VBuffer=%p, Tex0=%p\n",
			StreamNumber, OffsetInBytes, Stride, pStreamData, tex);
		OutputDebugStringA(buf);

		if (tex) tex->Release();
	}

	return origIDirect3DDevice9->SetStreamSource(StreamNumber, pStreamData, OffsetInBytes, Stride);
}


HRESULT IDirect3DDevice9Proxy::GetStreamSource(UINT StreamNumber,IDirect3DVertexBuffer9** ppStreamData,UINT* OffsetInBytes,UINT* pStride)
{
	return(origIDirect3DDevice9->GetStreamSource(StreamNumber,ppStreamData,OffsetInBytes,pStride));
}

HRESULT IDirect3DDevice9Proxy::SetStreamSourceFreq(UINT StreamNumber,UINT Divider)
{
	return(origIDirect3DDevice9->SetStreamSourceFreq(StreamNumber,Divider));
}

HRESULT IDirect3DDevice9Proxy::GetStreamSourceFreq(UINT StreamNumber,UINT* Divider)
{
	return(origIDirect3DDevice9->GetStreamSourceFreq(StreamNumber,Divider));
}

HRESULT IDirect3DDevice9Proxy::SetIndices(IDirect3DIndexBuffer9* pIndexData)
{
	logVertexBuffer << "SetIndices(IndexBuffer=" << pIndexData << ")\n";

	return(origIDirect3DDevice9->SetIndices(pIndexData));
}

HRESULT IDirect3DDevice9Proxy::GetIndices(IDirect3DIndexBuffer9** ppIndexData)
{
	return(origIDirect3DDevice9->GetIndices(ppIndexData));
}

HRESULT IDirect3DDevice9Proxy::CreatePixelShader(CONST DWORD* pFunction,IDirect3DPixelShader9** ppShader)
{
	return(origIDirect3DDevice9->CreatePixelShader(pFunction,ppShader));
}

HRESULT IDirect3DDevice9Proxy::SetPixelShader(IDirect3DPixelShader9* pShader)
{
	logPixelShader << "SetPixelShader(pShader=" << pShader << ")\n";
	logPixelShader.flush();
	if (g_captureNextFrame)
	g_frameLog << "SetPixelShader(" << pShader << ")\n";
	g_frameLog.flush();
	return origIDirect3DDevice9->SetPixelShader(pShader);
}


HRESULT IDirect3DDevice9Proxy::GetPixelShader(IDirect3DPixelShader9** ppShader)
{
	return(origIDirect3DDevice9->GetPixelShader(ppShader));
}

HRESULT IDirect3DDevice9Proxy::SetPixelShaderConstantF(UINT StartRegister,CONST float* pConstantData,UINT Vector4fCount)
{
	return(origIDirect3DDevice9->SetPixelShaderConstantF(StartRegister,pConstantData,Vector4fCount));
}

HRESULT IDirect3DDevice9Proxy::GetPixelShaderConstantF(UINT StartRegister,float* pConstantData,UINT Vector4fCount)
{
	return(origIDirect3DDevice9->GetPixelShaderConstantF(StartRegister,pConstantData,Vector4fCount));
}

HRESULT IDirect3DDevice9Proxy::SetPixelShaderConstantI(UINT StartRegister,CONST int* pConstantData,UINT Vector4iCount)
{
	return(origIDirect3DDevice9->SetPixelShaderConstantI(StartRegister,pConstantData,Vector4iCount));
}

HRESULT IDirect3DDevice9Proxy::GetPixelShaderConstantI(UINT StartRegister,int* pConstantData,UINT Vector4iCount)
{
	return(origIDirect3DDevice9->GetPixelShaderConstantI(StartRegister,pConstantData,Vector4iCount));
}

HRESULT IDirect3DDevice9Proxy::SetPixelShaderConstantB(UINT StartRegister,CONST BOOL* pConstantData,UINT  BoolCount)
{
	return(origIDirect3DDevice9->SetPixelShaderConstantB(StartRegister,pConstantData,BoolCount));
}

HRESULT IDirect3DDevice9Proxy::GetPixelShaderConstantB(UINT StartRegister,BOOL* pConstantData,UINT BoolCount)
{
	return(origIDirect3DDevice9->GetPixelShaderConstantB(StartRegister,pConstantData,BoolCount));
}

HRESULT IDirect3DDevice9Proxy::DrawRectPatch(UINT Handle,CONST float* pNumSegs,CONST D3DRECTPATCH_INFO* pRectPatchInfo)
{
	return(origIDirect3DDevice9->DrawRectPatch(Handle,pNumSegs, pRectPatchInfo));
}

HRESULT IDirect3DDevice9Proxy::DrawTriPatch(UINT Handle,CONST float* pNumSegs,CONST D3DTRIPATCH_INFO* pTriPatchInfo)
{
	return(origIDirect3DDevice9->DrawTriPatch(Handle, pNumSegs, pTriPatchInfo));
}

HRESULT IDirect3DDevice9Proxy::DeletePatch(UINT Handle)
{
	return(origIDirect3DDevice9->DeletePatch(Handle));
}

HRESULT IDirect3DDevice9Proxy::CreateQuery(D3DQUERYTYPE Type,IDirect3DQuery9** ppQuery)
{
	return(origIDirect3DDevice9->CreateQuery(Type,ppQuery));
}
