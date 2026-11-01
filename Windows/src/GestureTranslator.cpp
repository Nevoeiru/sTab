#include "GestureTranslator.h"
#include <stdio.h>

GestureTranslator::GestureTranslator()
    : m_isOrbitActive(false)
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
    input.ki.wVk = vkCode;
    input.ki.dwFlags = down ? 0 : KEYEVENTF_KEYUP;
    SendInput(1, &input, sizeof(INPUT));
}

void GestureTranslator::TriggerKeyCombination(WORD vk1, WORD vk2) {
    INPUT inputs[4] = {};
    inputs[0].type = INPUT_KEYBOARD; inputs[0].ki.wVk = vk1;
    inputs[1].type = INPUT_KEYBOARD; inputs[1].ki.wVk = vk2;
    inputs[2].type = INPUT_KEYBOARD; inputs[2].ki.wVk = vk2; inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;
    inputs[3].type = INPUT_KEYBOARD; inputs[3].ki.wVk = vk1; inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(4, inputs, sizeof(INPUT));
}

void GestureTranslator::TriggerKeyCombination(WORD vk1, WORD vk2, WORD vk3) {
    INPUT inputs[6] = {};
    inputs[0].type = INPUT_KEYBOARD; inputs[0].ki.wVk = vk1;
    inputs[1].type = INPUT_KEYBOARD; inputs[1].ki.wVk = vk2;
    inputs[2].type = INPUT_KEYBOARD; inputs[2].ki.wVk = vk3;
    inputs[3].type = INPUT_KEYBOARD; inputs[3].ki.wVk = vk3; inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;
    inputs[4].type = INPUT_KEYBOARD; inputs[4].ki.wVk = vk2; inputs[4].ki.dwFlags = KEYEVENTF_KEYUP;
    inputs[5].type = INPUT_KEYBOARD; inputs[5].ki.wVk = vk1; inputs[5].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(6, inputs, sizeof(INPUT));
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
                // Inicia rotação de câmera 3D no Blender
                SendMouseMiddle(true);
                m_isOrbitActive = true;
            } else if (gesturePhase == 1) {
                SendMouseMoveDelta(dx, dy);
            } else if (gesturePhase == 2) {
                SendMouseMiddle(false);
                m_isOrbitActive = false;
            }
            break;

        case GESTURE_PAN_DRAG:
            if (gesturePhase == 0) {
                // Inicia Pan de câmera (Shift + MMB)
                SendKey(VK_SHIFT, true);
                SendMouseMiddle(true);
                m_isPanActive = true;
            } else if (gesturePhase == 1) {
                SendMouseMoveDelta(dx, dy);
            } else if (gesturePhase == 2) {
                SendMouseMiddle(false);
                SendKey(VK_SHIFT, false);
                m_isPanActive = false;
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
            break;

        default:
            break;
    }
}

void GestureTranslator::ProcessSculptPadAction(const InputPacket& packet) {
    uint8_t action = (uint8_t)(packet.payload & 0xFF);

    switch (action) {
        case SCULPTPAD_HOLD_SHIFT_DOWN:
            if (!m_isShiftHeld) {
                SendKey(VK_SHIFT, true);
                m_isShiftHeld = true;
            }
            break;

        case SCULPTPAD_HOLD_SHIFT_UP:
            if (m_isShiftHeld) {
                SendKey(VK_SHIFT, false);
                m_isShiftHeld = false;
            }
            break;

        case SCULPTPAD_HOLD_CTRL_DOWN:
            if (!m_isCtrlHeld) {
                SendKey(VK_CONTROL, true);
                m_isCtrlHeld = true;
            }
            break;

        case SCULPTPAD_HOLD_CTRL_UP:
            if (m_isCtrlHeld) {
                SendKey(VK_CONTROL, false);
                m_isCtrlHeld = false;
            }
            break;

        case SCULPTPAD_HOLD_ALT_DOWN:
            if (!m_isAltHeld) {
                SendKey(VK_MENU, true);
                m_isAltHeld = true;
            }
            break;

        case SCULPTPAD_HOLD_ALT_UP:
            if (m_isAltHeld) {
                SendKey(VK_MENU, false);
                m_isAltHeld = false;
            }
            break;

        case SCULPTPAD_TRIGGER_RADIUS:
            // Dispara tecla 'F' para ajuste de raio
            SendKey('F', true);
            SendKey('F', false);
            break;

        case SCULPTPAD_TRIGGER_STRENGTH:
            // Dispara 'Shift + F' para ajuste de força
            TriggerKeyCombination(VK_SHIFT, 'F');
            break;

        case SCULPTPAD_TRIGGER_REMESH:
            // Dispara 'Ctrl + R' para Voxel Remesh
            TriggerKeyCombination(VK_CONTROL, 'R');
            break;

        case SCULPTPAD_TRIGGER_TRANSFER:
            // Dispara 'Alt + Q' para alternar objeto / subtool no Sculpt Mode
            TriggerKeyCombination(VK_MENU, 'Q');
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
}

