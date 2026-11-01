#ifndef STAB_H264_ENCODER_H
#define STAB_H264_ENCODER_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <cstdint>
#include <d3d11.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mftransform.h>
#include <icodecapi.h>
#include <codecapi.h>
#include <vector>
#include <functional>

/**
 * @brief Callback disparado quando um novo pacote NAL H.264 é codificado pela GPU.
 */
typedef std::function<void(const uint8_t* data, size_t size, bool isKeyFrame)> OnH264PacketCallback;

/**
 * @brief Codificador H.264 por hardware em modo de ultra-baixa latência (Media Foundation / NVENC / QSV / AMF).
 * Configurado com Zero B-Frames, CBR 35Mbps e suporte a injeção forçada de quadro IDR (REQUEST_IDR).
 */
class H264Encoder {
public:
    H264Encoder();
    ~H264Encoder();

    /**
     * @brief Inicializa o codificador H.264 para a resolução e taxa de quadros especificadas.
     * @param width Largura do vídeo (ex.: 1920 ou 2560).
     * @param height Altura do vídeo (ex.: 1080 ou 1600).
     * @param fps Taxa de quadros alvo (padrão 90 FPS para o Tab S9 FE+).
     * @param bitrateBps Taxa de bits alvo (padrão 35 Mbps).
     * @return true se inicializado com sucesso.
     */
    bool Initialize(int width, int height, int fps = 90, int bitrateBps = 35000000);

    /**
     * @brief Libera o codificador e o pipeline do Media Foundation.
     */
    void Shutdown();

    /**
     * @brief Força a emissão imediata de um quadro-chave IDR no próximo frame (Resposta a REQUEST_IDR).
     */
    void ForceKeyFrame();

    /**
     * @brief Codifica uma textura de tela capturada pela GPU.
     * @param pDevice Dispositivo Direct3D 11.
     * @param pContext Contexto de execução D3D11.
     * @param pTexture Textura BGRA capturada via DXGI.
     * @return true se codificado com sucesso.
     */
    bool EncodeFrame(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, ID3D11Texture2D* pTexture);

    void SetPacketCallback(OnH264PacketCallback cb) { m_onPacketCb = cb; }
    bool IsInitialized() const { return m_initialized; }

private:
    bool CreateEncoderMFT();
    bool ConfigureMediaTypes();
    bool ConfigureLowLatencyCodecParameters();

    int m_width;
    int m_height;
    int m_fps;
    int m_bitrateBps;
    bool m_initialized;
    bool m_forceKeyFrameNext;

    IMFTransform* m_encoderMFT;
    ICodecAPI*    m_codecAPI;
    ID3D11Texture2D* m_cpuStagingTexture;

    OnH264PacketCallback m_onPacketCb;
    UINT64 m_frameIndex;
    std::vector<uint8_t> m_nv12Buffer;
};

#endif // STAB_H264_ENCODER_H
