#ifndef STAB_DXGI_CAPTURE_H
#define STAB_DXGI_CAPTURE_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>

/**
 * @brief Capturador de tela de ultra-baixa latência utilizando a Desktop Duplication API (DirectX 11).
 * Lê os frames diretamente da memória de vídeo (VRAM) sem intermediar pela CPU (Zero-Copy).
 */
class DxgiCapture {
public:
    DxgiCapture();
    ~DxgiCapture();

    /**
     * @brief Inicializa o dispositivo Direct3D 11 e a duplicação de saída do monitor primário.
     * @return true se a duplicação for inicializada com sucesso.
     */
    bool Initialize();

    /**
     * @brief Libera os recursos do DirectX e a duplicação de tela.
     */
    void Shutdown();

    /**
     * @brief Captura o próximo frame disponível da GPU.
     * @param ppTexture Ponteiro para receber a textura D3D11 capturada.
     * @param timeoutMs Tempo limite de espera por novo frame (padrão 12ms para ~90fps).
     * @return true se um novo frame foi adquirido com sucesso.
     */
    bool AcquireNextFrame(ID3D11Texture2D** ppTexture, DWORD timeoutMs = 12);

    /**
     * @brief Libera o frame adquirido anteriormente junto à Desktop Duplication API.
     */
    void ReleaseFrame();

    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }
    int GetDesktopX() const { return m_desktopX; }
    int GetDesktopY() const { return m_desktopY; }
    ID3D11Device* GetDevice() const { return m_d3dDevice; }
    ID3D11DeviceContext* GetContext() const { return m_d3dContext; }
    bool IsInitialized() const { return m_initialized; }
    bool IsUsingVirtualDisplay() const { return m_isUsingVirtualDisplay; }

private:
    bool CreateD3DDevice(IDXGIAdapter* adapter = NULL);
    bool SetupDuplication();

    ID3D11Device*           m_d3dDevice;
    ID3D11DeviceContext*    m_d3dContext;
    IDXGIOutputDuplication* m_duplication;
    ID3D11Texture2D*        m_stagingTexture;
    ID3D11Texture2D*        m_lastFrameTexture;

    int m_width;
    int m_height;
    int m_desktopX;
    int m_desktopY;
    bool m_initialized;
    bool m_hasAcquiredFrame;
    bool m_isUsingVirtualDisplay;
};

#endif // STAB_DXGI_CAPTURE_H

