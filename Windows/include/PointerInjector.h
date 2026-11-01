#ifndef STAB_POINTER_INJECTOR_H
#define STAB_POINTER_INJECTOR_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <windows.h>
#include "Protocol.h"

/**
 * @brief Assinaturas nativas da Synthetic Pointer API (user32.dll).
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

/**
 * @brief Injetor nativo de eventos S-Pen via Windows Ink oficial (PT_PEN).
 * Mapeia pressão, inclinação X/Y e converte o botão lateral em clique direito (RMB).
 */
class PointerInjector {
public:
    PointerInjector();
    ~PointerInjector();

    /**
     * @brief Inicializa a API sintética do Windows Ink.
     * @return true se inicializado com sucesso, false caso a API não esteja disponível.
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

    bool IsInitialized() const { return m_initialized; }

private:
    bool LoadSyntheticPointerApis();
    void HandleBarrelButtonAsRMB(bool isButtonPressed, int screenX, int screenY);

    HSYNTHETICPOINTERDEVICE m_hDevice;
    HMODULE m_hUser32;

    pfnCreateSyntheticPointerDevice   m_pfnCreate;
    pfnInjectSyntheticPointerInput    m_pfnInject;
    pfnDestroySyntheticPointerDevice  m_pfnDestroy;

    bool m_initialized;
    bool m_wasTipDown;
    bool m_wasHovering;
    bool m_wasBarrelPressed;

    int m_targetX;
    int m_targetY;
    int m_targetWidth;
    int m_targetHeight;
};

#endif // STAB_POINTER_INJECTOR_H
