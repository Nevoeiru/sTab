#ifndef STAB_POINTER_INJECTOR_H
#define STAB_POINTER_INJECTOR_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <windows.h>
#include "Protocol.h"

// Flags oficiais do Microsoft Tablet PC Pen Service Property para supressão de latência
#define TABLET_DISABLE_PRESSANDHOLD        0x00000001
#define TABLET_DISABLE_PENTAPFEEDBACK      0x00000008
#define TABLET_DISABLE_PENBARRELFEEDBACK   0x00000010
#define TABLET_DISABLE_TOUCHUIFORCEON      0x00000100
#define TABLET_DISABLE_TOUCHUIFORCEOFF     0x00000200
#define TABLET_DISABLE_TOUCHSWITCH         0x00008000
#define TABLET_DISABLE_FLICKS              0x00010000
#define TABLET_ENABLE_FLICKSONOFF          0x00020000
#define TABLET_FORCE_FLAT_PENINPUT         0x00040000

#ifndef FEEDBACK_PEN_PRESSANDHOLD
#define FEEDBACK_PEN_PRESSANDHOLD 3
#endif

#ifndef POINTER_FLAG_FIRSTBUTTON
#define POINTER_FLAG_FIRSTBUTTON 0x00000010
#endif

#ifndef POINTER_FLAG_SECONDBUTTON
#define POINTER_FLAG_SECONDBUTTON 0x00000020
#endif

#ifndef POINTER_FLAG_CONFIDENCE
#define POINTER_FLAG_CONFIDENCE 0x00000400
#endif

#ifndef POINTER_FLAG_PRIMARY
#define POINTER_FLAG_PRIMARY 0x00002000
#endif

#ifndef POINTER_CHANGE_FIRSTBUTTON_DOWN
#define POINTER_CHANGE_FIRSTBUTTON_DOWN 1
#endif

#ifndef POINTER_CHANGE_FIRSTBUTTON_UP
#define POINTER_CHANGE_FIRSTBUTTON_UP 2
#endif

/**
 * @brief Assinaturas nativas da Synthetic Pointer API e Feedback API (user32.dll).
 * Carregadas dinamicamente em tempo de execução para máxima compatibilidade.
 */
typedef HSYNTHETICPOINTERDEVICE (WINAPI *pfnCreateSyntheticPointerDevice)(
    POINTER_INPUT_TYPE pointerType,
    ULONG maxCount,
    POINTER_FEEDBACK_MODE mode
);

typedef BOOL (WINAPI *pfnInjectSyntheticPointerInput)(
    HSYNTHETICPOINTERDEVICE device,
    const POINTER_TYPE_INFO *pointerInfo,
    ULONG count
);

typedef VOID (WINAPI *pfnDestroySyntheticPointerDevice)(
    HSYNTHETICPOINTERDEVICE device
);

typedef BOOL (WINAPI *pfnSetWindowFeedbackSetting)(
    HWND hwnd,
    DWORD feedback,
    DWORD dwFlags,
    UINT32 size,
    const VOID *configuration
);

/**
 * @brief Injetor nativo de eventos S-Pen via Windows Ink oficial (PT_PEN).
 * Mapeia pressão, inclinação X/Y, converte botão lateral em RMB, neutraliza
 * a latência de 150-250ms de Press-and-Hold e gerencia a trava modal contra MMB.
 */
class PointerInjector {
public:
    PointerInjector();
    ~PointerInjector();

    /**
     * @brief Inicializa a API sintética do Windows Ink.
     * @return true se inicializado com sucesso.
     */
    bool Initialize();

    /**
     * @brief Finaliza e libera o dispositivo virtual sintético.
     */
    void Shutdown();

    /**
     * @brief Injeta um pacote de dados da S-Pen como evento nativo de caneta.
     * @param packet Pacote com coordenadas, pressão e inclinação.
     * @return true se injetado com sucesso.
     */
    bool InjectPen(const InputPacket& packet);

    /**
     * @brief Define a área de tela alvo para o mapeamento de coordenadas.
     */
    void SetTargetDisplayBounds(int x, int y, int width, int height);

    /**
     * @brief Anula a latência de 150-250ms de Press-and-Hold e feedback visual na janela alvo.
     * @param hWnd Janela que receberá os eventos do Blender/escultura.
     */
    void SuppressPressAndHoldLatency(HWND hWnd);

    /**
     * @brief Define o estado da trava modal de navegação de câmera 3D.
     * Quando ativa, suspende a injeção da S-Pen para evitar traços acidentais com Shift/Ctrl.
     */
    void SetModalNavigationActive(bool active);

    bool IsModalNavigationActive() const { return m_isModalNavigationActive; }
    bool IsInitialized() const { return m_initialized; }

private:
    bool LoadSyntheticPointerApis();
    void HandleBarrelButtonAsRMB(bool isButtonPressed, int screenX, int screenY);
    void EnsureWindowLatencySuppressed(int screenX, int screenY);

    HSYNTHETICPOINTERDEVICE m_hDevice;
    HMODULE m_hUser32;

    pfnCreateSyntheticPointerDevice   m_pfnCreate;
    pfnInjectSyntheticPointerInput    m_pfnInject;
    pfnDestroySyntheticPointerDevice  m_pfnDestroy;
    pfnSetWindowFeedbackSetting       m_pfnSetFeedback;

    bool m_initialized;
    bool m_wasTipDown;
    bool m_wasHovering;
    bool m_wasBarrelPressed;
    bool m_isModalNavigationActive;

    HWND m_lastSuppressedHwnd;

    int m_targetX;
    int m_targetY;
    int m_targetWidth;
    int m_targetHeight;
};

#endif // STAB_POINTER_INJECTOR_H
