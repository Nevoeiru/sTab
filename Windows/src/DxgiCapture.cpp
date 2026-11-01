#include "DxgiCapture.h"
#include <stdio.h>

DxgiCapture::DxgiCapture()
    : m_d3dDevice(NULL)
    , m_d3dContext(NULL)
    , m_duplication(NULL)
    , m_stagingTexture(NULL)
    , m_lastFrameTexture(NULL)
    , m_width(1920)
    , m_height(1080)
    , m_initialized(false)
    , m_hasAcquiredFrame(false)
    , m_isUsingVirtualDisplay(false)
{
}

DxgiCapture::~DxgiCapture() {
    Shutdown();
}

bool DxgiCapture::CreateD3DDevice() {
    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0
    };
    D3D_FEATURE_LEVEL featureLevel;

    UINT createFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

    HRESULT hr = D3D11CreateDevice(
        NULL,
        D3D_DRIVER_TYPE_HARDWARE,
        NULL,
        createFlags,
        featureLevels,
        2,
        D3D11_SDK_VERSION,
        &m_d3dDevice,
        &featureLevel,
        &m_d3dContext
    );

    if (FAILED(hr)) {
        printf("[DxgiCapture] Erro ao criar D3D11 Device por hardware (hr: 0x%08lX)\n", hr);
        return false;
    }

    return true;
}

bool DxgiCapture::SetupDuplication() {
    if (m_duplication) {
        m_duplication->Release();
        m_duplication = NULL;
    }

    IDXGIDevice* dxgiDevice = NULL;
    HRESULT hr = m_d3dDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);
    if (FAILED(hr)) return false;

    IDXGIAdapter* adapter = NULL;
    hr = dxgiDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&adapter);
    dxgiDevice->Release();
    if (FAILED(hr)) return false;

    // Enumera todas as saídas de vídeo conectadas na GPU
    IDXGIOutput* selectedOutput = NULL;
    int selectedIndex = 0;
    bool foundTarget1610 = false;

    for (UINT i = 0; ; ++i) {
        IDXGIOutput* curOutput = NULL;
        if (adapter->EnumOutputs(i, &curOutput) == DXGI_ERROR_NOT_FOUND) {
            break;
        }

        DXGI_OUTPUT_DESC desc;
        curOutput->GetDesc(&desc);
        int w = desc.DesktopCoordinates.right - desc.DesktopCoordinates.left;
        int h = desc.DesktopCoordinates.bottom - desc.DesktopCoordinates.top;

        // Se encontrar o monitor virtual 2560x1600 (VDD 16:10), seleciona com prioridade máxima
        if (w == 2560 && h == 1600) {
            if (selectedOutput) selectedOutput->Release();
            selectedOutput = curOutput;
            selectedIndex = i;
            foundTarget1610 = true;
            break;
        } else if (!foundTarget1610 && selectedOutput == NULL) {
            selectedOutput = curOutput;
            selectedIndex = i;
        } else {
            curOutput->Release();
        }
    }

    adapter->Release();

    if (!selectedOutput) {
        printf("[DxgiCapture] Nenhum monitor de saida detectado para duplicacao.\n");
        return false;
    }

    DXGI_OUTPUT_DESC outputDesc;
    selectedOutput->GetDesc(&outputDesc);
    m_width = outputDesc.DesktopCoordinates.right - outputDesc.DesktopCoordinates.left;
    m_height = outputDesc.DesktopCoordinates.bottom - outputDesc.DesktopCoordinates.top;
    m_isUsingVirtualDisplay = foundTarget1610;

    if (foundTarget1610) {
        printf("[DxgiCapture] Monitor Virtual 16:10 nativo detectado (Output %d: 2560x1600 @ 90Hz)!\n", selectedIndex);
    } else {
        printf("[DxgiCapture] Utilizando saida de video (Output %d: %dx%d).\n", selectedIndex, m_width, m_height);
    }

    IDXGIOutput1* output1 = NULL;
    hr = selectedOutput->QueryInterface(__uuidof(IDXGIOutput1), (void**)&output1);
    selectedOutput->Release();
    if (FAILED(hr)) return false;

    hr = output1->DuplicateOutput(m_d3dDevice, &m_duplication);
    output1->Release();

    if (FAILED(hr)) {
        printf("[DxgiCapture] Erro em DuplicateOutput (hr: 0x%08lX)\n", hr);
        return false;
    }

    // Cria textura D3D11 de retenção para sustentar cadência estável durante cenas estáticas
    if (m_lastFrameTexture) {
        m_lastFrameTexture->Release();
        m_lastFrameTexture = NULL;
    }

    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = m_width;
    texDesc.Height = m_height;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    m_d3dDevice->CreateTexture2D(&texDesc, NULL, &m_lastFrameTexture);

    return true;
}

bool DxgiCapture::Initialize() {
    if (m_initialized) return true;

    if (!CreateD3DDevice()) return false;
    if (!SetupDuplication()) return false;

    m_initialized = true;
    printf("[DxgiCapture] Desktop Duplication GPU inicializado com sucesso (%dx%d).\n", m_width, m_height);
    return true;
}

void DxgiCapture::Shutdown() {
    ReleaseFrame();

    if (m_lastFrameTexture) {
        m_lastFrameTexture->Release();
        m_lastFrameTexture = NULL;
    }

    if (m_stagingTexture) {
        m_stagingTexture->Release();
        m_stagingTexture = NULL;
    }

    if (m_duplication) {
        m_duplication->Release();
        m_duplication = NULL;
    }

    if (m_d3dContext) {
        m_d3dContext->Release();
        m_d3dContext = NULL;
    }

    if (m_d3dDevice) {
        m_d3dDevice->Release();
        m_d3dDevice = NULL;
    }

    m_initialized = false;
}

bool DxgiCapture::AcquireNextFrame(ID3D11Texture2D** ppTexture, DWORD timeoutMs) {
    if (!m_initialized || !m_duplication) return false;

    if (m_hasAcquiredFrame) {
        ReleaseFrame();
    }

    DXGI_OUTDUPL_FRAME_INFO frameInfo;
    IDXGIResource* desktopResource = NULL;

    HRESULT hr = m_duplication->AcquireNextFrame(timeoutMs, &frameInfo, &desktopResource);

    if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
        // Nenhuma alteracao de pixels na tela dentro da janela de amostragem.
        // Sustenta a cadencia continua com a textura do ultimo frame capturado
        if (m_lastFrameTexture) {
            *ppTexture = m_lastFrameTexture;
            return true;
        }
        return false;
    }

    if (hr == DXGI_ERROR_ACCESS_LOST) {
        printf("[DxgiCapture] Acesso ao display perdido (mudanca de modo/resolucao). Reinicializando...\n");
        SetupDuplication();
        return false;
    }

    if (FAILED(hr)) {
        return false;
    }

    hr = desktopResource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)ppTexture);
    desktopResource->Release();

    if (FAILED(hr)) {
        m_duplication->ReleaseFrame();
        return false;
    }

    // Atualiza a textura de retencao com o novo frame em VRAM
    if (m_lastFrameTexture && *ppTexture) {
        m_d3dContext->CopyResource(m_lastFrameTexture, *ppTexture);
    }

    m_hasAcquiredFrame = true;
    return true;
}

void DxgiCapture::ReleaseFrame() {
    if (m_hasAcquiredFrame && m_duplication) {
        m_duplication->ReleaseFrame();
        m_hasAcquiredFrame = false;
    }
}

