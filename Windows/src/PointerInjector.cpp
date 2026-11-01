#include "PointerInjector.h"
#include <stdio.h>

PointerInjector::PointerInjector()
    : m_hDevice(NULL)
    , m_hUser32(NULL)
    , m_pfnCreate(NULL)
    , m_pfnInject(NULL)
    , m_pfnDestroy(NULL)
    , m_initialized(false)
    , m_wasTipDown(false)
    , m_wasHovering(false)
    , m_wasBarrelPressed(false)
    , m_targetX(0)
    , m_targetY(0)
    , m_targetWidth(1920)
    , m_targetHeight(1080)
{
}

PointerInjector::~PointerInjector() {
    Shutdown();
}

bool PointerInjector::LoadSyntheticPointerApis() {
    m_hUser32 = LoadLibraryW(L"user32.dll");
    if (!m_hUser32) {
        printf("[PointerInjector] Falha ao carregar user32.dll (Erro: %lu)\n", GetLastError());
        return false;
    }

    m_pfnCreate = reinterpret_cast<pfnCreateSyntheticPointerDevice>(reinterpret_cast<void*>(GetProcAddress(m_hUser32, "CreateSyntheticPointerDevice")));
    m_pfnInject = reinterpret_cast<pfnInjectSyntheticPointerInput>(reinterpret_cast<void*>(GetProcAddress(m_hUser32, "InjectSyntheticPointerInput")));
    m_pfnDestroy = reinterpret_cast<pfnDestroySyntheticPointerDevice>(reinterpret_cast<void*>(GetProcAddress(m_hUser32, "DestroySyntheticPointerDevice")));

    if (!m_pfnCreate || !m_pfnInject || !m_pfnDestroy) {
        printf("[PointerInjector] SyntheticPointer APIs nao disponiveis nesta versao do Windows.\n");
        return false;
    }

    return true;
}

bool PointerInjector::Initialize() {
    if (m_initialized) {
        return true;
    }

    if (!LoadSyntheticPointerApis()) {
        return false;
    }

    // Cria um ponteiro sintético do tipo Caneta (PT_PEN) com 1 ponto de contato máximo e sem feedback padrão
    m_hDevice = m_pfnCreate(PT_PEN, 1, POINTER_FEEDBACK_NONE);
    if (!m_hDevice) {
        printf("[PointerInjector] Erro ao criar SyntheticPointerDevice PT_PEN (Erro: %lu)\n", GetLastError());
        return false;
    }

    // Resolução de exibição padrão do monitor primário
    m_targetX = 0;
    m_targetY = 0;
    m_targetWidth = GetSystemMetrics(SM_CXSCREEN);
    m_targetHeight = GetSystemMetrics(SM_CYSCREEN);

    if (m_targetWidth <= 0) m_targetWidth = 1920;
    if (m_targetHeight <= 0) m_targetHeight = 1080;

    m_initialized = true;
    printf("[PointerInjector] Windows Ink Synthetic Pointer inicializado com sucesso (%dx%d).\n",
           m_targetWidth, m_targetHeight);

    return true;
}

void PointerInjector::Shutdown() {
    if (m_hDevice && m_pfnDestroy) {
        m_pfnDestroy(m_hDevice);
        m_hDevice = NULL;
    }

    if (m_hUser32) {
        FreeLibrary(m_hUser32);
        m_hUser32 = NULL;
    }

    m_pfnCreate = NULL;
    m_pfnInject = NULL;
    m_pfnDestroy = NULL;
    m_initialized = false;
    m_wasTipDown = false;
    m_wasHovering = false;
    m_wasBarrelPressed = false;
}

void PointerInjector::SetTargetDisplayBounds(int x, int y, int width, int height) {
    if (width > 0 && height > 0) {
        m_targetX = x;
        m_targetY = y;
        m_targetWidth = width;
        m_targetHeight = height;
    }
}

void PointerInjector::HandleBarrelButtonAsRMB(bool isButtonPressed, int screenX, int screenY) {
    if (isButtonPressed && !m_wasBarrelPressed) {
        // Dispara clique direito (RMB DOWN) para abrir o Brush Context Menu no Blender
        INPUT input = {};
        input.type = INPUT_MOUSE;
        input.mi.dx = (LONG)((screenX * 65535) / GetSystemMetrics(SM_CXSCREEN));
        input.mi.dy = (LONG)((screenY * 65535) / GetSystemMetrics(SM_CYSCREEN));
        input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_RIGHTDOWN;
        SendInput(1, &input, sizeof(INPUT));
        m_wasBarrelPressed = true;
    } else if (!isButtonPressed && m_wasBarrelPressed) {
        // Dispara liberação do clique direito (RMB UP)
        INPUT input = {};
        input.type = INPUT_MOUSE;
        input.mi.dx = (LONG)((screenX * 65535) / GetSystemMetrics(SM_CXSCREEN));
        input.mi.dy = (LONG)((screenY * 65535) / GetSystemMetrics(SM_CYSCREEN));
        input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_RIGHTUP;
        SendInput(1, &input, sizeof(INPUT));
        m_wasBarrelPressed = false;
    }
}

bool PointerInjector::InjectPen(const InputPacket& packet) {
    if (!m_initialized || !m_hDevice || !m_pfnInject) {
        return false;
    }

    // Mapeamento normalizado (0..65535) para as coordenadas de tela do host
    int screenX = m_targetX + (int)(((int64_t)packet.x * m_targetWidth) / 65535);
    int screenY = m_targetY + (int)(((int64_t)packet.y * m_targetHeight) / 65535);

    bool isHovering = (packet.flags & FLAG_PEN_HOVER) != 0;
    bool isTipDown  = (packet.flags & FLAG_PEN_CONTACT) != 0;
    bool isBarrel   = (packet.flags & FLAG_PEN_BARREL_BUTTON) != 0;

    // Gerencia o botão da caneta mapeado como Botão Direito do Mouse (RMB)
    HandleBarrelButtonAsRMB(isBarrel, screenX, screenY);

    POINTER_TYPE_INFO pti = {};
    pti.type = PT_PEN;
    pti.penInfo.pointerInfo.pointerType = PT_PEN;
    pti.penInfo.pointerInfo.pointerId = 0;
    pti.penInfo.pointerInfo.ptPixelLocation.x = screenX;
    pti.penInfo.pointerInfo.ptPixelLocation.y = screenY;

    // Definição da máquina de estados do ponteiro no Windows Ink
    if (isTipDown) {
        if (!m_wasTipDown) {
            pti.penInfo.pointerInfo.pointerFlags = POINTER_FLAG_DOWN | POINTER_FLAG_INRANGE | POINTER_FLAG_INCONTACT;
        } else {
            pti.penInfo.pointerInfo.pointerFlags = POINTER_FLAG_UPDATE | POINTER_FLAG_INRANGE | POINTER_FLAG_INCONTACT;
        }
    } else if (isHovering) {
        if (m_wasTipDown) {
            pti.penInfo.pointerInfo.pointerFlags = POINTER_FLAG_UP | POINTER_FLAG_INRANGE;
        } else {
            pti.penInfo.pointerInfo.pointerFlags = POINTER_FLAG_UPDATE | POINTER_FLAG_INRANGE;
        }
    } else {
        // Caneta se afastou completamente da tela
        if (m_wasTipDown) {
            pti.penInfo.pointerInfo.pointerFlags = POINTER_FLAG_UP;
        } else {
            pti.penInfo.pointerInfo.pointerFlags = POINTER_FLAG_UPDATE;
        }
    }

    // Configuração de flags específicas da S-Pen
    pti.penInfo.penFlags = PEN_FLAG_NONE;
    if (isBarrel) {
        pti.penInfo.penFlags |= PEN_FLAG_BARREL;
    }
    if (packet.flags & FLAG_PEN_INVERTED_ERASER) {
        pti.penInfo.penFlags |= PEN_FLAG_INVERTED;
    }

    // Ativa máscaras para pressão e inclinações X/Y
    pti.penInfo.penMask = PEN_MASK_PRESSURE | PEN_MASK_TILT_X | PEN_MASK_TILT_Y;

    // Normalização da pressão da S-Pen: 0..4096 mapeado para 0..1024 na Windows Ink API
    ULONG mappedPressure = (ULONG)((packet.pressure * 1024ULL) / 4096ULL);
    if (mappedPressure > 1024) mappedPressure = 1024;
    pti.penInfo.pressure = mappedPressure;

    // Clamping de inclinação (-90 a +90 graus)
    int16_t tX = packet.tiltX;
    int16_t tY = packet.tiltY;
    if (tX < -90) tX = -90; else if (tX > 90) tX = 90;
    if (tY < -90) tY = -90; else if (tY > 90) tY = 90;

    pti.penInfo.tiltX = tX;
    pti.penInfo.tiltY = tY;

    BOOL ok = m_pfnInject(m_hDevice, &pti, 1);

    m_wasTipDown = isTipDown;
    m_wasHovering = isHovering;

    return (ok == TRUE);
}
