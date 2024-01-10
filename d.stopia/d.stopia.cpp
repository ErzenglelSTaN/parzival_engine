#pragma once
#include "pch.h"
#include "d.stopia.hpp"

void read_conf(void)
{
	return;
}

void get_surface_normals(std::vector<VERTEX> vertices, std::vector<short> indices)
{
	// Three-Of-A-Kind VERTEX pair
	VERTEX P1;
	VERTEX P2;
	VERTEX P3;

	for (int i = 0; i < vertices.size(); i += 3)
	{
		// get vertices contained in triangle
		P1 = vertices[indices[i]];
		P2 = vertices[indices[i + 1]];
		P3 = vertices[indices[i + 2]];

		// calculate cross products
		D3DXVECTOR3 A = D3DXVECTOR3(P2.x - P1.x, P2.y - P1.y, P2.z - P1.y);
		D3DXVECTOR3 B = D3DXVECTOR3(P3.x - P1.x, P3.y - P1.y, P3.z - P1.y);

		D3DXVECTOR3 n = D3DXVECTOR3((A.y * B.z - A.z * B.y), (A.z * B.x - A.x * B.z), (A.x * B.y - A.y * B.x));

		float vector_length = sqrt(n.x * n.x + n.y * n.y + n.z * n.z);

		D3DXVECTOR3 n_normalized = D3DXVECTOR3(n.x / vector_length, n.y / vector_length, n.z / vector_length);

		P1.normal = n_normalized;
		P2.normal = n_normalized;
		P3.normal = n_normalized;
	}
}

// ########################## //
// EVIEOBJECT implementations //
// ########################## //
EVIEOBJECT::EVIEOBJECT(const char* filename, LPDIRECT3DDEVICE9 pDevice)
{

	// set transform matrices to default
	D3DXMatrixIdentity(&rotX);
	D3DXMatrixIdentity(&rotY);
	D3DXMatrixIdentity(&rotZ);
	D3DXMatrixIdentity(&trans);
	D3DXMatrixIdentity(&reset);

	LPD3DXBUFFER materials;

	D3DXLoadMeshFromX(LPCWSTR(filename), D3DXMESH_SYSTEMMEM, pDevice, NULL, &materials, NULL, &numMats, &mesh);

	D3DXMATERIAL* D3DMaterials = (D3DXMATERIAL*)materials->GetBufferPointer();
	meshMaterials = new D3DMATERIAL9[numMats];
	meshTextures = new LPDIRECT3DTEXTURE9[numMats];

	for (DWORD i = 0; i < numMats; i++)
	{
		meshMaterials[i] = D3DMaterials[i].MatD3D;
		meshMaterials[i].Ambient = meshMaterials[i].Diffuse;

		meshTextures[i] = NULL;
		if (D3DMaterials[i].pTextureFilename) D3DXCreateTextureFromFile(pDevice, LPCWSTR(D3DMaterials[i].pTextureFilename), &meshTextures[i]);
	}

	materials->Release();
}

void EVIEOBJECT::draw(LPDIRECT3DDEVICE9 pDevice)
{
	pDevice->SetTransform(D3DTS_WORLD, &(rotX * rotY * rotZ * trans));
	for (DWORD i = 0; i < numMats; i++)
	{
		pDevice->SetMaterial(&meshMaterials[i]);
		pDevice->SetTexture(0, meshTextures[i]);

		mesh->DrawSubset(i);
	}

	pDevice->SetTransform(D3DTS_WORLD, &reset);
}

void EVIEOBJECT::move(float xOffset, float yOffset, float zOffset)
{
	D3DXMatrixTranslation(&trans, xOffset, yOffset, zOffset);
}

void EVIEOBJECT::rot(float xDeg, float yDeg, float zDeg)
{
	D3DXMatrixRotationX(&rotX, xDeg);
	D3DXMatrixRotationY(&rotY, yDeg);
	D3DXMatrixRotationZ(&rotZ, zDeg);
}

void EVIEOBJECT::setResetMartix(D3DXMATRIX& source)
{
	memcpy(&reset, source, sizeof(source));
}

template<typename T>
T* EVIEOBJECT::convert(std::vector<T> vector)
{
}

EVIEOBJECT::~EVIEOBJECT()
{
	v_buffer->Release();
	i_buffer->Release();
}

// ######################## //
// DstopiaDriver implement //
// ######################## //

DSTOPIA_API LPDIRECT3D9 DstopiaDriver::AccessCore()
{
	return this->d3d;
}

DSTOPIA_API LPDIRECT3DDEVICE9 DstopiaDriver::AccessDevice()
{
	return this->d3ddev;
}

// this function initializes and prepares Direct3D for use
DSTOPIA_API void DstopiaDriver::initD3D(HWND hWnd, CONFIG conf)
{
	d3d = Direct3DCreate9(D3D_SDK_VERSION);

	D3DPRESENT_PARAMETERS d3dpp;

	ZeroMemory(&d3dpp, sizeof(d3dpp));
	d3dpp.Windowed = conf.windowed;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.hDeviceWindow = hWnd;
	d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
	d3dpp.BackBufferWidth = (UINT)conf.dimensions.x;
	d3dpp.BackBufferHeight = (UINT)conf.dimensions.y;
	d3dpp.EnableAutoDepthStencil = TRUE;
	d3dpp.AutoDepthStencilFormat = D3DFMT_D16;

	d3d->CreateDevice(D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		hWnd,
		D3DCREATE_SOFTWARE_VERTEXPROCESSING,
		&d3dpp,
		&d3ddev);

	init_graphics();
	init_lighting();

	d3ddev->SetRenderState(D3DRS_LIGHTING, TRUE);    // turn off the 3D lighting
	d3ddev->SetRenderState(D3DRS_AMBIENT, D3DCOLOR_XRGB(50, 50, 50));	//ever existent ambient color enable
	d3ddev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);    // turn off culling
	d3ddev->SetRenderState(D3DRS_ZENABLE, TRUE);    // turn on the z-buffer
}

// this is the function that cleans up Direct3D and COM
DSTOPIA_API void DstopiaDriver::cleanD3D(void)
{
	global_v_buffer->Release();
	global_i_buffer->Release();
	d3ddev->Release();
	d3d->Release();

	ClipCursor(NULL);
}

// this is the function that puts the 3D models into video RAM
DSTOPIA_API void DstopiaDriver::init_graphics(void)
{
	// create a vertex buffer interface called v_buffer
	d3ddev->CreateVertexBuffer(24 * sizeof(VERTEX),
		0,
		CUSTOMFVF,
		D3DPOOL_MANAGED,
		&global_v_buffer,
		NULL);

	// create a index buffer interface called i_buffer
	d3ddev->CreateIndexBuffer(36 * sizeof(short),
		0,
		D3DFMT_INDEX16,
		D3DPOOL_MANAGED,
		&global_i_buffer,
		NULL);
}

DSTOPIA_API void DstopiaDriver::init_lighting(void)
{
	D3DLIGHT9 light;    // create the light struct
	D3DMATERIAL9 material;    // create the material struct

	ZeroMemory(&light, sizeof(light));    // clear out the light struct for use
	light.Type = D3DLIGHT_DIRECTIONAL;    // make the light type 'directional light'
	light.Diffuse = D3DXCOLOR(0.5f, 0.5f, 0.5f, 1.0f);    // set the light's color
	light.Direction = D3DXVECTOR3(-1.0f, -0.3f, -1.0f);

	d3ddev->SetLight(0, &light);    // send the light struct properties to light #0
	d3ddev->LightEnable(0, TRUE);    // turn on light #0

	ZeroMemory(&material, sizeof(D3DMATERIAL9));    // clear out the struct for use
	material.Diffuse = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);    // set diffuse color to white
	material.Ambient = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);    // set ambient color to white

	d3ddev->SetMaterial(&material);    // set the globably-used material to &material
}

DSTOPIA_API IDirect3DSurface9* DstopiaDriver::LoadSurfaceFromFile(std::string FileName)
{
	HRESULT hResult;
	IDirect3DSurface9* surface = NULL;
	D3DXIMAGE_INFO imageInfo;
	LPCTSTR str = LPCTSTR(FileName.c_str());//so we get a pointer to a proper string to begin with
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