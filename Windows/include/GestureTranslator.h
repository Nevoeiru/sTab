#ifndef STAB_GESTURE_TRANSLATOR_H
#define STAB_GESTURE_TRANSLATOR_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <windows.h>
#include <functional>
#include "Protocol.h"

typedef std::function<void(bool isNavigating)> OnModalNavigationChangedCallback;

/**
 * @brief Tradutor de gestos de toque de dedo e comandos da barra lateral SculptPad.
 * Emite eventos Win32 de mouse e teclado (SendInput) otimizados para a câmera do Blender
 * e notifica a trava modal para suprimir injeções concorrentes de caneta.
 */
class GestureTranslator {
public:
    GestureTranslator();
    ~GestureTranslator();

    /**
     * @brief Processa um pacote de gestos táteis de dedo (PACKET_TYPE_TOUCH_GESTURE).
     */
    void ProcessTouchGesture(const InputPacket& packet);

    /**
     * @brief Processa uma ação da barra lateral SculptPad (PACKET_TYPE_SCULPTPAD).
     */
    void ProcessSculptPadAction(const InputPacket& packet);

    /**
     * @brief Define o callback de notificação de início/fim de navegação de câmera 3D.
     */
    void SetNavigationStateCallback(OnModalNavigationChangedCallback cb) { m_onNavStateChanged = cb; }

    /**
     * @brief Libera qualquer tecla ou botão do mouse retido (útil em desconexão do tablet).
     */
    void ResetAllInputStates();

    bool IsModalNavigationActive() const { return m_isOrbitActive || m_isPanActive; }

private:
    void SendMouseMiddle(bool down);
    void SendMouseMoveDelta(int dx, int dy);
    void SendMouseWheel(int wheelDelta);
    void SendKey(WORD vkCode, bool down);
    void TriggerKeyCombination(WORD vk1, WORD vk2);
    void TriggerKeyCombination(WORD vk1, WORD vk2, WORD vk3);
    void NotifyNavigationState(bool isNavigating);

    OnModalNavigationChangedCallback m_onNavStateChanged;

    bool m_isOrbitActive;
    bool m_isPanActive;
    bool m_isShiftHeld;
    bool m_isCtrlHeld;
    bool m_isAltHeld;

    int m_lastTouchX;
    int m_lastTouchY;
};

#endif // STAB_GESTURE_TRANSLATOR_H
