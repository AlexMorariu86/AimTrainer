
#include <Windows.h>
#include <mmsystem.h>
#include <d3dx9.h>
#include "Camera.h"



//-----------------------------------------------------------------------------
// Global variables
//-----------------------------------------------------------------------------
LPDIRECT3D9             D3D           = NULL; // Used to create the D3DDevice
LPDIRECT3DDEVICE9       d3dDevice     = NULL; // Our rendering device

LPD3DXMESH              Mesh          = NULL; // Our mesh object in sysmem
D3DMATERIAL9*           MeshMaterials = NULL; // Materials for our mesh
LPDIRECT3DTEXTURE9*     MeshTextures  = NULL; // Textures for our mesh
DWORD                   NumMaterials = 0L;   // Number of mesh materials
CXCamera *camera; 
POINT mousePos;
POINT lastMousePos = { 0, 0 };

static DWORD lastTime = 0;  
float deltaTime = 0.0f;      

LPD3DXMESH skyboxMesh = NULL;
LPDIRECT3DTEXTURE9 skyboxTextures[6];  // Textures for the 6 faces of the skybox




//-----------------------------------------------------------------------------
// Name: InitD3D()
// Desc: Initializes Direct3D
//-----------------------------------------------------------------------------
HRESULT InitD3D( HWND hWnd )
{
    // Create the D3D object.
    if( NULL == ( D3D = Direct3DCreate9( D3D_SDK_VERSION ) ) )
        return E_FAIL;

    // Set up the structure used to create the D3DDevice. Since we are now
    // using more complex geometry, we will create a device with a zbuffer.
    D3DPRESENT_PARAMETERS d3dpp; 
    ZeroMemory( &d3dpp, sizeof(d3dpp) );
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
    d3dpp.EnableAutoDepthStencil = TRUE;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D16;

    // Create the D3DDevice
    if( FAILED( D3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
                                      D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                                      &d3dpp, &d3dDevice ) ) )
    {
		if( FAILED( D3D->CreateDevice( D3DADAPTER_DEFAULT, D3DDEVTYPE_REF, hWnd,
                                      D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                                      &d3dpp, &d3dDevice ) ) )
			return E_FAIL;
    }

    // Turn on the zbuffer
    d3dDevice->SetRenderState( D3DRS_ZENABLE, TRUE );

    // Turn on ambient lighting 
    d3dDevice->SetRenderState( D3DRS_AMBIENT, 0xffffffff );


    return S_OK;
}



void InitiateCamera()
{
	camera = new CXCamera(d3dDevice);

	// Set up our view matrix. A view matrix can be defined given an eye point,
	// a point to lookat, and a direction for which way is up. Here, we set the
	// eye five units back along the z-axis and up three units, look at the 
	// origin, and define "up" to be in the y-direction.
	D3DXVECTOR3 vEyePt(0.0f, 3.0f, -5.0f);
	D3DXVECTOR3 vLookatPt(0.0f, 0.0f, 0.0f);
	D3DXVECTOR3 vUpVec(0.0f, 1.0f, 0.0f);
	D3DXMATRIXA16 matView;

	camera->LookAtPos(&vEyePt, &vLookatPt, &vUpVec);
}

//-----------------------------------------------------------------------------
// Name: InitGeometry()
// Desc: Load the mesh and build the material and texture arrays
//-----------------------------------------------------------------------------
HRESULT InitGeometry()
{
    // Load the mesh (for tiger.x or another mesh)
    LPD3DXBUFFER pD3DXMtrlBuffer;
    if (FAILED(D3DXLoadMeshFromX("Tiger.x", D3DXMESH_SYSTEMMEM, d3dDevice, NULL, &pD3DXMtrlBuffer, NULL, &NumMaterials, &Mesh)))
    {
        MessageBox(NULL, "Could not find tiger.x", "Error", MB_OK);
        return E_FAIL;
    }

    // Set up the materials and textures for the mesh
    D3DXMATERIAL* d3dxMaterials = (D3DXMATERIAL*)pD3DXMtrlBuffer->GetBufferPointer();
    MeshMaterials = new D3DMATERIAL9[NumMaterials];
    MeshTextures = new LPDIRECT3DTEXTURE9[NumMaterials];
    for (DWORD i = 0; i < NumMaterials; i++)
    {
        MeshMaterials[i] = d3dxMaterials[i].MatD3D;
        MeshMaterials[i].Ambient = MeshMaterials[i].Diffuse;

        MeshTextures[i] = NULL;
        if (d3dxMaterials[i].pTextureFilename != NULL && lstrlen(d3dxMaterials[i].pTextureFilename) > 0)
        {
            if (FAILED(D3DXCreateTextureFromFile(d3dDevice, d3dxMaterials[i].pTextureFilename, &MeshTextures[i])))
            {
                MessageBox(NULL, "Could not find texture map", "Error", MB_OK);
            }
        }
    }
    pD3DXMtrlBuffer->Release();

    // Create the skybox mesh
    HRESULT hr = D3DXCreateBox(d3dDevice, 500.0f, 500.0f, 500.0f, &skyboxMesh, NULL);
    if (FAILED(hr))
    {
        MessageBox(NULL, "Failed to create skybox mesh", "Error", MB_OK);
        return E_FAIL;
    }

    // Load skybox textures
    const char* skyboxTextureFiles[6] = {
        "skybox_posx.jpg", "skybox_negx.jpg",
        "skybox_posy.jpg", "skybox_negy.jpg",
        "skybox_posz.jpg", "skybox_negz.jpg"
    };
    for (int i = 0; i < 6; ++i)
    {
        hr = D3DXCreateTextureFromFile(d3dDevice, skyboxTextureFiles[i], &skyboxTextures[i]);
        if (FAILED(hr))
        {
            MessageBox(NULL, "Failed to load skybox textures", "Error", MB_OK);
            return E_FAIL;
        }
    }

    // Setup the camera
    InitiateCamera();
    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: Cleanup()
// Desc: Releases all previously initialized objects
//-----------------------------------------------------------------------------
VOID Cleanup()
{
    if( MeshMaterials != NULL ) 
        delete[] MeshMaterials;

    if( MeshTextures )
    {
        for( DWORD i = 0; i < NumMaterials; i++ )
        {
            if( MeshTextures[i] )
                MeshTextures[i]->Release();
        }
        delete[] MeshTextures;
    }
    if( Mesh != NULL )
        Mesh->Release();
    
    if( d3dDevice != NULL )
        d3dDevice->Release();

    if( D3D != NULL )
        D3D->Release();
}




float distance;
int direction = 1;//1 forward, -1 backward
int maximumStepDirection = 300;
int currentStep = 0;



void SetupProjectionMatrix()
{
	// For the projection matrix, we set up a perspective transform (which
	// transforms geometry from 3D view space to 2D viewport space, with
	// a perspective divide making objects smaller in the distance). To build
	// a perpsective transform, we need the field of view (1/4 pi is common),
	// the aspect ratio, and the near and far clipping planes (which define at
	// what distances geometry should be no longer be rendered).
	D3DXMATRIXA16 matProj;
D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI / 4, 1.0f, 1.0f, 100.0f);
d3dDevice->SetTransform(D3DTS_PROJECTION, &matProj);
}

//-----------------------------------------------------------------------------
// Name: SetupMatrices()
// Desc: Sets up the world, view, and projection transform matrices.
//-----------------------------------------------------------------------------
VOID SetupMatrices()
{

	SetupProjectionMatrix();

}
void HandleCameraInput(float deltaTime)
{
    // Get mouse movement deltas
    LONG mouseDeltaX = 0, mouseDeltaY = 0;
    if (GetCursorPos(&mousePos))
    {
        // Calculate the change in the mouse position
        mouseDeltaX = mousePos.x - lastMousePos.x;
        mouseDeltaY = mousePos.y - lastMousePos.y;

        // Update the last mouse position for the next frame
        lastMousePos = mousePos;
    }

    // Rotate camera based on mouse movement
    const float sensitivity = 0.005f;  // Sensitivity of the mouse (adjust as needed)
    camera->RotateRight(mouseDeltaX * sensitivity);
    camera->RotateDown(mouseDeltaY * sensitivity);

    // Update the camera view matrix
    camera->Update();
}



//-----------------------------------------------------------------------------
// Name: Render()
// Desc: Draws the scene
//-----------------------------------------------------------------------------
VOID Render()
{
    DWORD currentTime = GetTickCount();
    deltaTime = (currentTime - lastTime) / 1000.0f;  // Convert to seconds
    lastTime = currentTime;  // Store the current time for the next frame

    HandleCameraInput(deltaTime);

    // Clear the backbuffer and the zbuffer
    d3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);

    // Begin the scene
    if (SUCCEEDED(d3dDevice->BeginScene()))
    {
        // Render the skybox first (disable depth testing)
        d3dDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);  // Disable depth testing
        d3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE); // Disable culling

        for (int i = 0; i < 6; ++i)
        {
            d3dDevice->SetTexture(0, skyboxTextures[i]);
            skyboxMesh->DrawSubset(i);
        }

        // Re-enable depth testing and backface culling for other objects
        d3dDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
        d3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);

        // Setup the world, view, and projection matrices
        SetupMatrices();

        // Render the mesh
        for (DWORD i = 0; i < NumMaterials; i++)
        {
            d3dDevice->SetMaterial(&MeshMaterials[i]);
            d3dDevice->SetTexture(0, MeshTextures[i]);
            Mesh->DrawSubset(i);
        }

        // End the scene
        d3dDevice->EndScene();
    }

    // Present the backbuffer contents to the display
    d3dDevice->Present(NULL, NULL, NULL, NULL);
}




//-----------------------------------------------------------------------------
// Name: MsgProc()
// Desc: The window's message handler
//-----------------------------------------------------------------------------
LRESULT WINAPI MsgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    switch( msg )
    {
        case WM_DESTROY:
            Cleanup();
            PostQuitMessage( 0 );
            return 0;
    }

    return DefWindowProc( hWnd, msg, wParam, lParam );
}


//-----------------------------------------------------------------------------
// Name: WinMain()
// Desc: The application's entry point
//-----------------------------------------------------------------------------
INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR, INT )
{
    // Register the window class
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, MsgProc, 0L, 0L, 
                      GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
                      "D3D Tutorial", NULL };
    RegisterClassEx( &wc );

    // Create the application's window
    HWND hWnd = CreateWindow( "D3D Tutorial", "Aim Trainer", 
                              WS_OVERLAPPEDWINDOW, 100, 100, 1024, 1024,
                              GetDesktopWindow(), NULL, wc.hInstance, NULL );

    // Initialize Direct3D
    if( SUCCEEDED( InitD3D( hWnd ) ) )
    { 
        // Create the scene geometry
        if( SUCCEEDED( InitGeometry() ) )
        {
            // Show the window
            ShowWindow( hWnd, SW_SHOWDEFAULT );
            UpdateWindow( hWnd );

            // Enter the message loop
            MSG msg; 
            ZeroMemory( &msg, sizeof(msg) );
            while( msg.message!=WM_QUIT )
            {
                if( PeekMessage( &msg, NULL, 0U, 0U, PM_REMOVE ) )
                {
                    TranslateMessage( &msg );
                    DispatchMessage( &msg );
                }
                else
                    Render();
            }
        }
    }

    UnregisterClass( "D3D Tutorial", wc.hInstance );
    return 0;
}



