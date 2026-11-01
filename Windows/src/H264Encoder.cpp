#include "H264Encoder.h"
#include <stdio.h>
#include <cstdint>

// CLSID padrão do codificador Microsoft H.264 MFT: {6ca50344-051a-4ded-9779-a43305165e35}
static const GUID CLSID_CMSH264EncoderMFT_Local = 
    { 0x6ca50344, 0x051a, 0x4ded, { 0x97, 0x79, 0xa4, 0x33, 0x05, 0x16, 0x5e, 0x35 } };

// IID de ICodecAPI: {901db4c7-31ce-41a2-85dc-8fa0bf41b8da}
static const GUID IID_ICodecAPI_Local = 
    { 0x901db4c7, 0x31ce, 0x41a2, { 0x85, 0xdc, 0x8f, 0xa0, 0xbf, 0x41, 0xb8, 0xda } };

H264Encoder::H264Encoder()
    : m_width(1920)
    , m_height(1080)
    , m_fps(90)
    , m_bitrateBps(35000000)
    , m_initialized(false)
    , m_forceKeyFrameNext(false)
    , m_encoderMFT(NULL)
    , m_codecAPI(NULL)
    , m_cpuStagingTexture(NULL)
    , m_frameIndex(0)
{
}

H264Encoder::~H264Encoder() {
    Shutdown();
}

bool H264Encoder::CreateEncoderMFT() {
    HRESULT hr = CoCreateInstance(
        CLSID_CMSH264EncoderMFT_Local,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_IMFTransform,
        (void**)&m_encoderMFT
    );

    if (FAILED(hr)) {
        printf("[H264Encoder] Falha ao instanciar CLSID_CMSH264EncoderMFT (hr: 0x%08lX)\n", hr);
        return false;
    }

    hr = m_encoderMFT->QueryInterface(IID_ICodecAPI_Local, (void**)&m_codecAPI);
    if (SUCCEEDED(hr)) {
        ConfigureLowLatencyCodecParameters();
    }

    return true;
}

bool H264Encoder::ConfigureLowLatencyCodecParameters() {
    if (!m_codecAPI) return false;

    VARIANT var;
    VariantInit(&var);

    // 1. Ativa modo de ultra-baixa latência em tempo real
    var.vt = VT_BOOL;
    var.boolVal = VARIANT_TRUE;
    m_codecAPI->SetValue(&CODECAPI_AVLowLatencyMode, &var);

    // 2. Trava em Constant Bitrate (CBR) para estabilidade no cabo USB 2.0
    var.vt = VT_UI4;
    var.ulVal = eAVEncCommonRateControlMode_CBR;
    m_codecAPI->SetValue(&CODECAPI_AVEncCommonRateControlMode, &var);

    var.vt = VT_UI4;
    var.ulVal = m_bitrateBps;
    m_codecAPI->SetValue(&CODECAPI_AVEncCommonMeanBitRate, &var);

    // 3. Intervalo de GOP: 1 quadro-chave a cada 90 frames (1 segundo)
    var.vt = VT_UI4;
    var.ulVal = m_fps;
    m_codecAPI->SetValue(&CODECAPI_AVEncMPVGOPSize, &var);

    // 4. Zero B-Frames (elimina atraso de decodificação na pipeline do tablet)
    var.vt = VT_UI4;
    var.ulVal = 0;
    m_codecAPI->SetValue(&CODECAPI_AVEncMPVDefaultBPictureCount, &var);

    // 5. Prioriza velocidade máxima de codificação (Real-time 90fps)
    var.vt = VT_UI4;
    var.ulVal = 100;
    m_codecAPI->SetValue(&CODECAPI_AVEncCommonQualityVsSpeed, &var);

    return true;
}

bool H264Encoder::ConfigureMediaTypes() {
    // 1. Tipo de Mídia de Saída (Stream H.264 NAL)
    IMFMediaType* outputType = NULL;
    HRESULT hr = MFCreateMediaType(&outputType);
    if (FAILED(hr)) return false;

    outputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    outputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264);
    outputType->SetUINT32(MF_MT_AVG_BITRATE, m_bitrateBps);
    MFSetAttributeRatio(outputType, MF_MT_FRAME_RATE, m_fps, 1);
    MFSetAttributeSize(outputType, MF_MT_FRAME_SIZE, m_width, m_height);
    outputType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
    outputType->SetUINT32(MF_MT_MPEG2_PROFILE, eAVEncH264VProfile_Base); // Baseline para compatibilidade perfeita com Android MediaCodec

    hr = m_encoderMFT->SetOutputType(0, outputType, 0);
    outputType->Release();
    if (FAILED(hr)) {
        printf("[H264Encoder] Erro ao configurar OutputType H.264 (hr: 0x%08lX)\n", hr);
        return false;
    }

    // 2. Tipo de Mídia de Entrada (NV12 compatível com GPU)
    IMFMediaType* inputType = NULL;
    hr = MFCreateMediaType(&inputType);
    if (FAILED(hr)) return false;

    inputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    inputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12);
    MFSetAttributeRatio(inputType, MF_MT_FRAME_RATE, m_fps, 1);
    MFSetAttributeSize(inputType, MF_MT_FRAME_SIZE, m_width, m_height);
    inputType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);

    hr = m_encoderMFT->SetInputType(0, inputType, 0);
    inputType->Release();
    if (FAILED(hr)) {
        printf("[H264Encoder] Erro ao configurar InputType NV12 (hr: 0x%08lX)\n", hr);
        return false;
    }

    return true;
}

bool H264Encoder::Initialize(int width, int height, int fps, int bitrateBps) {
    if (m_initialized) return true;

    m_width = width;
    m_height = height;
    m_fps = fps;
    m_bitrateBps = bitrateBps;

    HRESULT hr = MFStartup(MF_VERSION, MFSTARTUP_NOSOCKET);
    if (FAILED(hr)) {
        printf("[H264Encoder] Falha em MFStartup (hr: 0x%08lX)\n", hr);
        return false;
    }

    if (!CreateEncoderMFT()) return false;
    if (!ConfigureMediaTypes()) return false;

    hr = m_encoderMFT->ProcessMessage(MFT_MESSAGE_COMMAND_FLUSH, 0);
    hr = m_encoderMFT->ProcessMessage(MFT_MESSAGE_NOTIFY_BEGIN_STREAMING, 0);
    hr = m_encoderMFT->ProcessMessage(MFT_MESSAGE_NOTIFY_START_OF_STREAM, 0);

    m_initialized = true;
    printf("[H264Encoder] Codificador H.264 Low-Latency GPU inicializado (%dx%d @ %d FPS, %d Mbps).\n",
           m_width, m_height, m_fps, m_bitrateBps / 1000000);
    return true;
}

void H264Encoder::Shutdown() {
    if (!m_initialized) return;

    if (m_encoderMFT) {
        m_encoderMFT->ProcessMessage(MFT_MESSAGE_NOTIFY_END_OF_STREAM, 0);
        m_encoderMFT->ProcessMessage(MFT_MESSAGE_COMMAND_DRAIN, 0);
    }

    if (m_cpuStagingTexture) {
        m_cpuStagingTexture->Release();
        m_cpuStagingTexture = NULL;
    }

    if (m_codecAPI) {
        m_codecAPI->Release();
        m_codecAPI = NULL;
    }

    if (m_encoderMFT) {
        m_encoderMFT->Release();
        m_encoderMFT = NULL;
    }

    MFShutdown();
    m_initialized = false;
}

void H264Encoder::ForceKeyFrame() {
    m_forceKeyFrameNext = true;
    printf("[H264Encoder] >>> FORCANDO GERACAO DE QUADRO-CHAVE (IDR FRAME) SOB DEMANDA <<<\n");
}

bool H264Encoder::EncodeFrame(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, ID3D11Texture2D* pTexture) {
    if (!m_initialized || !m_encoderMFT || !pTexture) return false;

    // Se houver solicitação de REQUEST_IDR pendente, força a emissão de frame chave no hardware
    if (m_forceKeyFrameNext && m_codecAPI) {
        VARIANT var;
        VariantInit(&var);
        var.vt = VT_UI4;
        var.ulVal = 1;
        m_codecAPI->SetValue(&CODECAPI_AVEncVideoForceKeyFrame, &var);
        m_forceKeyFrameNext = false;
    }

    // Criação de staging texture sob demanda caso ainda não exista
    if (!m_cpuStagingTexture) {
        D3D11_TEXTURE2D_DESC desc;
        pTexture->GetDesc(&desc);
        desc.Usage = D3D11_USAGE_STAGING;
        desc.BindFlags = 0;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        desc.MiscFlags = 0;

        HRESULT hr = pDevice->CreateTexture2D(&desc, NULL, &m_cpuStagingTexture);
        if (FAILED(hr)) {
            printf("[H264Encoder] Falha ao criar staging texture (hr: 0x%08lX)\n", hr);
            return false;
        }
    }

    // Copia a textura renderizada da GPU para o buffer de staging
    pContext->CopyResource(m_cpuStagingTexture, pTexture);

    D3D11_MAPPED_SUBRESOURCE mapped;
    HRESULT hr = pContext->Map(m_cpuStagingTexture, 0, D3D11_MAP_READ, 0, &mapped);
    if (FAILED(hr)) {
        printf("[H264Encoder] Falha em Map (hr: 0x%08lX)\n", hr);
        return false;
    }

    // Converte BGRA para NV12 compacto na memória
    int ySize = m_width * m_height;
    int uvSize = ySize / 2;
    int nv12TotalSize = ySize + uvSize;

    std::vector<uint8_t> nv12Buffer(nv12TotalSize);
    uint8_t* pY = nv12Buffer.data();
    uint8_t* pUV = pY + ySize;

    const uint8_t* pSrc = (const uint8_t*)mapped.pData;
    for (int y = 0; y < m_height; ++y) {
        const uint8_t* srcRow = pSrc + (y * mapped.RowPitch);
        for (int x = 0; x < m_width; ++x) {
            uint8_t b = srcRow[x * 4 + 0];
            uint8_t g = srcRow[x * 4 + 1];
            uint8_t r = srcRow[x * 4 + 2];

            // Coeficientes BT.601 para luminância Y
            pY[y * m_width + x] = (uint8_t)(((66 * r + 129 * g + 25 * b + 128) >> 8) + 16);

            // Subamostragem 4:2:0 para crominância UV
            if ((y % 2 == 0) && (x % 2 == 0)) {
                int uvIndex = (y / 2) * m_width + x;
                pUV[uvIndex + 0] = (uint8_t)(((-38 * r - 74 * g + 112 * b + 128) >> 8) + 128); // U
                pUV[uvIndex + 1] = (uint8_t)(((112 * r - 94 * g - 18 * b + 128) >> 8) + 128);  // V
            }
        }
    }

    pContext->Unmap(m_cpuStagingTexture, 0);

    // Cria o Sample de Entrada do Media Foundation
    IMFSample* pInSample = NULL;
    hr = MFCreateSample(&pInSample);
    if (FAILED(hr)) return false;

    IMFMediaBuffer* pInMediaBuffer = NULL;
    hr = MFCreateMemoryBuffer(nv12TotalSize, &pInMediaBuffer);
    if (FAILED(hr)) { pInSample->Release(); return false; }

    BYTE* pDest = NULL;
    pInMediaBuffer->Lock(&pDest, NULL, NULL);
    memcpy(pDest, nv12Buffer.data(), nv12TotalSize);
    pInMediaBuffer->Unlock();
    pInMediaBuffer->SetCurrentLength(nv12TotalSize);

    pInSample->AddBuffer(pInMediaBuffer);
    pInMediaBuffer->Release();

    // Timestamp de apresentação contínua (100-nanosegundos)
    LONGLONG sampleDuration = (10000000LL / m_fps);
    LONGLONG sampleTime = m_frameIndex * sampleDuration;
    pInSample->SetSampleTime(sampleTime);
    pInSample->SetSampleDuration(sampleDuration);
    m_frameIndex++;

    // Se o encoder ainda tiver dados pendentes, drena primeiro
    auto DrainOutput = [this, nv12TotalSize]() {
        while (true) {
            MFT_OUTPUT_STREAM_INFO osi = {};
            m_encoderMFT->GetOutputStreamInfo(0, &osi);

            MFT_OUTPUT_DATA_BUFFER outBuffer = {};
            IMFSample* pCreatedSample = NULL;

            if (!(osi.dwFlags & (MFT_OUTPUT_STREAM_PROVIDES_SAMPLES | MFT_OUTPUT_STREAM_CAN_PROVIDE_SAMPLES))) {
                MFCreateSample(&pCreatedSample);
                IMFMediaBuffer* pOutMediaBuffer = NULL;
                DWORD bufSize = osi.cbSize > 0 ? osi.cbSize : (DWORD)nv12TotalSize;
                MFCreateMemoryBuffer(bufSize, &pOutMediaBuffer);
                pCreatedSample->AddBuffer(pOutMediaBuffer);
                pOutMediaBuffer->Release();
                outBuffer.pSample = pCreatedSample;
            }

            DWORD status = 0;
            HRESULT hr = m_encoderMFT->ProcessOutput(0, 1, &outBuffer, &status);

            if (outBuffer.pEvents) {
                outBuffer.pEvents->Release();
                outBuffer.pEvents = NULL;
            }

            if (hr == (HRESULT)0xC00D36B1L) { // MF_E_TRANSFORM_STREAM_CHANGE
                if (pCreatedSample) {
                    pCreatedSample->Release();
                    pCreatedSample = NULL;
                }
                IMFMediaType* pNewType = NULL;
                hr = m_encoderMFT->GetOutputAvailableType(0, 0, &pNewType);
                if (SUCCEEDED(hr) && pNewType) {
                    m_encoderMFT->SetOutputType(0, pNewType, 0);
                    pNewType->Release();
                }
                continue;
            }

            if (hr == (HRESULT)0xC00D6D72L) { // MF_E_TRANSFORM_NEED_MORE_INPUT
                if (pCreatedSample) pCreatedSample->Release();
                break;
            }

            if (FAILED(hr)) {
                if (pCreatedSample) pCreatedSample->Release();
                break;
            }

            IMFSample* sampleWithData = outBuffer.pSample ? outBuffer.pSample : pCreatedSample;
            if (sampleWithData) {
                IMFMediaBuffer* buf = NULL;
                sampleWithData->ConvertToContiguousBuffer(&buf);
                if (buf) {
                    BYTE* pData = NULL;
                    DWORD len = 0;
                    buf->Lock(&pData, NULL, &len);

                    if (pData && len > 0 && m_onPacketCb) {
                        UINT32 isCleanPoint = 0;
                        sampleWithData->GetUINT32(MFSampleExtension_CleanPoint, &isCleanPoint);
                        m_onPacketCb((const uint8_t*)pData, (size_t)len, isCleanPoint != 0);
                    }

                    buf->Unlock();
                    buf->Release();
                }
            }

            if (pCreatedSample) {
                pCreatedSample->Release();
                pCreatedSample = NULL;
            }
            if (outBuffer.pSample && outBuffer.pSample != pCreatedSample) {
                outBuffer.pSample->Release();
                outBuffer.pSample = NULL;
            }
        }
    };

    // Tenta ProcessInput
    hr = m_encoderMFT->ProcessInput(0, pInSample, 0);
    if (hr == (HRESULT)0xC00D36B5L) { // MF_E_NOTACCEPTING
        DrainOutput();
        hr = m_encoderMFT->ProcessInput(0, pInSample, 0);
    }
    pInSample->Release();

    if (FAILED(hr)) {
        printf("[H264Encoder] Falha em ProcessInput (hr: 0x%08lX)\n", hr);
        return false;
    }

    // Drena os pacotes NAL codificados
    DrainOutput();
    return true;
}
