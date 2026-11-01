#ifndef STAB_GESTURE_TRANSLATOR_H
#define STAB_GESTURE_TRANSLATOR_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <windows.h>
#include "Protocol.h"

/**
 * @brief Tradutor de gestos de toque de dedo e comandos da barra lateral SculptPad.
 * Emite eventos Win32 de mouse e teclado (SendInput) otimizados para a câmera do Blender.
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
     * @brief Libera qualquer tecla ou botão do mouse retido (útil em desconexão do tablet).
     */
    void ResetAllInputStates();

private:
    void SendMouseMiddle(bool down);
    void SendMouseMoveDelta(int dx, int dy);
    void SendMouseWheel(int wheelDelta);
    void SendKey(WORD vkCode, bool down);
    void TriggerKeyCombination(WORD vk1, WORD vk2);
    void TriggerKeyCombination(WORD vk1, WORD vk2, WORD vk3);

    bool m_isOrbitActive;
    bool m_isPanActive;
    bool m_isShiftHeld;
    bool m_isCtrlHeld;
    bool m_isAltHeld;

    int m_lastTouchX;
    int m_lastTouchY;
};

#endif // STAB_GESTURE_TRANSLATOR_H
