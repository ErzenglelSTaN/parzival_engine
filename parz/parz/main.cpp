#include "main.hpp"

// the WindowProc function prototype
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

// the entry point for any Windows program
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	HWND hWnd;
	WNDCLASSEX wc;

	RAWINPUTDEVICE rid;
	rid.usUsagePage = 0x01;	//Mouse
	rid.usUsage = 0x02;
	rid.dwFlags = 0;
	rid.hwndTarget = NULL;

	if (RegisterRawInputDevices(&rid, 1, sizeof(rid)) == FALSE) exit(-1);

	ZeroMemory(&wc, sizeof(WNDCLASSEX));

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.lpszClassName = "WindowClass";

	RegisterClassEx(&wc);

	hWnd = CreateWindowEx(NULL, "WindowClass", "PARZIVAL -- TEST",
		WS_OVERLAPPEDWINDOW, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
		NULL, NULL, hInstance, NULL);

	ShowWindow(hWnd, nCmdShow);
	ShowCursor(FALSE);

	// set up and initialize Direct3D
	initD3D(hWnd);

	intro();

	// main loop
	MSG msg;
	while (TRUE)
	{
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if (msg.message == WM_QUIT)
			break;

		render_frame();
	}

	// clean up DirectX and COM
	cleanD3D();
	return msg.wParam;
}

// this is the main message handler for the program
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CLOSE:
	{
		DestroyWindow(hWnd);
	}break;

	case WM_DESTROY:
	{
		PostQuitMessage(0);
		cleanD3D();
		exit(0);
	} break;

	case WM_KEYDOWN:
	{
		if (GET_KEYSTATE_WPARAM(wParam) == VK_BACK)
		{
			*WorldXTranslation = 0;
			*WorldYTranslation = 0;
		}

	} break;

	case WM_INPUT:
	{
		UINT dataSize;

		GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, NULL, &dataSize, sizeof(RAWINPUTHEADER));

		if (dataSize > 0)
		{
			std::unique_ptr<BYTE[]> rawdata = std::make_unique<BYTE[]>(dataSize);
			if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, rawdata.get(), &dataSize, sizeof(RAWINPUTHEADER)) == dataSize)
			{
				RAWINPUT* raw = reinterpret_cast<RAWINPUT*>(rawdata.get());
				if (raw->header.dwType == RIM_TYPEMOUSE && raw->data.mouse.usFlags == MOUSE_MOVE_RELATIVE && player != NULL)
				{
					float x = 0;
					float y = 0;

					if (raw->data.mouse.lLastX == 1) x = -0.25f;
					else if (raw->data.mouse.lLastX == -1) x = 0.25f;

					if (raw->data.mouse.lLastY == 1) y = -0.25f;
					else if (raw->data.mouse.lLastY == -1) y = 0.25f;

					player->look(x, y);
					SetCursorPos(SCREEN_HEIGHT/2, SCREEN_WIDTH/2);
				}
			}
		}

		return DefWindowProc(hWnd, message, wParam, lParam);
	} break;
	};


	return DefWindowProc(hWnd, message, wParam, lParam);
}

// this function initializes and prepares Direct3D for use
void initD3D(HWND hWnd)
{
	d3d = Direct3DCreate9(D3D_SDK_VERSION);

	D3DPRESENT_PARAMETERS d3dpp;

	ZeroMemory(&d3dpp, sizeof(d3dpp));
	d3dpp.Windowed = FALSE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.hDeviceWindow = hWnd;
	d3dpp.BackBufferFormat = D3DFMT_X8R8G8B8;
	d3dpp.BackBufferWidth = SCREEN_WIDTH;
	d3dpp.BackBufferHeight = SCREEN_HEIGHT;
	d3dpp.EnableAutoDepthStencil = TRUE;
	d3dpp.AutoDepthStencilFormat = D3DFMT_D16;

	d3d->CreateDevice(D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		hWnd,
		D3DCREATE_SOFTWARE_VERTEXPROCESSING,
		&d3dpp,
		&d3ddev);

	init_graphics();
	init_light();

	d3ddev->SetRenderState(D3DRS_LIGHTING, TRUE);    // turn off the 3D lighting
	d3ddev->SetRenderState(D3DRS_ZENABLE, TRUE);    // turn on the z-buffer
	d3ddev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	d3ddev->SetRenderState(D3DRS_AMBIENT, D3DCOLOR_XRGB(50, 50, 50));    // ambient light
	d3ddev->SetRenderState(D3DRS_NORMALIZENORMALS, TRUE);

	// Player init
	player = new CAMERA(D3DXVECTOR3(0.0f, 0.0f, 25.0f), D3DXVECTOR3(0.0f, 0.0f, 0.0f), D3DXVECTOR3(0.0f, 1.0f, 0.0f));
}

// this is the function used to render a single frame
void render_frame(void)
{	
	d3ddev->Clear(0, NULL, D3DCLEAR_TARGET, BG_COLOR, 1.0f, 0);
	d3ddev->Clear(0, NULL, D3DCLEAR_ZBUFFER, BG_COLOR, 1.0f, 0);

	d3ddev->BeginScene();

	d3ddev->SetFVF(CUSTOMFVF);

	// set the view transform
	D3DXMATRIX matView;
	D3DXMatrixLookAtLH(&matView,
		&D3DXVECTOR3(0.0f, 0.0f, 25.0f),		// the camera position
		&D3DXVECTOR3(*camX, *camY, *camZ),			// the look-at position
		&D3DXVECTOR3(0.0f, 1.0f, 0.0f));		// the up direction
	d3ddev->SetTransform(D3DTS_VIEW, &matView);

	// set the projection transform
	D3DXMATRIX matProjection;
	D3DXMatrixPerspectiveFovLH(&matProjection,
		D3DXToRadian(90),									// the horizontal field of view
		(FLOAT)SCREEN_WIDTH / (FLOAT)SCREEN_HEIGHT,			// aspect ratio
		1.0f,												// the near view-plane
		100.0f);											// the far view-plane
	d3ddev->SetTransform(D3DTS_PROJECTION, &matProjection); // set the projection

	// complete World transformation matrices
	D3DXMATRIX matRotateX;
	D3DXMATRIX matRotateY;
	D3DXMATRIX matRotateZ;
	D3DXMatrixRotationX(&matRotateX, D3DXToRadian(*WorldXRotation));
	D3DXMatrixRotationY(&matRotateY, D3DXToRadian(*WorldYRotation));
	D3DXMatrixRotationZ(&matRotateZ, D3DXToRadian(*WorldZRotation));
	d3ddev->SetTransform(D3DTS_WORLD, &(matRotateX * matRotateY * matRotateZ));

	// draw objects

	d3ddev->EndScene();

	d3ddev->Present(NULL, NULL, NULL, NULL);
}

// this is the function that cleans up Direct3D and COM
void cleanD3D(void)
{
	global_v_buffer->Release();
	global_i_buffer->Release();
	d3ddev->Release();
	d3d->Release();
	if(sound != NULL) sound->Stop();
	ClipCursor(NULL);
}

// this is the function that puts the 3D models into video RAM
void init_graphics(void)
{
	// create a vertex buffer interface called v_buffer
	d3ddev->CreateVertexBuffer(8 * sizeof(VERTEX),
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

// this is the function to invoke the default screen lighting
void init_light(void)
{
	D3DLIGHT9 light;    // create the light struct
	D3DMATERIAL9 material;    // create the material struct

	ZeroMemory(&light, sizeof(light));    // clear out the light struct for use
	light.Type = D3DLIGHT_DIRECTIONAL;    // make the light type 'directional light'
	light.Diffuse = D3DXCOLOR(0.5f, 0.5f, 0.5f, 1.0f);    // set the light's color
	light.Direction = D3DXVECTOR3(-1.0f, 0.0f, -1.0f);


	d3ddev->SetLight(0, &light);    // send the light struct properties to light #0
	d3ddev->LightEnable(0, TRUE);    // turn on light #0

	ZeroMemory(&material, sizeof(D3DMATERIAL9));    // clear out the struct for use
	material.Diffuse = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);    // set diffuse color to white
	material.Ambient = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);    // set ambient color to white

	d3ddev->SetMaterial(&material);    // set the globably-used material to &material
}

void intro(void)
{
	int state = 0;
	NO_PUBLISHER* no_pub = new NO_PUBLISHER();
	// SKILL_ISSUE s_i;
	// PARZIVAL p;
	RELAY_OF_INTEGRITY* roi = new RELAY_OF_INTEGRITY();
	sound = new Sound(IDR_INTRO, TRUE);

	// main loop
	MSG msg;
	while (TRUE)
	{
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if (msg.message == WM_QUIT)
			break;

		if (!running) sound->Stop();

		d3ddev->Clear(0, NULL, D3DCLEAR_TARGET, BG_COLOR, 1.0f, 0);
		d3ddev->Clear(0, NULL, D3DCLEAR_ZBUFFER, BG_COLOR, 1.0f, 0);

		d3ddev->BeginScene();

		d3ddev->SetFVF(CUSTOMFVF);

		// set the projection transform
		D3DXMATRIX matProjection;
		D3DXMatrixPerspectiveFovLH(&matProjection,
			D3DXToRadian(90),									// the horizontal field of view
			(FLOAT)SCREEN_WIDTH / (FLOAT)SCREEN_HEIGHT,			// aspect ratio
			1.0f,												// the near view-plane
			100.0f);											// the far view-plane
		d3ddev->SetTransform(D3DTS_PROJECTION, &matProjection); // set the projection

		// complete World transformation matrices
		D3DXMATRIX matRotateX;
		D3DXMATRIX matRotateY;
		D3DXMATRIX matRotateZ;
		D3DXMatrixRotationX(&matRotateX, D3DXToRadian(*WorldXRotation));
		D3DXMatrixRotationY(&matRotateY, D3DXToRadian(*WorldYRotation));
		D3DXMatrixRotationZ(&matRotateZ, D3DXToRadian(*WorldZRotation));
		d3ddev->SetTransform(D3DTS_WORLD, &(matRotateX * matRotateY * matRotateZ));

		// draw intro
		switch (state)
		{
		case 0:
		{
			roi->draw();
		} break;
		
		case 1:
		{
			no_pub->draw();
		} break;
		
		case 2:
		{} break;
		}

		d3ddev->EndScene();

		d3ddev->Present(NULL, NULL, NULL, NULL);
	}
}

void genVoxelText(Object** pOut, const char* text, int size, D3DXCOLOR color, D3DXVECTOR3 x_y_z_start, FLOAT scale)
{
	// quit if there is not reference to object holder
	if (pOut == NULL) return;
	
	if (*pOut == NULL) *pOut = new Object("text1");

	char current = 0;
	Object* work = *pOut;
	FLOAT x = x_y_z_start.x;
	FLOAT y = x_y_z_start.y;
	FLOAT z = x_y_z_start.z;
	for (int i = 0; i < size; i++)
	{
		current = text[i];

		// switch and generate all possible characters
		// switch statements always fall throught to lowercase letter of same
		switch (current)
		{
			// alphanumerical characters
		case 'A':
		case 'a':
		{
			// left-hand stand
			work->addVoxel(x, y - 0.5, z, scale, scale, scale, color);
			work->addVoxel(x, y - 1, z, scale, scale, scale, color);
			work->addVoxel(x, y - 2, z, scale, scale, scale, color);
			work->addVoxel(x, y - 3, z, scale, scale, scale, color);
			work->addVoxel(x, y - 4, z, scale, scale, scale, color);
			work->addVoxel(x, y - 5, z, scale, scale, scale, color);

			// right-hand stand
			work->addVoxel(x + 3, y - 0.5, z, scale, scale, scale, color);
			work->addVoxel(x + 3, y - 1, z, scale, scale, scale, color);
			work->addVoxel(x + 3, y - 2, z, scale, scale, scale, color);
			work->addVoxel(x + 3, y - 3, z, scale, scale, scale, color);
			work->addVoxel(x + 3, y - 4, z, scale, scale, scale, color);
			work->addVoxel(x + 3, y - 5, z, scale, scale, scale, color);

			// upper connector
			work->addVoxel(x + 1, y, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y, z, scale, scale, scale, color);

			// middle connector
			work->addVoxel(x + 1, y - 3, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y - 3, z, scale, scale, scale, color);

			x += 5;

		} break;

		case 'B':
		case 'b':
		{
			// left-hand stand
			work->addVoxel(x, y, z, scale, scale, scale, color);
			work->addVoxel(x, y - 1, z, scale, scale, scale, color);
			work->addVoxel(x, y - 2, z, scale, scale, scale, color);
			work->addVoxel(x, y - 3, z, scale, scale, scale, color);
			work->addVoxel(x, y - 4, z, scale, scale, scale, color);
			work->addVoxel(x, y - 5, z, scale, scale, scale, color);

			// upper connection
			work->addVoxel(x + 1, y, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y, z, scale, scale, scale, color);

			// middle connection
			work->addVoxel(x + 1, y - 2.5, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y - 2.5, z, scale, scale, scale, color);

			// lower connection
			work->addVoxel(x + 1, y - 5, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y - 5, z, scale, scale, scale, color);

			// upper vertical arm
			work->addVoxel(x + 3, y - 0.5, z, scale, scale, scale, color);
			work->addVoxel(x + 3, y - 1.5, z, scale, scale, scale, color);

			// lower vertical arm
			work->addVoxel(x + 3, y - 3.5, z, scale, scale, scale, color);
			work->addVoxel(x + 3, y - 4.5, z, scale, scale, scale, color);

			x += 5;

		} break;

		case 'C':
		case 'c':
		{
			// left-hand stand
			work->addVoxel(x, y - 1, z, scale, scale, scale, color);
			work->addVoxel(x, y - 2, z, scale, scale, scale, color);
			work->addVoxel(x, y - 3, z, scale, scale, scale, color);
			work->addVoxel(x, y - 4, z, scale, scale, scale, color);

			// upper-arm
			work->addVoxel(x + 1, y, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y, z, scale, scale, scale, color);
			work->addVoxel(x + 3, y, z, scale, scale, scale, color);

			// lower-arm
			work->addVoxel(x + 1, y - 5, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y - 5, z, scale, scale, scale, color);
			work->addVoxel(x + 3, y - 5, z, scale, scale, scale, color);

			x += 5;

		} break;

		case 'D':
		case 'd':
		{
			// left-hand stand
			work->addVoxel(x, y, z, scale, scale, scale, color);
			work->addVoxel(x, y - 1, z, scale, scale, scale, color);
			work->addVoxel(x, y - 2, z, scale, scale, scale, color);
			work->addVoxel(x, y - 3, z, scale, scale, scale, color);
			work->addVoxel(x, y - 4, z, scale, scale, scale, color);
			work->addVoxel(x, y - 5, z, scale, scale, scale, color);

			// upper connection arm
			work->addVoxel(x + 1, y, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y, z, scale, scale, scale, color);

			// lower connection arm
			work->addVoxel(x + 1, y - 5, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y - 5, z, scale, scale, scale, color);

			// right-hand stand
			work->addVoxel(x + 3, y - 1, z, scale, scale, scale, color);
			work->addVoxel(x + 3, y - 2, z, scale, scale, scale, color);
			work->addVoxel(x + 3, y - 3, z, scale, scale, scale, color);
			work->addVoxel(x + 3, y - 4, z, scale, scale, scale, color);

			x += 5;

		} break;

		case 'E':
		case 'e':
		{
			// left-hand stand
			work->addVoxel(x, y, z, scale, scale, scale, color);
			work->addVoxel(x, y - 1, z, scale, scale, scale, color);
			work->addVoxel(x, y - 2, z, scale, scale, scale, color);
			work->addVoxel(x, y - 3, z, scale, scale, scale, color);
			work->addVoxel(x, y - 4, z, scale, scale, scale, color);
			work->addVoxel(x, y - 5, z, scale, scale, scale, color);

			// upper arm
			work->addVoxel(x + 1, y, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y, z, scale, scale, scale, color);
			work->addVoxel(x + 3, y, z, scale, scale, scale, color);

			// middle arm
			work->addVoxel(x + 1, y - 2.5, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y - 2.5, z, scale, scale, scale, color);
			work->addVoxel(x + 3, y - 2.5, z, scale, scale, scale, color);

			// lower arm
			work->addVoxel(x + 1, y - 5, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y - 5, z, scale, scale, scale, color);
			work->addVoxel(x + 3, y - 5, z, scale, scale, scale, color);

			x += 5;
		} break;

		case 'F':
		case 'f':
		{
			// left-hand stand
			work->addVoxel(x, y, z, scale, scale, scale, color);
			work->addVoxel(x, y - 1, z, scale, scale, scale, color);
			work->addVoxel(x, y - 2, z, scale, scale, scale, color);
			work->addVoxel(x, y - 3, z, scale, scale, scale, color);
			work->addVoxel(x, y - 4, z, scale, scale, scale, color);
			work->addVoxel(x, y - 5, z, scale, scale, scale, color);

			// upper arm
			work->addVoxel(x + 1, y, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y, z, scale, scale, scale, color);
			work->addVoxel(x + 3, y, z, scale, scale, scale, color);

			// middle arm
			work->addVoxel(x + 1, y - 2.5, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y - 2.5, z, scale, scale, scale, color);
			work->addVoxel(x + 3, y - 2.5, z, scale, scale, scale, color);

			x += 5;
		} break;

		case 'G':
		case 'g':
		{
			// left-hand stand
			work->addVoxel(x, y, z, scale, scale, scale, color);
			work->addVoxel(x, y - 1, z, scale, scale, scale, color);
			work->addVoxel(x, y - 2, z, scale, scale, scale, color);
			work->addVoxel(x, y - 3, z, scale, scale, scale, color);
			work->addVoxel(x, y - 4, z, scale, scale, scale, color);
			work->addVoxel(x, y - 5, z, scale, scale, scale, color);

			// upper arm
			work->addVoxel(x + 1, y, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y, z, scale, scale, scale, color);
			work->addVoxel(x + 3, y, z, scale, scale, scale, color);

			// lower arm
			work->addVoxel(x + 1, y - 5, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y - 5, z, scale, scale, scale, color);
			work->addVoxel(x + 3, y - 5, z, scale, scale, scale, color);

			// cattycorner
			work->addVoxel(x + 3, y - 4, z, scale, scale, scale, color);
			work->addVoxel(x + 3, y - 3, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y - 3, z, scale, scale, scale, color);

			x += 5;

		} break;

		case 'H':
		case 'h':
		{
			// left-hand stand
			work->addVoxel(x, y, z, scale, scale, scale, color);
			work->addVoxel(x, y - 1, z, scale, scale, scale, color);
			work->addVoxel(x, y - 2, z, scale, scale, scale, color);
			work->addVoxel(x, y - 3, z, scale, scale, scale, color);
			work->addVoxel(x, y - 4, z, scale, scale, scale, color);
			work->addVoxel(x, y - 5, z, scale, scale, scale, color);

			// right-hand stand
			work->addVoxel(x + 3, y, z, scale, scale, scale, color);
			work->addVoxel(x + 3, y - 1, z, scale, scale, scale, color);
			work->addVoxel(x + 3, y - 2, z, scale, scale, scale, color);
			work->addVoxel(x + 3, y - 3, z, scale, scale, scale, color);
			work->addVoxel(x + 3, y - 4, z, scale, scale, scale, color);
			work->addVoxel(x + 3, y - 5, z, scale, scale, scale, color);

			// connector
			work->addVoxel(x + 1, y - 2.5, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y - 2.5, z, scale, scale, scale, color);

			x += 5;
		} break;

		case 'I':
		case 'i':
		{
			work->addVoxel(x, y, z, scale, scale, scale, color);
			work->addVoxel(x, y - 1, z, scale, scale, scale, color);
			work->addVoxel(x, y - 2, z, scale, scale, scale, color);
			work->addVoxel(x, y - 3, z, scale, scale, scale, color);
			work->addVoxel(x, y - 4, z, scale, scale, scale, color);
			work->addVoxel(x, y - 5, z, scale, scale, scale, color);

			x += 2;
		} break;

		case 'J':
		case 'j':
		{
			// stand
			work->addVoxel(x + 2, y, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y - 1, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y - 2, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y - 3, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y - 4, z, scale, scale, scale, color);
		
			// bow
			work->addVoxel(x + 1, y - 5, z, scale, scale, scale, color);
			work->addVoxel(x, y - 5, z, scale, scale, scale, color);

			x += 4;
		} break;

		case 'K':
		case 'k':
		{
			// left-hand stand
			work->addVoxel(x, y, z, scale, scale, scale, color);
			work->addVoxel(x, y - 1, z, scale, scale, scale, color);
			work->addVoxel(x, y - 2, z, scale, scale, scale, color);
			work->addVoxel(x, y - 3, z, scale, scale, scale, color);
			work->addVoxel(x, y - 4, z, scale, scale, scale, color);
			work->addVoxel(x, y - 5, z, scale, scale, scale, color);

			// fork
			work->addVoxel(x + 3, y, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y - 1, z, scale, scale, scale, color);
			work->addVoxel(x + 1, y - 2, z, scale, scale, scale, color);
			work->addVoxel(x + 1, y - 3, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y - 4, z, scale, scale, scale, color);
			work->addVoxel(x + 3, y - 5, z, scale, scale, scale, color);

			x += 5;
		} break;

		case 'L':
		case 'l':
		{
			// left-hand stand
			work->addVoxel(x, y, z, scale, scale, scale, color);
			work->addVoxel(x, y - 1, z, scale, scale, scale, color);
			work->addVoxel(x, y - 2, z, scale, scale, scale, color);
			work->addVoxel(x, y - 3, z, scale, scale, scale, color);
			work->addVoxel(x, y - 4, z, scale, scale, scale, color);
			work->addVoxel(x, y - 5, z, scale, scale, scale, color);

			// lower arm
			work->addVoxel(x + 1, y - 5, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y - 5, z, scale, scale, scale, color);

			x += 4;
		} break;

		case 'M':
		case 'm':
		{
			// left-hand stand
			work->addVoxel(x, y, z, scale, scale, scale, color);
			work->addVoxel(x, y - 1, z, scale, scale, scale, color);
			work->addVoxel(x, y - 2, z, scale, scale, scale, color);
			work->addVoxel(x, y - 3, z, scale, scale, scale, color);
			work->addVoxel(x, y - 4, z, scale, scale, scale, color);
			work->addVoxel(x, y - 5, z, scale, scale, scale, color);

			// right-hand stand
			work->addVoxel(x + 4, y, z, scale, scale, scale, color);
			work->addVoxel(x + 4, y - 1, z, scale, scale, scale, color);
			work->addVoxel(x + 4, y - 2, z, scale, scale, scale, color);
			work->addVoxel(x + 4, y - 3, z, scale, scale, scale, color);
			work->addVoxel(x + 4, y - 4, z, scale, scale, scale, color);
			work->addVoxel(x + 4, y - 5, z, scale, scale, scale, color);

			// m-fork
			work->addVoxel(x + 1, y - 1, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y - 2, z, scale, scale, scale, color);
			work->addVoxel(x + 3, y - 1, z, scale, scale, scale, color);

			x += 6;
		} break;

		case 'N':
		case 'n':
		{
			// left-hand stand
			work->addVoxel(x, y, z, scale, scale, scale, color);
			work->addVoxel(x, y - 1, z, scale, scale, scale, color);
			work->addVoxel(x, y - 2, z, scale, scale, scale, color);
			work->addVoxel(x, y - 3, z, scale, scale, scale, color);
			work->addVoxel(x, y - 4, z, scale, scale, scale, color);
			work->addVoxel(x, y - 5, z, scale, scale, scale, color);

			// right-hand stand
			work->addVoxel(x + 4, y, z, scale, scale, scale, color);
			work->addVoxel(x + 4, y - 1, z, scale, scale, scale, color);
			work->addVoxel(x + 4, y - 2, z, scale, scale, scale, color);
			work->addVoxel(x + 4, y - 3, z, scale, scale, scale, color);
			work->addVoxel(x + 4, y - 4, z, scale, scale, scale, color);
			work->addVoxel(x + 4, y - 5, z, scale, scale, scale, color);

			// diagonal
			work->addVoxel(x + 1, y - 1, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y - 2, z, scale, scale, scale, color);
			work->addVoxel(x + 3, y - 3, z, scale, scale, scale, color);

			x += 6;
		} break;

		case 'O':
		case 'o':
		{
			// left-hand stand
			work->addVoxel(x, y - 1, z, scale, scale, scale, color);
			work->addVoxel(x, y - 2, z, scale, scale, scale, color);
			work->addVoxel(x, y - 3, z, scale, scale, scale, color);
			work->addVoxel(x, y - 4, z, scale, scale, scale, color);

			// right-hand stand
			work->addVoxel(x + 3, y - 1, z, scale, scale, scale, color);
			work->addVoxel(x + 3, y - 2, z, scale, scale, scale, color);
			work->addVoxel(x + 3, y - 3, z, scale, scale, scale, color);
			work->addVoxel(x + 3, y - 4, z, scale, scale, scale, color);

			// upper connector
			work->addVoxel(x + 1, y, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y, z, scale, scale, scale, color);

			// lower connector
			work->addVoxel(x + 1, y - 5, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y - 5, z, scale, scale, scale, color);

			x += 5;
		} break;

		case 'P':
		case 'p':
		{
			// left-hand stand
			work->addVoxel(x, y, z, scale, scale, scale, color);
			work->addVoxel(x, y - 1, z, scale, scale, scale, color);
			work->addVoxel(x, y - 2, z, scale, scale, scale, color);
			work->addVoxel(x, y - 3, z, scale, scale, scale, color);
			work->addVoxel(x, y - 4, z, scale, scale, scale, color);
			work->addVoxel(x, y - 5, z, scale, scale, scale, color);

			// upper arm
			work->addVoxel(x + 1, y, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y, z, scale, scale, scale, color);

			// lower arm
			work->addVoxel(x + 1, y - 3, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y - 3, z, scale, scale, scale, color);

			// right-hand beam
			work->addVoxel(x + 3, y - 1, z, scale, scale, scale, color);
			work->addVoxel(x + 3, y - 2, z, scale, scale, scale, color);

			x += 5;
		} break;

		case 'Q':
		case 'q':
		{
			// left-hand stand
			work->addVoxel(x, y - 1, z, scale, scale, scale, color);
			work->addVoxel(x, y - 2, z, scale, scale, scale, color);
			work->addVoxel(x, y - 3, z, scale, scale, scale, color);
			work->addVoxel(x, y - 4, z, scale, scale, scale, color);

			// right-hand stand
			work->addVoxel(x + 3, y - 1, z, scale, scale, scale, color);
			work->addVoxel(x + 3, y - 2, z, scale, scale, scale, color);
			work->addVoxel(x + 3, y - 3, z, scale, scale, scale, color);
			work->addVoxel(x + 3, y - 4, z, scale, scale, scale, color);

			// upper connector
			work->addVoxel(x + 1, y, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y, z, scale, scale, scale, color);

			// lower connector
			work->addVoxel(x + 1, y - 5, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y - 5, z, scale, scale, scale, color);

			// funny little dot
			work->addVoxel(x + 3, y - 5, z, scale, scale, scale, color);
			work->addVoxel(x + 4, y - 6, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y - 4, z, scale, scale, scale, color);
			

			x += 6;
		
		} break;

		case 'R':
		case 'r':
		{
			// left-hand side
			work->addVoxel(x, y, z, scale, scale, scale, color);
			work->addVoxel(x, y - 1, z, scale, scale, scale, color);
			work->addVoxel(x, y - 2, z, scale, scale, scale, color);
			work->addVoxel(x, y - 3, z, scale, scale, scale, color);
			work->addVoxel(x, y - 4, z, scale, scale, scale, color);
			work->addVoxel(x, y - 5, z, scale, scale, scale, color);

			// upper arm
			work->addVoxel(x + 1, y, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y, z, scale, scale, scale, color);

			// lower arm
			work->addVoxel(x + 1, y - 3, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y - 3, z, scale, scale, scale, color);

			// right-hand beam
			work->addVoxel(x + 3, y - 1, z, scale, scale, scale, color);
			work->addVoxel(x + 3, y - 2, z, scale, scale, scale, color);

			// diagonal beam
			work->addVoxel(x + 3, y - 4, z, scale, scale, scale, color);
			work->addVoxel(x + 3, y - 5, z, scale, scale, scale, color);

			x += 5;
		} break;

		case 'S':
		case 's':
		{
			// upper beam
			work->addVoxel(x, y, z, scale, scale, scale, color);
			work->addVoxel(x + 1, y, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y, z, scale, scale, scale, color);

			// middle beam
			work->addVoxel(x, y - 2.5, z, scale, scale, scale, color);
			work->addVoxel(x + 1, y - 2.5, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y - 2.5, z, scale, scale, scale, color);

			// lower beam
			work->addVoxel(x, y - 5, z, scale, scale, scale, color);
			work->addVoxel(x + 1, y - 5, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y - 5, z, scale, scale, scale, color);

			// connectors
			work->addVoxel(x, y - 1, z, scale, scale, scale, color);
			work->addVoxel(x, y - 2, z, scale, scale, scale, color);

			work->addVoxel(x + 2, y - 3, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y - 4, z, scale, scale, scale, color);

			x += 4;
		} break;

		case 'T':
		case 't':
		{
			// middle stand
			work->addVoxel(x + 1.5, y, z, scale, scale, scale, color);
			work->addVoxel(x + 1.5, y - 1, z, scale, scale, scale, color);
			work->addVoxel(x + 1.5, y - 2, z, scale, scale, scale, color);
			work->addVoxel(x + 1.5, y - 3, z, scale, scale, scale, color);
			work->addVoxel(x + 1.5, y - 4, z, scale, scale, scale, color);
			work->addVoxel(x + 1.5, y - 5, z, scale, scale, scale, color);

			// t-bone
			work->addVoxel(x + 1, y, z, scale, scale, scale, color);
			work->addVoxel(x + 0, y, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y, z, scale, scale, scale, color);
			work->addVoxel(x + 3, y, z, scale, scale, scale, color);

			x += 5;
		} break;

		case 'U':
		case 'u':
		{
			// left-hand scope
			work->addVoxel(x, y, z, scale, scale, scale, color);
			work->addVoxel(x, y - 1, z, scale, scale, scale, color);
			work->addVoxel(x, y - 2, z, scale, scale, scale, color);
			work->addVoxel(x, y - 3, z, scale, scale, scale, color);
			work->addVoxel(x, y - 4, z, scale, scale, scale, color);

			// right-hand scope
			work->addVoxel(x + 3, y, z, scale, scale, scale, color);
			work->addVoxel(x + 3, y - 1, z, scale, scale, scale, color);
			work->addVoxel(x + 3, y - 2, z, scale, scale, scale, color);
			work->addVoxel(x + 3, y - 3, z, scale, scale, scale, color);
			work->addVoxel(x + 3, y - 4, z, scale, scale, scale, color);

			// lower foot
			work->addVoxel(x + 1, y - 5, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y - 5, z, scale, scale, scale, color);

			x += 5;
		} break;

		case 'V':
		case 'v':
		{
			// left-hand scope
			work->addVoxel(x, y, z, scale, scale, scale, color);
			work->addVoxel(x, y - 1, z, scale, scale, scale, color);
			work->addVoxel(x, y - 2, z, scale, scale, scale, color);
			work->addVoxel(x, y - 3, z, scale, scale, scale, color);
			work->addVoxel(x + 1, y - 4, z, scale, scale, scale, color);

			// right-hand scope
			work->addVoxel(x + 4, y, z, scale, scale, scale, color);
			work->addVoxel(x + 4, y - 1, z, scale, scale, scale, color);
			work->addVoxel(x + 4, y - 2, z, scale, scale, scale, color);
			work->addVoxel(x + 4, y - 3, z, scale, scale, scale, color);
			work->addVoxel(x + 3, y - 4, z, scale, scale, scale, color);

			// lower foot
			work->addVoxel(x + 2, y - 5, z, scale, scale, scale, color);

			x += 6;
		} break;

		case 'W':
		case 'w':
		{
			// left-hand beam
			work->addVoxel(x, y, z, scale, scale, scale, color);
			work->addVoxel(x, y - 1, z, scale, scale, scale, color);
			work->addVoxel(x, y - 2, z, scale, scale, scale, color);
			work->addVoxel(x, y - 3, z, scale, scale, scale, color);
			work->addVoxel(x, y - 4, z, scale, scale, scale, color);
			work->addVoxel(x, y - 5, z, scale, scale, scale, color);
		
			// right-hand beam
			work->addVoxel(x + 4, y, z, scale, scale, scale, color);
			work->addVoxel(x + 4, y - 1, z, scale, scale, scale, color);
			work->addVoxel(x + 4, y - 2, z, scale, scale, scale, color);
			work->addVoxel(x + 4, y - 3, z, scale, scale, scale, color);
			work->addVoxel(x + 4, y - 4, z, scale, scale, scale, color);
			work->addVoxel(x + 4, y - 5, z, scale, scale, scale, color);

			// w-fork
			work->addVoxel(x + 1, y - 4, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y - 5, z, scale, scale, scale, color);
			work->addVoxel(x + 3, y - 4, z, scale, scale, scale, color);

			x += 6;
		} break;

		case 'X':
		case 'x':
		{
			// 1st diagonal beam
			work->addVoxel(x, y, z, scale, scale, scale, color);
			work->addVoxel(x + 1, y - 1, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y - 2, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y - 3, z, scale, scale, scale, color);
			work->addVoxel(x + 3, y - 4, z, scale, scale, scale, color);
			work->addVoxel(x + 4, y - 5, z, scale, scale, scale, color);

			// extension to 1st diagonal beam
			work->addVoxel(x + 1, y - 4, z, scale, scale, scale, color);
			work->addVoxel(x, y - 5, z, scale, scale, scale, color);
			work->addVoxel(x + 4, y, z, scale, scale, scale, color);
			work->addVoxel(x + 3, y - 1, z, scale, scale, scale, color);

			x += 6;
		} break;

		case 'Y':
		case 'y':
		{
			// middle beam
			work->addVoxel(x + 2, y - 2, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y - 3, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y - 4, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y - 5, z, scale, scale, scale, color);

			// extension parts
			work->addVoxel(x, y, z, scale, scale, scale, color);
			work->addVoxel(x + 1, y - 1, z, scale, scale, scale, color);
			work->addVoxel(x + 4, y, z, scale, scale, scale, color);
			work->addVoxel(x + 3, y - 1, z, scale, scale, scale, color);

			x += 6;
		} break;

		case 'Z':
		case 'z':
		{
			// upper beam
			work->addVoxel(x, y, z, scale, scale, scale, color);
			work->addVoxel(x + 1, y, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y, z, scale, scale, scale, color);
			work->addVoxel(x + 3, y, z, scale, scale, scale, color);
			work->addVoxel(x + 4, y, z, scale, scale, scale, color);

			// lower beam
			work->addVoxel(x, y - 5, z, scale, scale, scale, color);
			work->addVoxel(x + 1, y - 5, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y - 5, z, scale, scale, scale, color);
			work->addVoxel(x + 3, y - 5, z, scale, scale, scale, color);
			work->addVoxel(x + 4, y - 5, z, scale, scale, scale, color);

			// diagonal
			work->addVoxel(x + 3, y - 1, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y - 2, z, scale, scale, scale, color);
			work->addVoxel(x + 1, y - 3, z, scale, scale, scale, color);
			work->addVoxel(x, y - 4, z, scale, scale, scale, color);

			x += 6;
		} break;

		// numerical characters
		case '1':
		{
			// stand
			work->addVoxel(x + 2, y, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y - 1, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y - 2, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y - 3, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y - 4, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y - 5, z, scale, scale, scale, color);

			// hook
			work->addVoxel(x + 1, y  - 1, z, scale, scale, scale, color);
			work->addVoxel(x, y - 2, z, scale, scale, scale, color);

			x += 4;

		} break;

		case '2':
		{
			// lower beam
			work->addVoxel(x, y - 5, z, scale, scale, scale, color);
			work->addVoxel(x + 1, y - 5, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y - 5, z, scale, scale, scale, color);
			work->addVoxel(x + 3, y - 5, z, scale, scale, scale, color);

			// diagonal
			work->addVoxel(x, y - 4, z, scale, scale, scale, color);
			work->addVoxel(x + 1, y - 3, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y - 2, z, scale, scale, scale, color);
			work->addVoxel(x + 3, y - 1, z, scale, scale, scale, color);

			// hook
			work->addVoxel(x + 2, y, z, scale, scale, scale, color);
			work->addVoxel(x + 1, y, z, scale, scale, scale, color);
			work->addVoxel(x, y - 1, z, scale, scale, scale, color);

			x += 5;
		} break;

		case '3':
		{
			//  left-hand arm
			work->addVoxel(x, y, z, scale, scale, scale, color);
			work->addVoxel(x + 1, y, z, scale, scale, scale, color);

			work->addVoxel(x, y - 2.5, z, scale, scale, scale, color);
			work->addVoxel(x + 1, y - 2.5, z, scale, scale, scale, color);

			work->addVoxel(x, y - 5, z, scale, scale, scale, color);
			work->addVoxel(x + 1, y - 5, z, scale, scale, scale, color);

			// right-hand connector
			work->addVoxel(x + 2, y - 0.5, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y - 1.5, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y - 3.5, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y - 4.5, z, scale, scale, scale, color);

			x += 4;
		} break;

		case '4':
		{
			// catty corner
			work->addVoxel(x, y, z, scale, scale, scale, color);
			work->addVoxel(x, y - 1, z, scale, scale, scale, color);
			work->addVoxel(x, y - 2, z, scale, scale, scale, color);
			work->addVoxel(x, y - 3, z, scale, scale, scale, color);
			work->addVoxel(x + 1, y - 3, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y - 3, z, scale, scale, scale, color);
			work->addVoxel(x + 3, y - 3, z, scale, scale, scale, color);
		
			// holding beam
			work->addVoxel(x + 2, y - 2, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y - 4, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y - 5, z, scale, scale, scale, color);

			x += 5;
		} break;

		case '5':
		{
			// upper beam
			work->addVoxel(x, y, z, scale, scale, scale, color);
			work->addVoxel(x + 1, y, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y, z, scale, scale, scale, color);
			work->addVoxel(x + 3, y, z, scale, scale, scale, color);

			// tron beam
			work->addVoxel(x, y - 1, z, scale, scale, scale, color);
			work->addVoxel(x, y - 2, z, scale, scale, scale, color);

			// curve fork
			work->addVoxel(x + 1, y - 2, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y - 2, z, scale, scale, scale, color);
			work->addVoxel(x + 3, y - 3, z, scale, scale, scale, color);
			work->addVoxel(x + 3, y - 4, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y - 5, z, scale, scale, scale, color);
			work->addVoxel(x + 1, y - 5, z, scale, scale, scale, color);
			work->addVoxel(x, y - 5, z, scale, scale, scale, color);

			x += 5;
		} break;

		case '6':
		{
			// left-hand beam
			work->addVoxel(x + 2, y, z, scale, scale, scale, color);
			work->addVoxel(x + 1, y, z, scale, scale, scale, color);
			work->addVoxel(x, y - 1, z, scale, scale, scale, color);
			work->addVoxel(x, y - 2, z, scale, scale, scale, color);
			work->addVoxel(x, y - 3, z, scale, scale, scale, color);
			work->addVoxel(x, y - 4, z, scale, scale, scale, color);

			// o recurse
			work->addVoxel(x + 1, y - 5, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y - 5, z, scale, scale, scale, color);
			work->addVoxel(x + 3, y - 4, z, scale, scale, scale, color);
			work->addVoxel(x + 3, y - 3.5, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y - 2.5, z, scale, scale, scale, color);
			work->addVoxel(x + 1, y - 2.5, z, scale, scale, scale, color);
		
			x += 5;
		} break;

		case '7':
		{
			// upper beam
			work->addVoxel(x, y, z, scale, scale, scale, color);
			work->addVoxel(x + 1, y, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y, z, scale, scale, scale, color);
			work->addVoxel(x + 3, y, z, scale, scale, scale, color);

			// diagonal
			work->addVoxel(x + 3, y - 1, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y - 2, z, scale, scale, scale, color);
			work->addVoxel(x + 1, y - 3, z, scale, scale, scale, color);
			work->addVoxel(x + 1, y - 4, z, scale, scale, scale, color);
			work->addVoxel(x + 1, y - 5, z, scale, scale, scale, color);


			x += 5;
		} break;

		case '8':
		{
			// upper o
			work->addVoxel(x + 1, y, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y, z, scale, scale, scale, color);

			work->addVoxel(x, y - 0.5, z, scale, scale, scale, color);
			work->addVoxel(x, y - 1.5, z, scale, scale, scale, color);

			work->addVoxel(x + 3, y - 0.5, z, scale, scale, scale, color);
			work->addVoxel(x + 3, y - 1.5, z, scale, scale, scale, color);

			// middle beam
			work->addVoxel(x + 1, y - 2.5, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y - 2.5, z, scale, scale, scale, color);

			// lower u
			work->addVoxel(x + 1, y - 5, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y - 5, z, scale, scale, scale, color);

			work->addVoxel(x, y - 3.5, z, scale, scale, scale, color);
			work->addVoxel(x, y - 4.5, z, scale, scale, scale, color);

			work->addVoxel(x + 3, y - 3.5, z, scale, scale, scale, color);
			work->addVoxel(x + 3, y - 4.5, z, scale, scale, scale, color);

			x += 5;
		} break;

		case '9':
		{
			// downward fork
			work->addVoxel(x + 3, y - 1, z, scale, scale, scale, color);
			work->addVoxel(x + 3, y - 2, z, scale, scale, scale, color);
			work->addVoxel(x + 3, y - 3, z, scale, scale, scale, color);
			work->addVoxel(x + 3, y - 4, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y - 5, z, scale, scale, scale, color);
			work->addVoxel(x + 1, y - 5, z, scale, scale, scale, color);

			// leftward curse
			work->addVoxel(x + 2, y, z, scale, scale, scale, color);
			work->addVoxel(x + 1, y, z, scale, scale, scale, color);
			work->addVoxel(x, y - 1, z, scale, scale, scale, color);
			work->addVoxel(x, y - 1.5, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y - 2.5, z, scale, scale, scale, color);
			work->addVoxel(x + 1, y - 2.5, z, scale, scale, scale, color);

			x += 5;
		} break;

		case '0':
		{
			// left-hand beam
			work->addVoxel(x, y - 1, z, scale, scale, scale, color);
			work->addVoxel(x, y - 2, z, scale, scale, scale, color);
			work->addVoxel(x, y - 3, z, scale, scale, scale, color);
			work->addVoxel(x, y - 4, z, scale, scale, scale, color);
		
			// right-hand beam
			work->addVoxel(x + 3, y - 1, z, scale, scale, scale, color);
			work->addVoxel(x + 3, y - 2, z, scale, scale, scale, color);
			work->addVoxel(x + 3, y - 3, z, scale, scale, scale, color);
			work->addVoxel(x + 3, y - 4, z, scale, scale, scale, color);

			// lower beam
			work->addVoxel(x + 1, y - 5, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y - 5, z, scale, scale, scale, color);

			// upper beam
			work->addVoxel(x + 1, y, z, scale, scale, scale, color);
			work->addVoxel(x + 2, y, z, scale, scale, scale, color);

			// diagonal
			work->addVoxel(x + 2, y - 2, z, scale, scale, scale, color);
			work->addVoxel(x + 1, y - 3, z, scale, scale, scale, color);
			
			x += 5;
		} break;

		// special characters
		case '^':
		{} break;

		case '°':
		{} break;

		case '!':
		{} break;

		case '\"':
		{} break;

		case '§':
		{} break;

		case '$':
		{} break;

		case '%':
		{} break;

		case '&':
		{} break;

		case '/':
		{} break;

		case '(':
		{} break;

		case ')':
		{} break;

		case '=':
		{} break;

		case '\?':
		{} break;

		case '`':
		{} break;

		case '²':
		{} break;

		case '³':
		{} break;

		case '{':
		{} break;

		case '[':
		{} break;

		case ']':
		{} break;

		case '}':
		{} break;

		case '\\':
		{} break;

		case '´':
		{} break;

		case '@':
		{} break;

		case '€':
		{} break;

		case '*':
		{} break;

		case '+':
		{} break;

		case '~':
		{} break;

		case '\'':
		{} break;

		case '#':
		{} break;

		case 'µ':
		{} break;

		case ';':
		{} break;

		case ',':
		{} break;

		case ':':
		{} break;

		case '.':
		{} break;

		case '_':
		{} break;

		case '-':
		{} break;

		case '<':
		{} break;

		case '>':
		{} break;

		case '|':
		{} break;

		case '\n':
		{
			y -= 8;
			x = x_y_z_start.x;

		} break;

		// make space
		default:
		{
			x += 5;
		} break;
		}
	}
}