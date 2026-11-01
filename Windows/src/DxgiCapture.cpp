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
    , m_desktopX(0)
    , m_desktopY(0)
    , m_initialized(false)
    , m_hasAcquiredFrame(false)
    , m_isUsingVirtualDisplay(false)
{
}

DxgiCapture::~DxgiCapture() {
    Shutdown();
}

bool DxgiCapture::CreateD3DDevice(IDXGIAdapter* adapter) {
    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0
    };
    D3D_FEATURE_LEVEL featureLevel;

    UINT createFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
    D3D_DRIVER_TYPE driverType = adapter ? D3D_DRIVER_TYPE_UNKNOWN : D3D_DRIVER_TYPE_HARDWARE;

    HRESULT hr = D3D11CreateDevice(
        adapter,
        driverType,
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

    // 1. Vincula a thread ao Desktop de entrada ativo (resolve erro 0x80070005)
    HDESK hDesk = OpenInputDesktop(0, FALSE, GENERIC_ALL);
    if (hDesk) {
        SetThreadDesktop(hDesk);
        CloseDesktop(hDesk);
    }

    IDXGIFactory1* factory = NULL;
    HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&factory);
    if (FAILED(hr)) {
        printf("[DxgiCapture] Erro ao criar DXGIFactory1 (hr: 0x%08lX)\n", hr);
        return false;
    }

    IDXGIAdapter1* selectedAdapter = NULL;
    IDXGIOutput* selectedOutput = NULL;
    int selectedAdapterIndex = 0;
    int selectedOutputIndex = 0;
    int bestScore = -1;

    for (UINT a = 0; ; ++a) {
        IDXGIAdapter1* curAdapter = NULL;
        if (factory->EnumAdapters1(a, &curAdapter) == DXGI_ERROR_NOT_FOUND) break;

        DXGI_ADAPTER_DESC1 aDesc;
        curAdapter->GetDesc1(&aDesc);
        if (aDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
            curAdapter->Release();
            continue;
        }

        for (UINT o = 0; ; ++o) {
            IDXGIOutput* curOutput = NULL;
            if (curAdapter->EnumOutputs(o, &curOutput) == DXGI_ERROR_NOT_FOUND) break;

            DXGI_OUTPUT_DESC oDesc;
            curOutput->GetDesc(&oDesc);
            int w = oDesc.DesktopCoordinates.right - oDesc.DesktopCoordinates.left;
            int h = oDesc.DesktopCoordinates.bottom - oDesc.DesktopCoordinates.top;
            int left = oDesc.DesktopCoordinates.left;
            int top = oDesc.DesktopCoordinates.top;

            int score = 10; // Base score para monitor primario
            bool isSecondary = (left != 0 || top != 0 || o > 0 || a > 0);
            if (isSecondary) {
                score += 100; // Prioriza monitor secundario estendido
            }
            if (w == 1920 && h == 1080) {
                score += 50; // Alvo preferencial 1080p
            } else if (w == 2560 && h == 1600) {
                score += 40; // Alvo nativo 16:10
            } else if (w > 800) {
                score += 15;
            }

            if (score > bestScore) {
                if (selectedOutput) selectedOutput->Release();
                if (selectedAdapter) selectedAdapter->Release();
                selectedOutput = curOutput;
                selectedAdapter = curAdapter;
                curAdapter->AddRef();
                selectedAdapterIndex = a;
                selectedOutputIndex = o;
                bestScore = score;
            } else {
                curOutput->Release();
            }
        }

        curAdapter->Release();
    }
    factory->Release();

    if (!selectedOutput || !selectedAdapter) {
        printf("[DxgiCapture] Nenhum monitor de saida detectado para duplicacao.\n");
        if (selectedOutput) selectedOutput->Release();
        if (selectedAdapter) selectedAdapter->Release();
        return false;
    }

    // Cria o D3D11Device no adapter correto que possui a saida
    if (m_d3dDevice) {
        m_d3dDevice->Release();
        m_d3dDevice = NULL;
    }
    if (m_d3dContext) {
        m_d3dContext->Release();
        m_d3dContext = NULL;
    }

    if (!CreateD3DDevice(selectedAdapter)) {
        selectedOutput->Release();
        selectedAdapter->Release();
        return false;
    }
    selectedAdapter->Release();

    DXGI_OUTPUT_DESC outputDesc;
    selectedOutput->GetDesc(&outputDesc);
    m_width = outputDesc.DesktopCoordinates.right - outputDesc.DesktopCoordinates.left;
    m_height = outputDesc.DesktopCoordinates.bottom - outputDesc.DesktopCoordinates.top;
    m_desktopX = outputDesc.DesktopCoordinates.left;
    m_desktopY = outputDesc.DesktopCoordinates.top;
    m_isUsingVirtualDisplay = (m_desktopX != 0 || m_desktopY != 0 || selectedOutputIndex > 0 || selectedAdapterIndex > 0);

    if (m_isUsingVirtualDisplay) {
        printf("[DxgiCapture] Monitor Estendido selecionado: (Adapter %d, Output %d: %dx%d em %d,%d)!\n",
               selectedAdapterIndex, selectedOutputIndex, m_width, m_height, m_desktopX, m_desktopY);
    } else {
        printf("[DxgiCapture] Utilizando monitor primario espelhado (Adapter %d, Output %d: %dx%d).\n",
               selectedAdapterIndex, selectedOutputIndex, m_width, m_height);
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

    // Cria textura D3D11 de retencao para sustentar cadencia estavel durante cenas estaticas
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

    // Vincula a thread ao Desktop de entrada ativo
    HDESK hDesk = OpenInputDesktop(0, FALSE, GENERIC_ALL);
    if (hDesk) {
        SetThreadDesktop(hDesk);
        CloseDesktop(hDesk);
    }

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

bool DxgiCapture::AcquireNextFrame(ID3D11Texture2D** ppTexture, DWORD timeoutMs, bool* pIsNewFrame) {
    if (pIsNewFrame) *pIsNewFrame = false;
    if (!m_initialized || !m_duplication) return false;

    if (m_hasAcquiredFrame) {
        ReleaseFrame();
    }

    DXGI_OUTDUPL_FRAME_INFO frameInfo;
    IDXGIResource* desktopResource = NULL;

    HRESULT hr = m_duplication->AcquireNextFrame(timeoutMs, &frameInfo, &desktopResource);

    if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
        // Nenhuma alteracao de pixels na tela dentro da janela de amostragem.
        // Sustenta a cadencia com a textura do ultimo frame capturado se disponivel.
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

    // Houve atualizacao real de pixels na GPU
    if (pIsNewFrame) *pIsNewFrame = true;

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

