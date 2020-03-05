#include <windows.h>
#include <math.h>
#include <stdio.h>
#include <d3d8.h>
#pragma comment (lib, "d3d8.lib")
#include <d3dx8.h>
#pragma comment (lib, "d3dx8.lib")
#include <MMSystem.h> 
#pragma comment (lib, "WinMM.Lib")
#define APPNAME  "Grid 3D"
char	AppTitle[96];
#define _RELEASE_(p) { if(p) { (p)->Release(); (p)=NULL; };};
#define _DELETE_(p)  { if(p) { delete (p);     (p)=NULL; };};

LPDIRECT3D8             p_d3d          = NULL;
LPDIRECT3DDEVICE8       p_d3d_Device   = NULL;
LPDIRECT3DVERTEXBUFFER8 p_VertexBuffer = NULL;
LPDIRECT3DINDEXBUFFER8  p_IndexBuffer  = NULL;
LPDIRECT3DTEXTURE8		p_Texture_001  = NULL;

D3DLIGHT8 light;	
D3DTEXTUREFILTERTYPE CurrentFilter;
D3DFILLMODE			 CurrentFillMd;

#define D3DFVF_CUSTOMVERTEX ( D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1)
struct CUSTOMVERTEX { float x, y, z, nx, ny, nz, tu, tv; };

#define FLAG_SIZE         64
#define NUM_FLAG_VERTICES ((FLAG_SIZE+1)*(FLAG_SIZE+1))
#define NUM_FLAG_INDICES  (FLAG_SIZE*FLAG_SIZE*6)

HWND hWnd;
HINSTANCE hTInst,hPInst;
int CmdShow;
bool fPlayMode=true;
bool fWiteMode=false;
bool fAlfaBlnd=false;

float a=35.0f;
#define vertA {  -a, 0.0f,     -a,	0.0f,	1.0f,	1.0f,	0.0f,	1.0f}
#define vertB {  -a, 0.0f,      a,	0.0f,	1.0f,	1.0f,	0.0f,	0.0f}
#define vertC {   a, 0.0f,     -a,	0.0f,	1.0f,	1.0f,	1.0f,	1.0f}
#define vertD {   a, 0.0f,      a,	0.0f,	1.0f,	1.0f,	1.0f,	0.0f}
#define vertS {0.0f, a*1.2f, 0.0f,	0.0f,	1.0f,	1.0f,	0.5f,	0.5f}

CUSTOMVERTEX g_Vertices[12] = {
	vertS,	vertB,	vertA,
	vertS,	vertD,	vertB,
	vertS,	vertC,	vertD,
	vertS,	vertA,	vertC,
};

CUSTOMVERTEX g_IndexedV[5] = {
	vertA, vertB, vertC, vertD, vertS,
};

WORD d_Indexses[12]={
	4,1,0,	4,3,1,	4,2,3,	4,0,2,
};

void DestroyDirect3D8 (void){
	_RELEASE_ (p_Texture_001);
	_RELEASE_ (p_IndexBuffer);
	_RELEASE_ (p_VertexBuffer);
	_RELEASE_ (p_d3d_Device);
	_RELEASE_ (p_d3d);
};

VOID NormalizeTriangles(int i, CUSTOMVERTEX vertexlelist[]){
 	for (int j=0;j<i;j+=3){
		D3DXVECTOR3 v1 = D3DXVECTOR3 (vertexlelist[j+1].x-vertexlelist[j].x, vertexlelist[j+1].y-vertexlelist[j].y, vertexlelist[j+1].z-vertexlelist[j].z);
		D3DXVECTOR3 v2 = D3DXVECTOR3 (vertexlelist[j+2].x-vertexlelist[j+1].x, vertexlelist[j+2].y-vertexlelist[j+1].y, vertexlelist[j+2].z-vertexlelist[j+1].z);
		D3DXVec3Cross(&v1,&v1,&v2);
		D3DXVec3Normalize(&v1,&v1);
		vertexlelist[j].nx=v1.x;
		vertexlelist[j].ny=v1.y;
		vertexlelist[j].nz=v1.z;
		vertexlelist[j+1].nx=v1.x;
		vertexlelist[j+1].ny=v1.y;
		vertexlelist[j+1].nz=v1.z;
		vertexlelist[j+2].nx=v1.x;
		vertexlelist[j+2].ny=v1.y;
		vertexlelist[j+2].nz=v1.z;
	};
};

void SetVS4(int x, int y, CUSTOMVERTEX* VS){
	float t1;
	float   u=(float)x*2-FLAG_SIZE;
	float   v=(float)y*2-FLAG_SIZE;
	float txf=FLAG_SIZE/8;
	if(!fPlayMode) {t1=0;} else {t1=timeGetTime()/640.0f;};
    VS->x  = (float)(x*2-FLAG_SIZE);
	VS->y  = 128*cos(sqrt(u*u+v*v)/6+t1)/sqrt(u*u+v*v+48)+16;
	VS->z  = (float)(y*2-FLAG_SIZE);
	VS->tu = (float)x*txf/(FLAG_SIZE);
	VS->tv = (float)y*txf/(FLAG_SIZE);
};

HRESULT TransGeometry4(){
	CUSTOMVERTEX* pVertices;
	if(FAILED(p_VertexBuffer->Lock (0,NUM_FLAG_INDICES*sizeof(CUSTOMVERTEX),
		(BYTE**)&pVertices,0))) return E_FAIL;
	WORD i=0;
	for(int ix=0; ix<FLAG_SIZE; ix++){
		for(int iy=0; iy<FLAG_SIZE; iy++){
			SetVS4(ix,   iy,   &pVertices[i++]);			
			SetVS4(ix+1, iy,   &pVertices[i++]);			
			SetVS4(ix,   iy+1, &pVertices[i++]);			
			SetVS4(ix+1, iy,   &pVertices[i++]);			
			SetVS4(ix+1, iy+1, &pVertices[i++]);			
			SetVS4(ix,   iy+1, &pVertices[i++]);			
		};
	};
	NormalizeTriangles(NUM_FLAG_INDICES, pVertices);
	p_VertexBuffer->Unlock();
	return  S_OK;
};

HRESULT InitGeometry4(){
	if(FAILED(p_d3d_Device->CreateVertexBuffer (
		NUM_FLAG_INDICES*sizeof(CUSTOMVERTEX),	
		0,										
		D3DFVF_CUSTOMVERTEX,					
		D3DPOOL_MANAGED,						
		&p_VertexBuffer))) return E_FAIL;		
	WORD i=0;
	CUSTOMVERTEX* pVertices;
	if(FAILED(p_VertexBuffer->Lock (
		0,										
		NUM_FLAG_INDICES*sizeof(CUSTOMVERTEX),  
		(BYTE**)&pVertices,						
		0))) return E_FAIL;						
	for(int ix=0; ix<FLAG_SIZE; ix++){
		for(int iy=0; iy<FLAG_SIZE; iy++){
			SetVS4(ix,   iy,   &pVertices[i++]);			
			SetVS4(ix+1, iy,   &pVertices[i++]);			
			SetVS4(ix,   iy+1, &pVertices[i++]);			
			SetVS4(ix+1, iy,   &pVertices[i++]);			
			SetVS4(ix+1, iy+1, &pVertices[i++]);			
			SetVS4(ix,   iy+1, &pVertices[i++]);			
		};
	};
	NormalizeTriangles(NUM_FLAG_INDICES, pVertices);
	p_VertexBuffer->Unlock();
	return  S_OK;
};

HRESULT InitGeometry3(){
	if(FAILED(p_d3d_Device->CreateVertexBuffer (
		sizeof(g_IndexedV),						
		0,										
		D3DFVF_CUSTOMVERTEX,					
		D3DPOOL_MANAGED,						
		&p_VertexBuffer))) return E_FAIL;		
	VOID* pVertices;
	if(FAILED(p_VertexBuffer->Lock (
		0,										
		sizeof(g_IndexedV),						
		(BYTE**)&pVertices,						
		0))) return E_FAIL;						
	memcpy (pVertices, g_IndexedV, sizeof(g_IndexedV));
	p_VertexBuffer->Unlock();
	
	if(FAILED(p_d3d_Device->CreateIndexBuffer (
		sizeof(d_Indexses),				
		0,								
		D3DFMT_INDEX16,					
		D3DPOOL_MANAGED,				
		&p_IndexBuffer))) return E_FAIL;
	VOID* pVerticesI;
	if(FAILED(p_IndexBuffer->Lock (
		0,
		sizeof(d_Indexses),
		(BYTE**)&pVerticesI,
		0))) return E_FAIL;
	memcpy (pVerticesI, d_Indexses, sizeof(d_Indexses));
	p_IndexBuffer->Unlock();
	return  S_OK;
};

HRESULT InitGeometry2(){
	NormalizeTriangles(sizeof(g_Vertices)/sizeof(CUSTOMVERTEX), g_Vertices);
	if(FAILED(p_d3d_Device->CreateVertexBuffer (
		sizeof(g_Vertices),					
		0,									
		D3DFVF_CUSTOMVERTEX,				
		D3DPOOL_MANAGED,					
		&p_VertexBuffer))) return E_FAIL;	
	VOID* pVertices;
	if(FAILED(p_VertexBuffer->Lock (
		0,							
		sizeof(g_Vertices),			
		(BYTE**)&pVertices,			
		0))) return E_FAIL;			
	memcpy (pVertices, g_Vertices, sizeof(g_Vertices));
	p_VertexBuffer->Unlock();
	return  S_OK;
}

HRESULT InitGeometry(){
	if(FAILED(p_d3d_Device->CreateVertexBuffer (
		NUM_FLAG_VERTICES*sizeof(CUSTOMVERTEX), 
		0,										
		D3DFVF_CUSTOMVERTEX,					
		D3DPOOL_MANAGED,						
		&p_VertexBuffer))) return E_FAIL;		
	CUSTOMVERTEX* pVertices;
	if(FAILED(p_VertexBuffer->Lock (
		0,										
		NUM_FLAG_VERTICES*sizeof(CUSTOMVERTEX), 
		(BYTE**)&pVertices,						
		0))) return E_FAIL;						
	for(WORD ix=0; ix<FLAG_SIZE+1; ix++){
		for(WORD iy=0; iy<FLAG_SIZE+1; iy++){
			FLOAT tu = ix/(float)(FLAG_SIZE+1);
			FLOAT tv = iy/(float)(FLAG_SIZE+1);
			pVertices[ix+iy*(FLAG_SIZE+1)].x = (float)(ix - FLAG_SIZE/2);
			pVertices[ix+iy*(FLAG_SIZE+1)].y = 3.0f*(cos((float)iy/3.0f)+cos((float)ix/3.0f));
			pVertices[ix+iy*(FLAG_SIZE+1)].z = (float)(iy - FLAG_SIZE/2);
			pVertices[ix+iy*(FLAG_SIZE+1)].nx = 0.0f;
			pVertices[ix+iy*(FLAG_SIZE+1)].ny = 1.0f;
			pVertices[ix+iy*(FLAG_SIZE+1)].nz = 0.0f;
			pVertices[ix+iy*(FLAG_SIZE+1)].tu = tu;
			pVertices[ix+iy*(FLAG_SIZE+1)].tv = tv;
		};
	};
	p_VertexBuffer->Unlock();
	if(FAILED(p_d3d_Device->CreateIndexBuffer (
		NUM_FLAG_INDICES*sizeof(WORD),	
		0,								
		D3DFMT_INDEX16,					
		D3DPOOL_MANAGED,				
		&p_IndexBuffer))) return E_FAIL;
	WORD* pVerticesI;
	if(FAILED(p_IndexBuffer->Lock (
		0,
		NUM_FLAG_INDICES*sizeof(WORD),
		(BYTE**)&pVerticesI,
		0))) return E_FAIL;
	for(WORD i=0, iy=0; iy<FLAG_SIZE; iy++ ){
		for(WORD ix=0; ix<FLAG_SIZE; ix++){
			pVerticesI[i++] = (ix+0) + (iy+1)*(FLAG_SIZE+1);
			pVerticesI[i++] = (ix+0) + (iy+0)*(FLAG_SIZE+1);
			pVerticesI[i++] = (ix+1) + (iy+0)*(FLAG_SIZE+1);
			pVerticesI[i++] = (ix+0) + (iy+1)*(FLAG_SIZE+1);
			pVerticesI[i++] = (ix+1) + (iy+0)*(FLAG_SIZE+1);
			pVerticesI[i++] = (ix+1) + (iy+1)*(FLAG_SIZE+1);
		};
	};
	p_IndexBuffer->Unlock();
	return  S_OK;
};

HRESULT SetUpD3D(void){
	D3DDISPLAYMODE			d3ddm;
	D3DPRESENT_PARAMETERS	d3dpp;
	if(NULL == (p_d3d = Direct3DCreate8(D3D_SDK_VERSION))) return E_FAIL;
	if(FAILED(p_d3d->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm))) return E_FAIL;
	ZeroMemory (&d3dpp, sizeof(d3dpp));
	d3dpp.BackBufferFormat = d3ddm.Format;
	d3dpp.Windowed = true;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.EnableAutoDepthStencil = true;
	d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
	if(FAILED(p_d3d->CreateDevice(
			D3DADAPTER_DEFAULT,
			D3DDEVTYPE_HAL, 
			hWnd,
			D3DCREATE_SOFTWARE_VERTEXPROCESSING,
			&d3dpp,
			&p_d3d_Device))){
		if(FAILED(p_d3d->CreateDevice(
				D3DADAPTER_DEFAULT,
				D3DDEVTYPE_REF, 
				hWnd,
				D3DCREATE_SOFTWARE_VERTEXPROCESSING,
				&d3dpp,
				&p_d3d_Device)))
			return E_FAIL;
	}
	
	D3DXVECTOR3 vecDir;
	ZeroMemory (&light, sizeof(D3DLIGHT8));
	light.Type = D3DLIGHT_DIRECTIONAL;
	light.Diffuse.r  = 1.0f;
	light.Diffuse.g  = 1.0f;
	light.Diffuse.b  = 1.0f;
	light.Range = 1000.0f;
	p_d3d_Device->LightEnable (0, true);
	p_d3d_Device->SetRenderState(D3DRS_LIGHTING, true);
	p_d3d_Device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	p_d3d_Device->SetRenderState(D3DRS_ZENABLE, TRUE);
	p_d3d_Device->SetRenderState(D3DRS_FILLMODE, CurrentFillMd);
	p_d3d_Device->SetRenderState(D3DRS_AMBIENT, 0x00502020); 
	p_d3d_Device->SetRenderState (D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	p_d3d_Device->SetRenderState (D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	return S_OK;
}

VOID PlayWorld(){
	float t1=timeGetTime()/640.0f;
	float t2=t1/2.0f;
	float t3=t1/3.0f;
    D3DXMATRIX matWorld;
	D3DXMatrixIdentity(&matWorld);
    D3DXMatrixRotationY(&matWorld, 0);
    p_d3d_Device->SetTransform(D3DTS_WORLD, &matWorld);
	D3DXMATRIX  matView;
	D3DXVECTOR3 vEyePt( sin(t2)*(cos(t3)+1.2f)*50.0f, (sin(t3)+1.2f)*36.0f, cos(t2)*(cos(t3)+1.2f)*50);
	D3DXVECTOR3 vLookatPt( 0.0f, 0.0f, 0.0f );
	D3DXVECTOR3 vUpVec( 0.0f, 1.0f, 0.0f );
	D3DXMatrixLookAtLH(&matView, &vEyePt, &vLookatPt, &vUpVec);
	p_d3d_Device->SetTransform(D3DTS_VIEW, &matView);
    D3DXMATRIX matProj;
    D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI/4, 1.0f, 1.0f, 1000.0f);
    p_d3d_Device->SetTransform(D3DTS_PROJECTION, &matProj);
	D3DXVECTOR3 vecDir = D3DXVECTOR3(
		cosf(t1),
        1.0f,
        sinf(t1));
	D3DXVec3Normalize((D3DXVECTOR3*)&light.Direction, &vecDir);
	p_d3d_Device->SetLight (0, &light);
	if(fPlayMode) TransGeometry4();
	D3DMATERIAL8 mtrl1;
	ZeroMemory (&mtrl1, sizeof(D3DMATERIAL8));
	mtrl1.Diffuse.r = mtrl1.Ambient.r = 1.0f;
	mtrl1.Diffuse.g = mtrl1.Ambient.g = 1.0f;
	mtrl1.Diffuse.b = mtrl1.Ambient.b = 0.0f;
	mtrl1.Diffuse.a = mtrl1.Ambient.a = 1.0f;
	p_d3d_Device->SetMaterial (&mtrl1);
	p_d3d_Device->SetTexture(0, p_Texture_001);
	p_d3d_Device->SetTextureStageState(0, D3DTSS_MAGFILTER, CurrentFilter);
	p_d3d_Device->SetTextureStageState(0, D3DTSS_MINFILTER, CurrentFilter);
	p_d3d_Device->SetTextureStageState(0, D3DTSS_MIPFILTER, CurrentFilter);
	if (fAlfaBlnd) {
		p_d3d_Device->SetRenderState (D3DRS_ALPHABLENDENABLE, true);
	}else{
		p_d3d_Device->SetRenderState (D3DRS_ALPHABLENDENABLE, false);
	};
};

void RenderScreen (void){
	if(!fWiteMode) {
		p_d3d_Device->Clear (0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB (0, 0, 0), 1.0f, 0);
	}else{
		p_d3d_Device->Clear (0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB (255, 255, 255), 1.0f, 0);
	}
	p_d3d_Device->BeginScene ();
	PlayWorld();
	p_d3d_Device->SetStreamSource (0, p_VertexBuffer, sizeof(CUSTOMVERTEX));
	p_d3d_Device->SetVertexShader (D3DFVF_CUSTOMVERTEX);
	p_d3d_Device->DrawPrimitive (D3DPT_TRIANGLELIST, 0, NUM_FLAG_INDICES/3);
	p_d3d_Device->EndScene();
	p_d3d_Device->Present (NULL, NULL, NULL, NULL);
};

void SetTitle(){
	strcpy(AppTitle,"Grid 3D ");
	switch (CurrentFilter){
	case D3DTEXF_NONE:
		strcat(AppTitle,"(Filter=D3DTEXF_NONE ");
		break;
	case D3DTEXF_POINT:
		strcat(AppTitle,"(Filter=D3DTEXF_POINT ");
		break;
	case D3DTEXF_LINEAR:
		strcat(AppTitle,"(Filter=D3DTEXF_LINEAR ");
		break;
	case D3DTEXF_ANISOTROPIC:
		strcat(AppTitle,"(Filter=D3DTEXF_ANISOTROPIC ");
		break;
	};
	switch (CurrentFillMd){
	case D3DFILL_POINT:
		strcat(AppTitle,"FillMode=D3DFILL_POINT)");
		break;
	case D3DFILL_WIREFRAME:
		strcat(AppTitle,"FillMode=D3DFILL_WIREFRAME)");
		break;
	case D3DFILL_SOLID:
		strcat(AppTitle,"FillMode=D3DFILL_SOLID)");
		break;
	};
	strcat(AppTitle," Copyright (c) Grushetsky V.A.");
};

LRESULT CALLBACK WindowProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam){
	BOOL modechangefl;
	switch (message){
	case WM_KEYDOWN:{
		int VK = (int)wParam;
		modechangefl=true;
		if (VK==VK_F8)  {fPlayMode=!fPlayMode;} else
		if (VK==VK_F9)  {fWiteMode=!fWiteMode;} else
		if (VK==VK_F11) {fAlfaBlnd=!fAlfaBlnd;} else
		switch(VK){
		case VK_F1:
			CurrentFilter = D3DTEXF_NONE;
			break;
		case VK_F2:
			CurrentFilter = D3DTEXF_POINT;
			break;
		case VK_F3:
			CurrentFilter = D3DTEXF_LINEAR;
			break;
		case VK_F4:
			CurrentFilter = D3DTEXF_ANISOTROPIC;
			break;
		case VK_F5:
			CurrentFillMd = D3DFILL_POINT;
			break;
		case VK_F6:
			CurrentFillMd = D3DFILL_WIREFRAME;
			break;
		case VK_F7:
			CurrentFillMd = D3DFILL_SOLID;
			break;
		default:
			modechangefl=false;
		};
		if (modechangefl){
			SetTitle();
			SetWindowText(hWnd, AppTitle);
			p_d3d_Device->SetRenderState (D3DRS_FILLMODE, CurrentFillMd);
		};
		break;
	};
	case WM_DESTROY:
		PostQuitMessage (0);
		break;
	};
	return DefWindowProc(hWnd, message, wParam, lParam);
};

bool WindowInit (HINSTANCE hThisInst, int nCmdShow){
	WNDCLASS		    wcl;
	wcl.hInstance		= hThisInst;
	wcl.lpszClassName	= APPNAME;
	wcl.lpfnWndProc		= WindowProc;
	wcl.style			= 0;
	wcl.hIcon			= LoadIcon (hThisInst, IDC_ICON);
	wcl.hCursor			= LoadCursor (hThisInst, IDC_ARROW);
	wcl.lpszMenuName	= NULL;
	wcl.cbClsExtra		= 0;
	wcl.cbWndExtra		= 0;
	wcl.hbrBackground	= (HBRUSH) GetStockObject (BLACK_BRUSH);
	RegisterClass (&wcl);
	int sx = GetSystemMetrics (SM_CXSCREEN);
	int sy = GetSystemMetrics (SM_CYSCREEN);

	hWnd = CreateWindow(
		APPNAME,
		AppTitle,
		WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU | WS_MAXIMIZEBOX,
		sx/2-512, sy/2-384,
		1024,
		768,
		NULL,
		NULL,
		hThisInst,
		NULL);
	if(!hWnd) return false;
	return true;
};

int APIENTRY WinMain (HINSTANCE hThisInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow){
	MSG msg;
	HRESULT d3derrcode;
	char d3derrmsg[64];
	hTInst=hThisInst;
	hPInst=hPrevInst;
	CmdShow=nCmdShow;
	CurrentFilter = D3DTEXF_NONE;
	CurrentFillMd = D3DFILL_SOLID;
	SetTitle();
	if(!WindowInit (hThisInst, nCmdShow)) return false;
	d3derrcode=SetUpD3D();
	if(!d3derrcode)
		d3derrcode=InitGeometry4();
	if (d3derrcode){
		sprintf(d3derrmsg, "Initialization procedure return an error code: %dx", d3derrcode);
		MessageBox(NULL, d3derrmsg, "ERROR", MB_ICONERROR);
		return 1;
	};
	if(FAILED(D3DXCreateTextureFromFileEx (
			p_d3d_Device,
			"005.png",
			0, 0, 0, 0,
			D3DFMT_UNKNOWN,
			D3DPOOL_MANAGED,
			D3DX_DEFAULT,
			D3DX_DEFAULT,
			0xFF000000,
			NULL,
			NULL,
			&p_Texture_001))){
		MessageBox(NULL, "Не найдена текстура 005.png", "ERROR", MB_ICONERROR);
		return 2;
	}
	ShowWindow (hWnd, nCmdShow);
	UpdateWindow (hWnd);
	while (1)	{
		if(PeekMessage (&msg, NULL, 0, 0, PM_NOREMOVE)){
			if(!GetMessage (&msg, NULL, 0, 0)) break;
			TranslateMessage (&msg);
			DispatchMessage (&msg);
		}else RenderScreen ();
	};
	DestroyDirect3D8();
	return 0;
};

