// include the basic windows header files and the Direct3D header file
#pragma once
#include <windows.h>
#include <windowsx.h>
#include <DirectXCollision.h>
#include <DirectXCollision.inl>
#include <d3d9.h>
#include <d3dx9.h>
#include <XAudio2.h>
#include <d3dx9tex.h>
#include <vector>
#include <string>
#include <thread>
#include "resource.h"

#ifdef _XBOX //Big-Endian
#define fourccRIFF 'RIFF'
#define fourccDATA 'data'
#define fourccFMT 'fmt '
#define fourccWAVE 'WAVE'
#define fourccXWMA 'XWMA'
#define fourccDPDS 'dpds'
#endif

#ifndef _XBOX //Little-Endian
#define fourccRIFF 'FFIR'
#define fourccDATA 'atad'
#define fourccFMT ' tmf'
#define fourccWAVE 'EVAW'
#define fourccXWMA 'AMWX'
#define fourccDPDS 'sdpd'
#endif

// define the screen resolution
#define SCREEN_WIDTH 2560
#define SCREEN_HEIGHT 1440
#define BG_COLOR D3DCOLOR_XRGB(0, 0, 0)

// include the Direct3D Library files
#pragma comment (lib, "d3d9.lib")
#pragma comment (lib, "d3dx9.lib")

// global declarations
#pragma once
static LPDIRECT3D9 d3d;									// the pointer to our Direct3D interface
static LPDIRECT3DDEVICE9 d3ddev;						// the pointer to the device class
static LPDIRECT3DVERTEXBUFFER9 global_v_buffer = NULL;	// the pointer to the vertex buffer
static LPDIRECT3DINDEXBUFFER9 global_i_buffer = NULL;	// the pointer to the index buffer
static ID3DXFont* font = NULL;
static float* WorldYRotation = new float(0);			// rotation degree Y (temp)
static float* WorldXRotation = new float(0);			// rotation degree X (temp)
static float* WorldZRotation = new float(0);			// rotation degree Z (temp)
static float* WorldYTranslation = new float(0);			// translation value Z (temp)
static float* WorldXTranslation = new float(0);			// translation value X (temp)
static float* camX = new float(0);						
static float* camY = new float(0);
static float* camZ = new float(0);

bool* running = new bool(FALSE);
static IDirect3DSurface9* LoadSurfaceFromFile(std::string FileName)
{
	HRESULT hResult;
	IDirect3DSurface9* surface = NULL;
	D3DXIMAGE_INFO imageInfo;
	LPCTSTR str = FileName.c_str();//so we get a pointer to a proper string to begin with
	hResult = D3DXGetImageInfoFromFile(str, &imageInfo);
	if (FAILED(hResult))
	{
		//return NULL;
		imageInfo.Height = 824;
		imageInfo.Width = 2000;
		imageInfo.Depth = 32;
	}
	hResult = d3ddev->CreateOffscreenPlainSurface(imageInfo.Width,
		imageInfo.Height,
		D3DFMT_X8R8G8B8,
		D3DPOOL_DEFAULT,
		&surface,
		NULL);
	if (FAILED(hResult))
	{
		return NULL;
	}

	hResult = D3DXLoadSurfaceFromFile(surface, NULL, NULL, str, NULL, D3DX_DEFAULT, 0, NULL);

	if (FAILED(hResult))
	{
		return NULL;
	}

	return surface;
}

#define CUSTOMFVF (D3DFVF_XYZ | D3DFVF_NORMAL)			// shaded FVF
//#define CUSTOMFVF (D3DFVF_XYZ | D3DFVF_DIFFUSE)			// unshaded FVF

struct SHADEDVERTEX
{
	FLOAT x, y, z;
	D3DVECTOR NORMAL;
};
struct VERTEX
{
	FLOAT x, y, z;
	DWORD color;
	VERTEX() : VERTEX(0.0f, 0.0f, 0.0f, D3DCOLOR_XRGB(0, 0, 0)) {}
	VERTEX(FLOAT X, FLOAT Y, FLOAT Z, DWORD Color)
	{
		x = X;
		y = Y;
		z = Z;
		color = Color;
	}
};

// representing cameras that can be switched
class CAMERA
{
private:
	D3DXMATRIX cam;
	D3DXVECTOR3 eye;
	D3DXVECTOR3 lookAt;
	D3DXVECTOR3 up;

public:
	CAMERA(D3DXVECTOR3 pos, D3DXVECTOR3 lookat, D3DXVECTOR3 up)
	{
		D3DXMatrixLookAtLH(&cam, &pos, &lookat, &up);
		eye = pos;
		lookAt = lookat;
		this->up = up;
	}

	CAMERA() : CAMERA(D3DXVECTOR3(0, 0, 0), D3DXVECTOR3(0, 0, 0), D3DXVECTOR3(0, 1, 0)) {}

	// set World Camera to this->cam
	void setCamera() { d3ddev->SetTransform(D3DTS_VIEW, &cam); }

	// change Camera to absolute position
	// update: TRUE if camera should also be set as current cam
	void changePos(D3DXVECTOR3 pos, bool update = TRUE)
	{
		eye = pos;
		D3DXMatrixLookAtLH(&cam, &eye, &lookAt, &up);
		if (update) d3ddev->SetTransform(D3DTS_VIEW, &cam);
	}
	void changePos(FLOAT xPos, FLOAT yPos, FLOAT zPos, bool update = TRUE) 
	{
		changePos(D3DXVECTOR3(xPos, yPos, zPos), update);
	}

	// change Camera to absolute lookat
	// update: TRUE if camera should also be set as current cam
	void changeLookAt(D3DXVECTOR3 lookat, bool update = TRUE)
	{
		lookAt = lookat;
		D3DXMatrixLookAtLH(&cam, &eye, &lookAt, &up);
		if (update) d3ddev->SetTransform(D3DTS_VIEW, &cam);
	}
	void changeLookAt(FLOAT xLA, FLOAT yLA, FLOAT zLA, bool update = TRUE)
	{
		changeLookAt(D3DXVECTOR3(xLA, yLA, zLA), update);
	}

	// change Camera up direction
	// update: TRUE if camera should also be set as current cam
	void changeUp(D3DXVECTOR3 Up, bool update = TRUE)
	{
		up = Up;
		D3DXMatrixLookAtLH(&cam, &eye, &lookAt, &up);
		if (update) d3ddev->SetTransform(D3DTS_VIEW, &cam);
	}
	void changeUp(FLOAT x, FLOAT y, FLOAT z, bool update = TRUE)
	{
		changeUp(D3DXVECTOR3(x, y, z), update);
	}

	// set dynamic move offset of position and lookat
	// update: TRUE if camera should also be set as current cam
	void move(D3DXVECTOR3 move, bool update = TRUE)
	{
		lookAt.z += move.z;
		eye += move;
		D3DXMatrixLookAtLH(&cam, &eye, &lookAt, &up);
		if (update) d3ddev->SetTransform(D3DTS_VIEW, &cam);
	}
	void move(FLOAT xMove, FLOAT yMove, FLOAT zMove, bool update = TRUE)
	{
		move(D3DXVECTOR3(xMove, yMove, zMove), update);
	}

	// set dynamic look offset
	void look(FLOAT xLookOffset, FLOAT yLookOffset)
	{
		lookAt.x += xLookOffset;
		lookAt.y += yLookOffset;
		// handle turn magic
		D3DXMatrixLookAtLH(&cam, &eye, &lookAt, &up);
		d3ddev->SetTransform(D3DTS_VIEW, &cam);
	}

	D3DXMATRIX* getCam() { return &cam; }
	D3DXVECTOR3 getCamPos() {}
	D3DXVECTOR3 getCamLookAt() {}
	D3DXVECTOR3 getCamUp() {}

	void operator=(CAMERA& c)
	{
		this->cam = c.cam;
	}
};
// Used for:
// - building objects
// - particles
// - particle effects
// - scaling / building / fading effects on objects
class VOXEL
{
private:
	LPDIRECT3DVERTEXBUFFER9 v_buffer = NULL;    // the pointer to the vertex buffer
	LPDIRECT3DINDEXBUFFER9 i_buffer = NULL;    // the pointer to the index buffer
	D3DMATERIAL9 material;    // create the material struct
	SHADEDVERTEX vertices[24];
	short indices[36] =
	{
		0, 1, 2,    // side 1
		2, 1, 3,
		4, 5, 6,    // side 2
		6, 5, 7,
		8, 9, 10,    // side 3
		10, 9, 11,
		12, 13, 14,    // side 4
		14, 13, 15,
		16, 17, 18,    // side 5
		18, 17, 19,
		20, 21, 22,    // side 6
		22, 21, 23,
	};


public:
	VOXEL() : VOXEL(0.0, 0.0, 0.0) {}
	VOXEL(FLOAT Xorigin, FLOAT Yorigin, FLOAT Zorigin, D3DXCOLOR color = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f))
	{
		vertices[0] =  { -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f, };    // side 1
		vertices[1] =  {  0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f, };
		vertices[2] =  { -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f, };
		vertices[3] =  {  0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f, };
		vertices[4] =  { -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f, };    // side 2
		vertices[5] =  { -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, };
		vertices[6] =  {  0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f, };
		vertices[7] =  {  0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, };
		vertices[8] =  { -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f, };    // side 3
		vertices[9] =  { -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f, };
		vertices[10] = {  0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f, };
		vertices[11] = {  0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f, };
		vertices[12] = { -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f, };    // side 4
		vertices[13] = {  0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f, };
		vertices[14] = { -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f, };
		vertices[15] = {  0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f, };
		vertices[16] = {  0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f, };    // side 5
		vertices[17] = {  0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f, };
		vertices[18] = {  0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f, };
		vertices[19] = {  0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f, };
		vertices[20] = { -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f, };    // side 6
		vertices[21] = { -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f, };
		vertices[22] = { -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f, };
		vertices[33] = { -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f, };

		ZeroMemory(&material, sizeof(D3DMATERIAL9));    // clear out the struct for use
		material.Diffuse = color;    // set diffuse color
		material.Ambient = color;    // set ambient color

		move(Xorigin, Yorigin, Zorigin);
		rotate(0, 180, 0);

		// create a vertex buffer interface called v_buffer
		d3ddev->CreateVertexBuffer(24 * sizeof(SHADEDVERTEX),
			0,
			CUSTOMFVF,
			D3DPOOL_MANAGED,
			&v_buffer,
			NULL);

		// create a index buffer interface called i_buffer
		d3ddev->CreateIndexBuffer(36 * sizeof(short),
			0,
			D3DFMT_INDEX16,
			D3DPOOL_MANAGED,
			&i_buffer,
			NULL);
	}
	VOXEL(FLOAT Xorigin, FLOAT Yorigin, FLOAT Zorigin, FLOAT Xscale, FLOAT Yscale, FLOAT Zscale, D3DXCOLOR color = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f))
	{
		vertices[0] =  {-(0.5f * Xscale), -(0.5f * Yscale),  (0.5f * Zscale),  0.0f,  0.0f,  1.0f, };    // FRONT
		vertices[1] =  { (0.5f * Xscale), -(0.5f * Yscale),  (0.5f * Zscale),  0.0f,  0.0f,  1.0f, };
		vertices[2] =  {-(0.5f * Xscale),  (0.5f * Yscale),  (0.5f * Zscale),  0.0f,  0.0f,  1.0f, };
		vertices[3] =  { (0.5f * Xscale),  (0.5f * Yscale),  (0.5f * Zscale),  0.0f,  0.0f,  1.0f, };
		vertices[4] =  {-(0.5f * Xscale), -(0.5f * Yscale), -(0.5f * Zscale),  0.0f,  0.0f, -1.0f, };    // BACK
		vertices[5] =  { (0.5f * Xscale), -(0.5f * Yscale), -(0.5f * Zscale),  0.0f,  0.0f, -1.0f, };
		vertices[6] =  {-(0.5f * Xscale),  (0.5f * Yscale), -(0.5f * Zscale),  0.0f,  0.0f, -1.0f, };
		vertices[7] =  { (0.5f * Xscale),  (0.5f * Yscale), -(0.5f * Zscale),  0.0f,  0.0f, -1.0f, };
		vertices[8] =  {-(0.5f * Xscale),  (0.5f * Yscale), -(0.5f * Zscale),  0.0f,  1.0f,  0.0f, };    // TOP
		vertices[9] =  {-(0.5f * Xscale),  (0.5f * Yscale),  (0.5f * Zscale),  0.0f,  1.0f,  0.0f, };
		vertices[10] = { (0.5f * Xscale),  (0.5f * Yscale), -(0.5f * Zscale),  0.0f,  1.0f,  0.0f, };
		vertices[11] = { (0.5f * Xscale),  (0.5f * Yscale),  (0.5f * Zscale),  0.0f,  1.0f,  0.0f, };
		vertices[12] = {-(0.5f * Xscale), -(0.5f * Yscale), -(0.5f * Zscale),  0.0f, -1.0f,  0.0f, };    // BOTTOM
		vertices[13] = {-(0.5f * Xscale), -(0.5f * Yscale),  (0.5f * Zscale),  0.0f, -1.0f,  0.0f, };
		vertices[14] = { (0.5f * Xscale), -(0.5f * Yscale), -(0.5f * Zscale),  0.0f, -1.0f,  0.0f, };
		vertices[15] = { (0.5f * Xscale), -(0.5f * Yscale),  (0.5f * Zscale),  0.0f, -1.0f,  0.0f, };
		vertices[16] = { (0.5f * Xscale), -(0.5f * Yscale), -(0.5f * Zscale),  1.0f,  0.0f,  0.0f, };    // LEFT
		vertices[17] = { (0.5f * Xscale),  (0.5f * Yscale), -(0.5f * Zscale),  1.0f,  0.0f,  0.0f, };
		vertices[18] = { (0.5f * Xscale), -(0.5f * Yscale),  (0.5f * Zscale),  1.0f,  0.0f,  0.0f, };
		vertices[19] = { (0.5f * Xscale),  (0.5f * Yscale),  (0.5f * Zscale),  1.0f,  0.0f,  0.0f, };
		vertices[20] = {-(0.5f * Xscale), -(0.5f * Yscale), -(0.5f * Zscale), -1.0f,  0.0f,  0.0f, };    // RIGHT
		vertices[21] = {-(0.5f * Xscale),  (0.5f * Yscale), -(0.5f * Zscale), -1.0f,  0.0f,  0.0f, };
		vertices[22] = {-(0.5f * Xscale), -(0.5f * Yscale),  (0.5f * Zscale), -1.0f,  0.0f,  0.0f, };
		vertices[23] = {-(0.5f * Xscale),  (0.5f * Yscale),  (0.5f * Zscale), -1.0f,  0.0f,  0.0f, };

		ZeroMemory(&material, sizeof(D3DMATERIAL9));    // clear out the struct for use
		material.Diffuse = color;    // set diffuse color
		material.Ambient = color;    // set ambient color

		move(Xorigin*Xscale, Yorigin*Yscale, Zorigin*Zscale);
		rotate(0, 180, 0);

		// create a vertex buffer interface called v_buffer
		d3ddev->CreateVertexBuffer(24 * sizeof(SHADEDVERTEX),
			0,
			CUSTOMFVF,
			D3DPOOL_MANAGED,
			&v_buffer,
			NULL);

		// create a index buffer interface called i_buffer
		d3ddev->CreateIndexBuffer(36 * sizeof(short),
			0,
			D3DFMT_INDEX16,
			D3DPOOL_MANAGED,
			&i_buffer,
			NULL);
	}
	VOXEL(FLOAT Xorigin, FLOAT Yorigin, FLOAT Zorigin, D3DMATERIAL9 mat)
	{
		vertices[0] = { -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f, };    // side 1
		vertices[1] = { 0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f, };
		vertices[2] = { -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f, };
		vertices[3] = { 0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f, };
		vertices[4] = { -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f, };    // side 2
		vertices[5] = { -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, };
		vertices[6] = { 0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f, };
		vertices[7] = { 0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, };
		vertices[8] = { -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f, };    // side 3
		vertices[9] = { -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f, };
		vertices[10] = { 0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f, };
		vertices[11] = { 0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f, };
		vertices[12] = { -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f, };    // side 4
		vertices[13] = { 0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f, };
		vertices[14] = { -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f, };
		vertices[15] = { 0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f, };
		vertices[16] = { 0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f, };    // side 5
		vertices[17] = { 0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f, };
		vertices[18] = { 0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f, };
		vertices[19] = { 0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f, };
		vertices[20] = { -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f, };    // side 6
		vertices[21] = { -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f, };
		vertices[22] = { -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f, };
		vertices[33] = { -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f, };

		material = mat;

		move(Xorigin, Yorigin, Zorigin);
		rotate(0, 180, 0);

		// create a vertex buffer interface called v_buffer
		d3ddev->CreateVertexBuffer(24 * sizeof(SHADEDVERTEX),
			0,
			CUSTOMFVF,
			D3DPOOL_MANAGED,
			&v_buffer,
			NULL);

		// create a index buffer interface called i_buffer
		d3ddev->CreateIndexBuffer(36 * sizeof(short),
			0,
			D3DFMT_INDEX16,
			D3DPOOL_MANAGED,
			&i_buffer,
			NULL);
	}
	VOXEL(FLOAT Xorigin, FLOAT Yorigin, FLOAT Zorigin, FLOAT Xscale, FLOAT Yscale, FLOAT Zscale, D3DMATERIAL9 mat)
	{
		vertices[0] = { -(0.5f * Xscale), -(0.5f * Yscale),  (0.5f * Zscale),  0.0f,  0.0f,  1.0f, };    // FRONT
		vertices[1] = { (0.5f * Xscale), -(0.5f * Yscale),  (0.5f * Zscale),  0.0f,  0.0f,  1.0f, };
		vertices[2] = { -(0.5f * Xscale),  (0.5f * Yscale),  (0.5f * Zscale),  0.0f,  0.0f,  1.0f, };
		vertices[3] = { (0.5f * Xscale),  (0.5f * Yscale),  (0.5f * Zscale),  0.0f,  0.0f,  1.0f, };
		vertices[4] = { -(0.5f * Xscale), -(0.5f * Yscale), -(0.5f * Zscale),  0.0f,  0.0f, -1.0f, };    // BACK
		vertices[5] = { (0.5f * Xscale), -(0.5f * Yscale), -(0.5f * Zscale),  0.0f,  0.0f, -1.0f, };
		vertices[6] = { -(0.5f * Xscale),  (0.5f * Yscale), -(0.5f * Zscale),  0.0f,  0.0f, -1.0f, };
		vertices[7] = { (0.5f * Xscale),  (0.5f * Yscale), -(0.5f * Zscale),  0.0f,  0.0f, -1.0f, };
		vertices[8] = { -(0.5f * Xscale),  (0.5f * Yscale), -(0.5f * Zscale),  0.0f,  1.0f,  0.0f, };    // TOP
		vertices[9] = { -(0.5f * Xscale),  (0.5f * Yscale),  (0.5f * Zscale),  0.0f,  1.0f,  0.0f, };
		vertices[10] = { (0.5f * Xscale),  (0.5f * Yscale), -(0.5f * Zscale),  0.0f,  1.0f,  0.0f, };
		vertices[11] = { (0.5f * Xscale),  (0.5f * Yscale),  (0.5f * Zscale),  0.0f,  1.0f,  0.0f, };
		vertices[12] = { -(0.5f * Xscale), -(0.5f * Yscale), -(0.5f * Zscale),  0.0f, -1.0f,  0.0f, };    // BOTTOM
		vertices[13] = { -(0.5f * Xscale), -(0.5f * Yscale),  (0.5f * Zscale),  0.0f, -1.0f,  0.0f, };
		vertices[14] = { (0.5f * Xscale), -(0.5f * Yscale), -(0.5f * Zscale),  0.0f, -1.0f,  0.0f, };
		vertices[15] = { (0.5f * Xscale), -(0.5f * Yscale),  (0.5f * Zscale),  0.0f, -1.0f,  0.0f, };
		vertices[16] = { (0.5f * Xscale), -(0.5f * Yscale), -(0.5f * Zscale),  1.0f,  0.0f,  0.0f, };    // LEFT
		vertices[17] = { (0.5f * Xscale),  (0.5f * Yscale), -(0.5f * Zscale),  1.0f,  0.0f,  0.0f, };
		vertices[18] = { (0.5f * Xscale), -(0.5f * Yscale),  (0.5f * Zscale),  1.0f,  0.0f,  0.0f, };
		vertices[19] = { (0.5f * Xscale),  (0.5f * Yscale),  (0.5f * Zscale),  1.0f,  0.0f,  0.0f, };
		vertices[20] = { -(0.5f * Xscale), -(0.5f * Yscale), -(0.5f * Zscale), -1.0f,  0.0f,  0.0f, };    // RIGHT
		vertices[21] = { -(0.5f * Xscale),  (0.5f * Yscale), -(0.5f * Zscale), -1.0f,  0.0f,  0.0f, };
		vertices[22] = { -(0.5f * Xscale), -(0.5f * Yscale),  (0.5f * Zscale), -1.0f,  0.0f,  0.0f, };
		vertices[23] = { -(0.5f * Xscale),  (0.5f * Yscale),  (0.5f * Zscale), -1.0f,  0.0f,  0.0f, };

		material = mat;

		move(Xorigin*Xscale, Yorigin*Yscale, Zorigin*Zscale);
		rotate(0, 180, 0);

		// create a vertex buffer interface called v_buffer
		d3ddev->CreateVertexBuffer(24 * sizeof(SHADEDVERTEX),
			0,
			CUSTOMFVF,
			D3DPOOL_MANAGED,
			&v_buffer,
			NULL);

		// create a index buffer interface called i_buffer
		d3ddev->CreateIndexBuffer(36 * sizeof(short),
			0,
			D3DFMT_INDEX16,
			D3DPOOL_MANAGED,
			&i_buffer,
			NULL);
	}
	void move(FLOAT Xmove, FLOAT Ymove, FLOAT Zmove)
	{
		for (int i = 0; i < 24; i++) 
		{ 
			vertices[i].x += Xmove; 
			vertices[i].y += Ymove; 
			vertices[i].z += Zmove; 
		}
	}
	void rotate(FLOAT Xdeg, FLOAT Ydeg, FLOAT Zdeg)
	{
		for (int i = 0; i < 24; i++)
		{
			D3DXMATRIX xRot;
			D3DXMATRIX yRot;
			D3DXMATRIX zRot;
			D3DXMatrixRotationX(&xRot, D3DXToRadian(Xdeg));
			D3DXMatrixRotationY(&yRot, D3DXToRadian(Ydeg));
			D3DXMatrixRotationZ(&zRot, D3DXToRadian(Zdeg));
			D3DXMATRIX result = (xRot * yRot * zRot);

			FLOAT x = vertices[i].x;
			FLOAT y = vertices[i].y;
			FLOAT z = vertices[i].z;

			vertices[i].x = (x * result._11) + (y * result._21) + (z * result._31) + (1 * result._41);
			vertices[i].y = (x * result._12) + (y * result._22) + (z * result._32) + (1 * result._42);
			vertices[i].z = (x * result._13) + (y * result._23) + (z * result._33) + (1 * result._43);
		}
	}
	void draw()
	{
		VOID* pVoid;    // a void pointer

		// lock v_buffer and load the vertices into it
		v_buffer->Lock(0, 0, (void**)&pVoid, 0);
		memcpy(pVoid, vertices, 24 * sizeof(SHADEDVERTEX));
		v_buffer->Unlock();

		// lock i_buffer and load the indices into it
		i_buffer->Lock(0, 0, (void**)&pVoid, 0);
		memcpy(pVoid, indices, 36 * sizeof(short));
		i_buffer->Unlock();

		// select the vertex buffer to display
		d3ddev->SetStreamSource(0, v_buffer, 0, sizeof(SHADEDVERTEX));
		d3ddev->SetIndices(i_buffer);
		d3ddev->SetMaterial(&material);

		// draw the voxel
		d3ddev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 24, 0, 12);
	}
	~VOXEL()
	{
		v_buffer->Release();
		i_buffer->Release();
	}
};
// object being reflected in Direct3D
class Object
{
private:
	static const int SIZE = 1000000;
	std::string object_identifier;
	VOXEL* voxels[SIZE];
	int voxelcount = 0;
	void(*activation)(void);
	void(*collision)(void);

public:
	Object(std::string OID)
	{
		object_identifier = OID;
		memset(voxels, 0, SIZE);
		voxelcount = 0;
		activation = NULL;
		collision = NULL;
	}
	
	Object(std::string OID, FLOAT Xorigin, FLOAT Yorigin, FLOAT Zorigin, D3DXCOLOR color = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f), void(*activationFunction)(void) = NULL, void(*collisionFunction)(void) = NULL)
	{
		object_identifier = OID;
		memset(voxels, 0, SIZE);
		voxels[0] = new VOXEL(Xorigin, Yorigin, Zorigin, color);
		voxelcount = 1;
		activation = activationFunction;
		collision = collisionFunction;
	}

	Object(std::string OID, FLOAT Xorigin, FLOAT Yorigin, FLOAT Zorigin, FLOAT Xscale, FLOAT Yscale, FLOAT Zscale, D3DXCOLOR color = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f), void(*activationFunction)(void) = NULL, void(*collisionFunction)(void) = NULL)
	{
		object_identifier = OID;
		memset(voxels, 0, SIZE);
		voxels[0] = new VOXEL(Xorigin, Yorigin, Zorigin, Xscale, Yscale, Zscale, color);
		voxelcount = 1;
		activation = activationFunction;
		collision = collisionFunction;
	}

	Object(std::string OID, VOXEL voxels[], int size, void(*activationFunction)(void) = NULL, void(*collisionFunction)(void) = NULL)
	{
		object_identifier = OID;
		memset(this->voxels, 0, SIZE);
		for (int i = 0; i < SIZE; i++) this->voxels[i] = &voxels[i];
		voxelcount = size;
		activation = activationFunction;
		collision = collisionFunction;
	}

	void draw()
	{
		for (int i = 0; i < voxelcount; i++) voxels[i]->draw();
	}

	void rotate(FLOAT Xdeg, FLOAT Ydeg, FLOAT Zdeg)
	{
		for (int i = 0; i < voxelcount; i++) voxels[i]->rotate(Xdeg, Ydeg, Zdeg);
	}

	void rotate(FLOAT Xdeg, FLOAT Ydeg, FLOAT Zdeg, int Voxel)
	{
		try
		{
			voxels[Voxel]->rotate(Xdeg, Ydeg, Zdeg);
		}
		catch (...)
		{
			return;
		}
	}

	void move(FLOAT Xmove, FLOAT Ymove, FLOAT Zmove)
	{
		for (int i = 0; i < voxelcount; i++) voxels[i]->move(Xmove, Ymove, Zmove);
	}

	void move(FLOAT Xmove, FLOAT Ymove, FLOAT Zmove, int Voxel)
	{
		if (Voxel < voxelcount) voxels[Voxel]->move(Xmove, Ymove, Zmove);
	}

	void changeColor(DWORD color)
	{

	}

	void changeColor(DWORD color, int Voxel)
	{

	}

	std::string Identifier() { return object_identifier; }

	void addVoxel(VOXEL* voxel)
	{
		if (voxelcount - 1 < SIZE)
		{
			voxels[voxelcount] = voxel;
			voxelcount++;
		}
	}

	void addVoxel(FLOAT Xorigin, FLOAT Yorigin, FLOAT Zorigin, D3DXCOLOR color = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f))
	{
		if (voxelcount - 1 < SIZE)
		{
			voxels[voxelcount] = new VOXEL(Xorigin, Yorigin, Zorigin, color);
			voxelcount++;
		}
	}

	void addVoxel(FLOAT Xorigin, FLOAT Yorigin, FLOAT Zorigin, FLOAT Xscale, FLOAT Yscale, FLOAT Zscale, D3DXCOLOR color = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f))
	{
		if (voxelcount - 1 < SIZE)
		{
			voxels[voxelcount] = new VOXEL(Xorigin, Yorigin, Zorigin, Xscale,Yscale, Zscale, color);
			voxelcount++;
		}
	}

	Object* operator=(const Object& o)
	{
		voxelcount = o.voxelcount;
		for (int i = 0; i < voxelcount; i++)
		{
			voxels[i] = o.voxels[i];
		}
		activation = o.activation;
		collision = o.collision;
		return this;
	}

/*
	void addVoxels(VOXEL voxels[]);
	void addVoxels(DLL* voxels);
	void addConnectedVoxel(VOXEL* voxel);
	bool isColliding(bool start);
	bool isActivated(bool start);
	bool isActivatable();
	int VoxelCount();
	DLL* getVoxelsAsDLL();
	VOXEL* getVoxelsAsArray();
	*/
};
class Scene 
{
private:
	int objectcount;
	Object* objects[1000000];
public:
	Scene()
	{
		objectcount = 0;
		memset(objects, 0, 100000);
	}
	void add(Object* o)
	{
		bool exists = FALSE;
		for (int i = 0; i < objectcount; i++)
		{
			if (objects[i]->Identifier()._Equal(o->Identifier()))
			{
				exists = TRUE;
				break;
			}
		}

		if (objectcount < 999999 && !exists)
		{
			objects[objectcount] = o;
			objectcount += 1;
		}
	}
	void draw()
	{
		int i = 0;
		while (i < objectcount)
		{
			objects[i]->draw();
			i++;
		}
	}

	// rotate single object in Scene
	// - FLOAT Xdeg, Ydeg, Zdeg : rotation degrees per axis
	// - std::string OID : object_identifier string of Object to be rotated
	// - int Voxel : specifies which Voxel should be rotated ( negative Values = whole object)  
	void rotate(FLOAT Xdeg, FLOAT Ydeg, FLOAT Zdeg, std::string OID, int Voxel = -1)
	{
		for (int i = 0; i < objectcount; i++)
		{
			if (objects[i]->Identifier()._Equal(OID))
			{
				if (Voxel <= -1) objects[i]->rotate(Xdeg, Ydeg, Zdeg);
				else objects[i]->rotate(Xdeg, Ydeg, Zdeg, Voxel);
				break;
			}
		}
	}

	Object** getObjects() { return objects; }
	int ObjectCount() { return objectcount; }
};

// kinetic body representing the player
class Player
{
private:
	std::string uname;		// players user name
	Object* model;			// player model
	CAMERA* camera;			// first/third person camera bound to player
};

class Sound 
{
private:
	std::thread* sp = NULL;
	int resource = 0;	

	static void playSound(int resource)
	{
		*running = TRUE;
		PlaySound(MAKEINTRESOURCE(resource), NULL, SND_RESOURCE);
		*running = FALSE;
	}

public:

	void Start()
	{
		sp = new std::thread(&playSound, resource);
	}
	
	void Stop()
	{
		sp->detach();
		sp->~thread();
	}

	void reset(int resource, bool startOnInit = FALSE)
	{
		this->resource = resource;
		if (startOnInit) Start();
	}

	Sound(int resource, bool startOnInit = FALSE)
	{
		reset(resource, startOnInit);
	}
};

Sound* sound = NULL;
// later switch to player
CAMERA* player = NULL;


// generates 3D Voxel Text
// Params:
// - Object* pOut: pointer to Object that represents the text in Direct3D
// - char* text: text string
// - FLOAT up_left_start: defines position of the up voxel on the start of the text
// - FLOAT scale: defines the scale of the text 
// - !!!Scale influences the Position, so thinking for up_left_start is adviced!! 
// - !! All coordinates are stored in Direct3D cartesian system, not the
// - default computer system !!
void genVoxelText(Object** pOut, const char* text, int size, D3DXCOLOR color = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f), D3DXVECTOR3 x_y_z_start = D3DXVECTOR3(0.0f, 0.0f, 0.0f), FLOAT scale = 1.0f);

struct NO_PUBLISHER
{
private:
	Object no_pub = Object("no_pub");
	Object presents = Object("presents");

public:
	NO_PUBLISHER()
	{
		// NO_PUB writing cursive
		{
			// N
			no_pub.addVoxel(-20, 0.0f, 0.0f, 2.0f, 2.0f, 2.0f, D3DCOLOR_XRGB(255, 25, 25));
			no_pub.addVoxel(-19.5, 1.0f, 0.0f, 2.0f, 2.0f, 2.0f, D3DCOLOR_XRGB(255, 25, 25));
			no_pub.addVoxel(-19, 2.0f, 0.0f, 2.0f, 2.0f, 2.0f, D3DCOLOR_XRGB(255, 25, 25));
			no_pub.addVoxel(-18.5, 3.0f, 0.0f, 2.0f, 2.0f, 2.0f, D3DCOLOR_XRGB(255, 25, 25));
			no_pub.addVoxel(-18, 4.0f, 0.0f, 2.0f, 2.0f, 2.0f, D3DCOLOR_XRGB(255, 25, 25));
			no_pub.addVoxel(-17.5, 5.0f, 0.0f, 2.0f, 2.0f, 2.0f, D3DCOLOR_XRGB(255, 25, 25));
			no_pub.addVoxel(-17, 4.0f, 0.0f, 2.0f, 2.0f, 2.0f, D3DCOLOR_XRGB(255, 25, 25));
			no_pub.addVoxel(-16.5, 3.0f, 0.0f, 2.0f, 2.0f, 2.0f, D3DCOLOR_XRGB(255, 25, 25));
			no_pub.addVoxel(-16, 2.0f, 0.0f, 2.0f, 2.0f, 2.0f, D3DCOLOR_XRGB(255, 25, 25));
			no_pub.addVoxel(-16, 0.0f, 0.0f, 2.0f, 2.0f, 2.0f, D3DCOLOR_XRGB(255, 25, 25));
			no_pub.addVoxel(-15.5, 1.0f, 0.0f, 2.0f, 2.0f, 2.0f, D3DCOLOR_XRGB(255, 25, 25));
			no_pub.addVoxel(-15, 2.0f, 0.0f, 2.0f, 2.0f, 2.0f, D3DCOLOR_XRGB(255, 25, 25));
			no_pub.addVoxel(-14.5, 3.0f, 0.0f, 2.0f, 2.0f, 2.0f, D3DCOLOR_XRGB(255, 25, 25));
			no_pub.addVoxel(-14, 4.0f, 0.0f, 2.0f, 2.0f, 2.0f, D3DCOLOR_XRGB(255, 25, 25));
			no_pub.addVoxel(-13.5, 5.0f, 0.0f, 2.0f, 2.0f, 2.0f, D3DCOLOR_XRGB(255, 25, 25));

			// O
			no_pub.addVoxel(-13.5, 1.0f, 0.0f, 2.0f, 2.0f, 2.0f, D3DCOLOR_XRGB(255, 25, 25));
			no_pub.addVoxel(-13, 2.0f, 0.0f, 2.0f, 2.0f, 2.0f, D3DCOLOR_XRGB(255, 25, 25));
			no_pub.addVoxel(-12.5, 3.0f, 0.0f, 2.0f, 2.0f, 2.0f, D3DCOLOR_XRGB(255, 25, 25));
			no_pub.addVoxel(-12, 4.0f, 0.0f, 2.0f, 2.0f, 2.0f, D3DCOLOR_XRGB(255, 25, 25));
			no_pub.addVoxel(-11, 5.0f, 0.0f, 2.0f, 2.0f, 2.0f, D3DCOLOR_XRGB(255, 25, 25));
			no_pub.addVoxel(-10, 5.0f, 0.0f, 2.0f, 2.0f, 2.0f, D3DCOLOR_XRGB(255, 25, 25));
			no_pub.addVoxel(-12.5, 0.0f, 0.0f, 2.0f, 2.0f, 2.0f, D3DCOLOR_XRGB(255, 25, 25));
			no_pub.addVoxel(-11.5, 0.0f, 0.0f, 2.0f, 2.0f, 2.0f, D3DCOLOR_XRGB(255, 25, 25));
			no_pub.addVoxel(-10.5, 1.0f, 0.0f, 2.0f, 2.0f, 2.0f, D3DCOLOR_XRGB(255, 25, 25));
			no_pub.addVoxel(-10, 2.0f, 0.0f, 2.0f, 2.0f, 2.0f, D3DCOLOR_XRGB(255, 25, 25));
			no_pub.addVoxel(-9.5, 3.0f, 0.0f, 2.0f, 2.0f, 2.0f, D3DCOLOR_XRGB(255, 25, 25));
			no_pub.addVoxel(-9, 4.0f, 0.0f, 2.0f, 2.0f, 2.0f, D3DCOLOR_XRGB(255, 25, 25));

			// _
			no_pub.addVoxel(-10.5, -1.0f, 0.0f, 2.0f, 2.0f, 2.0f, D3DCOLOR_XRGB(255, 25, 25));
			no_pub.addVoxel(-9.5, -1.0f, 0.0f, 2.0f, 2.0f, 2.0f, D3DCOLOR_XRGB(255, 25, 25));
			no_pub.addVoxel(-8.5, -1.0f, 0.0f, 2.0f, 2.0f, 2.0f, D3DCOLOR_XRGB(255, 25, 25));

			// P
			no_pub.addVoxel(-7.5, 0.0f, 0.0f, 2.0f, 2.0f, 2.0f, D3DCOLOR_XRGB(255, 25, 25));
			no_pub.addVoxel(-7, 1.0f, 0.0f, 2.0f, 2.0f, 2.0f, D3DCOLOR_XRGB(255, 25, 25));
			no_pub.addVoxel(-6.5, 2.0f, 0.0f, 2.0f, 2.0f, 2.0f, D3DCOLOR_XRGB(255, 25, 25));
			no_pub.addVoxel(-6, 3.0f, 0.0f, 2.0f, 2.0f, 2.0f, D3DCOLOR_XRGB(255, 25, 25));
			no_pub.addVoxel(-5.5, 4.0f, 0.0f, 2.0f, 2.0f, 2.0f, D3DCOLOR_XRGB(255, 25, 25));
			no_pub.addVoxel(-5, 5.0f, 0.0f, 2.0f, 2.0f, 2.0f, D3DCOLOR_XRGB(255, 25, 25));

			no_pub.addVoxel(-4.0, 5.0f, 0.0f, 2.0f, 2.0f, 2.0f, D3DCOLOR_XRGB(255, 25, 25));
			no_pub.addVoxel(-3.0, 4.0f, 0.0f, 2.0f, 2.0f, 2.0f, D3DCOLOR_XRGB(255, 25, 25));
			no_pub.addVoxel(-3.5, 3.0f, 0.0f, 2.0f, 2.0f, 2.0f, D3DCOLOR_XRGB(255, 25, 25));
			no_pub.addVoxel(-4.5, 2.0f, 0.0f, 2.0f, 2.0f, 2.0f, D3DCOLOR_XRGB(255, 25, 25));
			no_pub.addVoxel(-5, 2.0f, 0.0f, 2.0f, 2.0f, 2.0f, D3DCOLOR_XRGB(255, 25, 25));

			// U
			no_pub.addVoxel(-2.5, 1.0f, 0.0f, 2.0f, 2.0f, 2.0f, D3DCOLOR_XRGB(255, 25, 25));
			no_pub.addVoxel(-2, 2.0f, 0.0f, 2.0f, 2.0f, 2.0f, D3DCOLOR_XRGB(255, 25, 25));
			no_pub.addVoxel(-1.5, 3.0f, 0.0f, 2.0f, 2.0f, 2.0f, D3DCOLOR_XRGB(255, 25, 25));
			no_pub.addVoxel(-1, 4.0f, 0.0f, 2.0f, 2.0f, 2.0f, D3DCOLOR_XRGB(255, 25, 25));
			no_pub.addVoxel(-0.5, 5.0f, 0.0f, 2.0f, 2.0f, 2.0f, D3DCOLOR_XRGB(255, 25, 25));
			no_pub.addVoxel(-1.5, 0.0f, 0.0f, 2.0f, 2.0f, 2.0f, D3DCOLOR_XRGB(255, 25, 25));
			no_pub.addVoxel(-0.5, 0.0f, 0.0f, 2.0f, 2.0f, 2.0f, D3DCOLOR_XRGB(255, 25, 25));
			no_pub.addVoxel(0.5, 1.0f, 0.0f, 2.0f, 2.0f, 2.0f, D3DCOLOR_XRGB(255, 25, 25));
			no_pub.addVoxel(1, 2.0f, 0.0f, 2.0f, 2.0f, 2.0f, D3DCOLOR_XRGB(255, 25, 25));
			no_pub.addVoxel(1.5, 3.0f, 0.0f, 2.0f, 2.0f, 2.0f, D3DCOLOR_XRGB(255, 25, 25));
			no_pub.addVoxel(2, 4.0f, 0.0f, 2.0f, 2.0f, 2.0f, D3DCOLOR_XRGB(255, 25, 25));
			no_pub.addVoxel(2.5, 5.0f, 0.0f, 2.0f, 2.0f, 2.0f, D3DCOLOR_XRGB(255, 25, 25));

			// B
			no_pub.addVoxel(3.5, 0.0, 0.0, 2.0, 2.0, 2.0, D3DCOLOR_XRGB(255, 25, 25));
			no_pub.addVoxel(4.0, 1.0, 0.0, 2.0, 2.0, 2.0, D3DCOLOR_XRGB(255, 25, 25));
			no_pub.addVoxel(4.5, 2.0, 0.0, 2.0, 2.0, 2.0, D3DCOLOR_XRGB(255, 25, 25));
			no_pub.addVoxel(5.0, 3.0, 0.0, 2.0, 2.0, 2.0, D3DCOLOR_XRGB(255, 25, 25));
			no_pub.addVoxel(5.5, 4.0, 0.0, 2.0, 2.0, 2.0, D3DCOLOR_XRGB(255, 25, 25));
			no_pub.addVoxel(6.0, 5.0, 0.0, 2.0, 2.0, 2.0, D3DCOLOR_XRGB(255, 25, 25));
			no_pub.addVoxel(7.0, 5.0, 0.0, 2.0, 2.0, 2.0, D3DCOLOR_XRGB(255, 25, 25));
			no_pub.addVoxel(5.5, 2.5, 0.0, 2.0, 2.0, 2.0, D3DCOLOR_XRGB(255, 25, 25));
			no_pub.addVoxel(2.8, 0.0, 0.0, 3.0, 2.0, 2.0, D3DCOLOR_XRGB(255, 25, 25));
			VOXEL* v1 = new VOXEL(0, 0, 0, 1.0, 4.0, 2.0, D3DCOLOR_XRGB(255, 25, 25));
			VOXEL* v2 = new VOXEL(0, 0, 0, 1.0, 4.0, 2.0, D3DCOLOR_XRGB(255, 25, 25));
			v1->rotate(0, 0, 30);
			v2->rotate(0, 0, 25);
			v1->move(14.5, 7, -1);
			v2->move(12, 2, -1);
			no_pub.addVoxel(v1);
			no_pub.addVoxel(v2);

			no_pub.move(4.0f, -3.0f, -25.0f);
		};

		// PRESENTS writing no-cursive
		{
			// P
			presents.addVoxel(0.0, -5.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(0.0, -6.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(0.0, -7.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(0.0, -8.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(0.0, -9.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(0.0, -10.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(1.0, -5.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(2.0, -6.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(2.0, -7.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(1.0, -8.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));

			// R
			presents.addVoxel(4.0, -5.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(4.0, -6.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(4.0, -7.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(4.0, -8.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(4.0, -9.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(4.0, -10.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(5.0, -5.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(6.0, -6.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(6.0, -7.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(5.0, -8.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(6.0, -9.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(6.0, -10.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));

			// E
			presents.addVoxel(8.0, -5.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(8.0, -6.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(8.0, -7.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(8.0, -8.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(8.0, -9.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(8.0, -10.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(9.0, -5.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(9.0, -7.5f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(9.0, -10.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));

			// S
			presents.addVoxel(11.0, -5.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(12.0, -5.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(13.0, -5.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(11.0, -6.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(11.0, -7.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(11.0, -7.5f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(12.0, -7.5f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(13.0, -7.5f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(13.0, -8.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(13.0, -9.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(11.0, -10.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(12.0, -10.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(13.0, -10.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));

			// E
			presents.addVoxel(15.0, -5.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(15.0, -6.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(15.0, -7.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(15.0, -8.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(15.0, -9.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(15.0, -10.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(16.0, -5.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(16.0, -7.5f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(16.0, -10.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));

			// N
			presents.addVoxel(18.0, -5.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(18.0, -6.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(18.0, -7.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(18.0, -8.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(18.0, -9.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(18.0, -10.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));

			presents.addVoxel(19.0, -6.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(19.5, -7.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(20.0, -8.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(20.5, -9.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));

			presents.addVoxel(21.0, -5.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(21.0, -6.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(21.0, -7.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(21.0, -8.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(21.0, -9.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(21.0, -10.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));

			// T
			presents.addVoxel(23.0, -5.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(24.0, -5.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(25.0, -5.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(26.0, -5.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(27.0, -5.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));


			presents.addVoxel(25.0, -5.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(25.0, -6.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(25.0, -7.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(25.0, -8.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(25.0, -9.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(25.0, -10.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));

			// S
			presents.addVoxel(29.0, -5.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(30.0, -5.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(31.0, -5.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(29.0, -6.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(29.0, -7.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(29.0, -7.5f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(30.0, -7.5f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(31.0, -7.5f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(31.0, -8.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(31.0, -9.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(29.0, -10.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(30.0, -10.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));
			presents.addVoxel(31.0, -10.0f, 0.0f, 0.25, 0.25, 0.25, D3DCOLOR_XRGB(255, 25, 25));


			// move to desired location
			presents.move(0, -1, 0);
		};
	}

	void draw()
	{
		no_pub.draw();
		presents.draw();
	}
};

struct RELAY_OF_INTEGRITY
{
private:
	Object* relay = new Object("relay");
public:
	RELAY_OF_INTEGRITY()
	{
		genVoxelText(&relay, "relay\nof\nintegrity", 19, D3DXCOLOR(1.0f, 0.5f, 0.2f, 1.0f), {-25.0f, 13.0f, 0.0});
		relay->rotate(0.0f, -20.0f, 0.0f);
	}

	void draw()
	{
		relay->draw();
	}
};


// function prototypes
void initD3D(HWND hWnd);    // sets up and initializes Direct3D
void render_frame(void);    // renders a single frame
void cleanD3D(void);		// closes Direct3D and releases memory
void init_graphics(void);	// 3D declarations
void intro(void);
void init_light(void);	// sets up lighting
