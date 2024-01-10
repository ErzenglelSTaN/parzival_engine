#pragma once
#ifdef DSTOPIA_EXPORTS
#define DSTOPIA_API __declspec(dllexport)
#else
#define DSTOPIA_API __declspec(dllimport)
#endif

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

#ifdef PC //Big-Endian
#define fourccRIFF 'RIFF'
#define fourccDATA 'data'
#define fourccFMT 'fmt '
#define fourccWAVE 'WAVE'
#define fourccXWMA 'XWMA'
#define fourccDPDS 'dpds'
#endif

#ifndef PC //Little-Endian
#define fourccRIFF 'FFIR'
#define fourccDATA 'atad'
#define fourccFMT ' tmf'
#define fourccWAVE 'EVAW'
#define fourccXWMA 'AMWX'
#define fourccDPDS 'sdpd'
#endif

// include the Direct3D Library files
#pragma comment (lib, "d3d9.lib")
#pragma comment (lib, "d3dx9.lib")

#define BG_COLOR D3DCOLOR_XRGB(0, 0, 0)
#define CUSTOMFVF (D3DFVF_XYZ | D3DFVF_NORMAL)

struct CONFIG
{
	bool windowed;
	D3DXVECTOR2 dimensions;
	//texture load;
};

struct VERTEX
{
	FLOAT x, y, z;
	D3DVECTOR normal;

	VERTEX() : VERTEX(0.0f, 0.0f, 0.0f, { 0.0f, 0.0f, 0.0f }) {};
	VERTEX(FLOAT X, FLOAT Y, FLOAT Z, FLOAT NX, FLOAT NY, FLOAT NZ) : VERTEX(X, Y, Z, { NX, NY, NZ }) {};
	VERTEX(FLOAT X, FLOAT Y, FLOAT Z, D3DVECTOR NORMAL)
	{
		x = X;
		y = Y;
		z = Z;
		normal = NORMAL;
	};
};

class EVIEOBJECT
{
private:
	LPDIRECT3DVERTEXBUFFER9 v_buffer = NULL;    // the pointer to the vertex buffer
	LPDIRECT3DINDEXBUFFER9 i_buffer = NULL;    // the pointer to the index buffer
	std::vector<VERTEX> vertices;
	std::vector<short> indices;
	ID3DXMesh* mesh;
	DWORD numMats;
	D3DMATERIAL9* meshMaterials;
	LPDIRECT3DTEXTURE9* meshTextures;

	//rotation stores
	D3DXMATRIX trans;
	D3DXMATRIX rotX;
	D3DXMATRIX rotY;
	D3DXMATRIX rotZ;

	D3DXMATRIX reset;

public:
	DSTOPIA_API EVIEOBJECT(const char* filename, LPDIRECT3DDEVICE9 pDevice);

	// translate methods
	//void moveX(float offset);
	//void moveY(float offset);
	//void moveZ(float offset);
	DSTOPIA_API void move(float xOffset, float yOffset, float zOffset);

	// rotation around world origin methods
	//void rotX(float deg);
	//void rotX(float deg);
	//void rotX(float deg);
	DSTOPIA_API void rot(float xDeg, float yDeg, float zDeg);

	// rotation around perpenticular axis methods

	// transform world reset Matrix
	DSTOPIA_API void setResetMartix(D3DXMATRIX& source);

	DSTOPIA_API void draw(LPDIRECT3DDEVICE9 pDevice);
	template<typename T>
	DSTOPIA_API T* convert(std::vector<T> vector);
	DSTOPIA_API ~EVIEOBJECT();
};

class Scene
{
private:
	// scene "search & keep" properties
	int id;
	char* name;
	char* desc;

	EVIEOBJECT** objects;

	DSTOPIA_API Scene();
public:
	// Factory function creating heap object //
	DSTOPIA_API Scene* CreateScene();

	// Factory function creating stack object //
	DSTOPIA_API Scene CreateSceneDirect();

	// Creates a new EVIEOBJECT into scene directly //
	// ret : int = index where object was placed //
	DSTOPIA_API int CreateObject(const char* filepath);

	// Adds existing preconfigured object to scene //
	// ret : int = index where object was placed //
	DSTOPIA_API int AddObject(EVIEOBJECT* object);

	// Develops a binding between two existing objects //
	// ret : bool = TRUE if binding was created, FALSE if binding exists or not possible //
	DSTOPIA_API bool DevelopBinding(int baseObject, int subObject);


};

class DstopiaDriver
{
private:
	LPDIRECT3D9 d3d;									// the pointer to the Direct3D interface
	LPDIRECT3DDEVICE9 d3ddev;						// the pointer to the device class
	LPDIRECT3DVERTEXBUFFER9 global_v_buffer = NULL;	// the pointer to the vertex buffer
	LPDIRECT3DINDEXBUFFER9 global_i_buffer = NULL;	// the pointer to the index buffer
	ID3DXFont* font = NULL;


public:
	DSTOPIA_API LPDIRECT3D9 AccessCore();
	DSTOPIA_API LPDIRECT3DDEVICE9 AccessDevice();

	DSTOPIA_API void initD3D(HWND hWnd, CONFIG conf);    // sets up and initializes Direct3D
	DSTOPIA_API void cleanD3D(void);		// closes Direct3D and releases memory
	DSTOPIA_API void init_graphics(void);	// 3D declarations
	DSTOPIA_API void init_lighting(void);	// sets up lighting
	DSTOPIA_API IDirect3DSurface9* LoadSurfaceFromFile(std::string FileName);
	
};

/* // NEEDS some change
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
*/

extern "C++" DSTOPIA_API void read_conf(void);
extern "C++" DSTOPIA_API void get_surface_normals(std::vector<VERTEX> vertices, std::vector<short> indices);