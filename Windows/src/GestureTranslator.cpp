#include "GestureTranslator.h"
#include <stdio.h>

GestureTranslator::GestureTranslator()
    : m_onNavStateChanged(nullptr)
    , m_onDisplayModeToggle(nullptr)
    , m_isOrbitActive(false)
    , m_isPanActive(false)
    , m_isShiftHeld(false)
    , m_isCtrlHeld(false)
    , m_isAltHeld(false)
    , m_lastTouchX(0)
    , m_lastTouchY(0)
{
}

GestureTranslator::~GestureTranslator() {
    ResetAllInputStates();
}

void GestureTranslator::NotifyNavigationState(bool isNavigating) {
    if (m_onNavStateChanged) {
        m_onNavStateChanged(isNavigating);
    }
}

void GestureTranslator::SendMouseMiddle(bool down) {
    INPUT input = {};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = down ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP;
    SendInput(1, &input, sizeof(INPUT));
}

void GestureTranslator::SendMouseMoveDelta(int dx, int dy) {
    if (dx == 0 && dy == 0) return;
    INPUT input = {};
    input.type = INPUT_MOUSE;
    input.mi.dx = dx;
    input.mi.dy = dy;
    input.mi.dwFlags = MOUSEEVENTF_MOVE;
    SendInput(1, &input, sizeof(INPUT));
}

void GestureTranslator::SendMouseWheel(int wheelDelta) {
    if (wheelDelta == 0) return;
    INPUT input = {};
    input.type = INPUT_MOUSE;
    input.mi.mouseData = (DWORD)wheelDelta;
    input.mi.dwFlags = MOUSEEVENTF_WHEEL;
    SendInput(1, &input, sizeof(INPUT));
}

void GestureTranslator::SendKey(WORD vkCode, bool down) {
    INPUT input = {};
    input.type = INPUT_KEYBOARD;

    // Normaliza modificadores genéricos para a variante esquerda (padrão Blender)
    if (vkCode == VK_SHIFT) vkCode = VK_LSHIFT;
    if (vkCode == VK_CONTROL) vkCode = VK_LCONTROL;
    if (vkCode == VK_MENU) vkCode = VK_LMENU;

    input.ki.wVk = vkCode;
    input.ki.wScan = (WORD)MapVirtualKeyW(vkCode, MAPVK_VK_TO_VSC);
    input.ki.dwFlags = down ? 0 : KEYEVENTF_KEYUP;

    if (vkCode == VK_RCONTROL || vkCode == VK_RMENU) {
        input.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
    }

    SendInput(1, &input, sizeof(INPUT));
}

void GestureTranslator::TriggerSingleKey(WORD vk) {
    SendKey(vk, true);
    Sleep(25);
    SendKey(vk, false);
}

void GestureTranslator::TriggerKeyCombination(WORD vk1, WORD vk2) {
    SendKey(vk1, true);
    Sleep(25);
    SendKey(vk2, true);
    Sleep(25);
    SendKey(vk2, false);
    Sleep(15);
    SendKey(vk1, false);
}

void GestureTranslator::TriggerKeyCombination(WORD vk1, WORD vk2, WORD vk3) {
    SendKey(vk1, true);
    Sleep(15);
    SendKey(vk2, true);
    Sleep(25);
    SendKey(vk3, true);
    Sleep(25);
    SendKey(vk3, false);
    Sleep(15);
    SendKey(vk2, false);
    Sleep(15);
    SendKey(vk1, false);
}

void GestureTranslator::ProcessTouchGesture(const InputPacket& packet) {
    uint8_t gestureType  = (uint8_t)(packet.payload & 0xFF);
    uint8_t gesturePhase = (uint8_t)((packet.payload >> 8) & 0xFF); // 0 = Início, 1 = Movimento, 2 = Fim

    // Em gestos touch, tiltX e tiltY são reutilizados como deltas (dx, dy)
    int dx = (int)packet.tiltX;
    int dy = (int)packet.tiltY;

    switch (gestureType) {
        case GESTURE_ORBIT_DRAG:
            if (gesturePhase == 0) {
                // Inicia rotação de câmera 3D no Blender e ativa trava modal
                NotifyNavigationState(true);
                SendMouseMiddle(true);
                m_isOrbitActive = true;
            } else if (gesturePhase == 1) {
                SendMouseMoveDelta(dx, dy);
            } else if (gesturePhase == 2) {
                SendMouseMiddle(false);
                m_isOrbitActive = false;
                NotifyNavigationState(false);
            }
            break;

        case GESTURE_PAN_DRAG:
            if (gesturePhase == 0) {
                // Inicia Pan de câmera (Shift + MMB) e ativa trava modal
                NotifyNavigationState(true);
                SendKey(VK_SHIFT, true);
                SendMouseMiddle(true);
                m_isPanActive = true;
            } else if (gesturePhase == 1) {
                SendMouseMoveDelta(dx, dy);
            } else if (gesturePhase == 2) {
                SendMouseMiddle(false);
                SendKey(VK_SHIFT, false);
                m_isPanActive = false;
                NotifyNavigationState(false);
            }
            break;

        case GESTURE_PINCH_ZOOM: {
            // Zoom contínuo na câmera usando o delta calculado no tablet
            int wheelDelta = (int)packet.tiltY;
            SendMouseWheel(wheelDelta);
            break;
        }

        case GESTURE_TAP_TWO_FINGERS:
            // 2 dedos na tela = Undo (Ctrl + Z)
            TriggerKeyCombination(VK_CONTROL, 'Z');
            break;

        case GESTURE_TAP_THREE_FINGERS:
            // 3 dedos na tela = Redo (Ctrl + Shift + Z)
            TriggerKeyCombination(VK_CONTROL, VK_SHIFT, 'Z');
            break;

        case GESTURE_STATE_END:
            if (m_isOrbitActive) {
                SendMouseMiddle(false);
                m_isOrbitActive = false;
            }
            if (m_isPanActive) {
                SendMouseMiddle(false);
                SendKey(VK_SHIFT, false);
                m_isPanActive = false;
            }
            NotifyNavigationState(false);
            break;

        default:
            break;
    }
}

void GestureTranslator::ProcessSculptPadAction(const InputPacket& packet) {
    uint8_t action = (uint8_t)(packet.payload & 0xFF);

    switch (action) {
        case SCULPTPAD_HOLD_SHIFT_DOWN:
            SendKey(VK_LSHIFT, true);
            m_isShiftHeld = true;
            break;

        case SCULPTPAD_HOLD_SHIFT_UP:
            SendKey(VK_LSHIFT, false);
            m_isShiftHeld = false;
            break;

        case SCULPTPAD_HOLD_CTRL_DOWN:
            SendKey(VK_LCONTROL, true);
            m_isCtrlHeld = true;
            break;

        case SCULPTPAD_HOLD_CTRL_UP:
            SendKey(VK_LCONTROL, false);
            m_isCtrlHeld = false;
            break;

        case SCULPTPAD_HOLD_ALT_DOWN:
            SendKey(VK_LMENU, true);
            m_isAltHeld = true;
            break;

        case SCULPTPAD_HOLD_ALT_UP:
            SendKey(VK_LMENU, false);
            m_isAltHeld = false;
            break;

        // Atalhos de letra simples (botões de clique rápido)
        case SCULPTPAD_TRIGGER_RADIUS:
        case SCULPTPAD_KEY_F:
            TriggerSingleKey('F');
            break;

        case SCULPTPAD_KEY_A:
            TriggerSingleKey('A');
            break;

        case SCULPTPAD_KEY_Z:
            TriggerSingleKey('Z');
            break;

        case SCULPTPAD_KEY_N:
            TriggerSingleKey('N');
            break;

        case SCULPTPAD_KEY_COMMA:
            TriggerSingleKey(VK_OEM_COMMA);
            break;

        case SCULPTPAD_KEY_PERIOD:
            TriggerSingleKey(VK_OEM_PERIOD);
            break;

        case SCULPTPAD_KEY_QUOTE:
            TriggerSingleKey(VK_OEM_7);
            break;

        case SCULPTPAD_KEY_TAB:
            TriggerSingleKey(VK_TAB);
            break;

        // Combinações de teclas simples
        case SCULPTPAD_COMBO_CTRL_S:
            TriggerKeyCombination(VK_CONTROL, 'S');
            break;

        case SCULPTPAD_COMBO_SHIFT_Z:
            TriggerKeyCombination(VK_SHIFT, 'Z');
            break;

        case SCULPTPAD_TRIGGER_STRENGTH:
        case SCULPTPAD_COMBO_SHIFT_F:
            TriggerKeyCombination(VK_SHIFT, 'F');
            break;

        // Histórico de escultura (Undo / Redo)
        case SCULPTPAD_UNDO:
            TriggerKeyCombination(VK_CONTROL, 'Z');
            break;

        case SCULPTPAD_REDO:
            TriggerKeyCombination(VK_CONTROL, VK_SHIFT, 'Z');
            break;

        case SCULPTPAD_TRIGGER_REMESH:
            TriggerKeyCombination(VK_CONTROL, 'R');
            break;

        case SCULPTPAD_TRIGGER_TRANSFER:
            TriggerKeyCombination(VK_MENU, 'Q');
            break;

        case SCULPTPAD_TOGGLE_DISPLAY_MODE:
            if (m_onDisplayModeToggle) {
                m_onDisplayModeToggle();
            }
            break;

        default:
            break;
    }
}

void GestureTranslator::ResetAllInputStates() {
    if (m_isOrbitActive) {
        SendMouseMiddle(false);
        m_isOrbitActive = false;
    }
    if (m_isPanActive) {
        SendMouseMiddle(false);
        m_isPanActive = false;
    }
    if (m_isShiftHeld) {
        SendKey(VK_SHIFT, false);
        m_isShiftHeld = false;
    }
    if (m_isCtrlHeld) {
        SendKey(VK_CONTROL, false);
        m_isCtrlHeld = false;
    }
    if (m_isAltHeld) {
        SendKey(VK_MENU, false);
        m_isAltHeld = false;
    }
    NotifyNavigationState(false);
}

