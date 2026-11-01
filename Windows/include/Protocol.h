#ifndef STAB_PROTOCOL_H
#define STAB_PROTOCOL_H

#include <stdint.h>

#pragma pack(push, 1)

/**
 * @brief Tipos de pacotes de dados no protocolo binário sTab.
 */
enum PacketType : uint8_t {
    PACKET_TYPE_PEN            = 0x01,  ///< Evento de digitalizador S-Pen (Windows Ink)
    PACKET_TYPE_TOUCH_GESTURE  = 0x02,  ///< Gesto de toque de dedo na tela (Orbit/Pan/Zoom/Undo/Redo)
    PACKET_TYPE_SCULPTPAD      = 0x03,  ///< Modificadores táteis da barra lateral SculptPad
    PACKET_TYPE_VIDEO_CONTROL  = 0x04   ///< Controle de vídeo (Canal reverso REQUEST_IDR)
};

/**
 * @brief Flags de estado para a S-Pen e toques na tela.
 */
enum PacketFlags : uint8_t {
    FLAG_NONE                  = 0x00,
    FLAG_PEN_HOVER             = 0x01,  ///< S-Pen dentro do raio de detecção (~15mm do vidro)
    FLAG_PEN_CONTACT           = 0x02,  ///< Ponta física da S-Pen tocando o vidro
    FLAG_PEN_BARREL_BUTTON     = 0x04,  ///< Botão físico lateral da S-Pen pressionado (Mapeado como RMB)
    FLAG_PEN_INVERTED_ERASER   = 0x08,  ///< Ponta invertida / modo borracha
    FLAG_TOUCH_GESTURE_ACTIVE  = 0x10   ///< Gesto de dedo ativo na área da viewport 3D
};

/**
 * @brief Subtipos de gestos táteis de dedo (PACKET_TYPE_TOUCH_GESTURE).
 */
enum GestureType : uint8_t {
    GESTURE_NONE               = 0x00,
    GESTURE_ORBIT_DRAG         = 0x01,  ///< 1 dedo arrastando -> MMB Orbit no Blender
    GESTURE_PAN_DRAG           = 0x02,  ///< 2 dedos arrastando -> Shift + MMB Pan no Blender
    GESTURE_PINCH_ZOOM         = 0x03,  ///< Pinça de 2 dedos -> Zoom contínuo na viewport
    GESTURE_TAP_TWO_FINGERS    = 0x04,  ///< Toque rápido com 2 dedos -> Undo (Ctrl+Z)
    GESTURE_TAP_THREE_FINGERS  = 0x05,  ///< Toque rápido com 3 dedos -> Redo (Ctrl+Shift+Z)
    GESTURE_STATE_END          = 0x06   ///< Finalização / liberação do gesto ativo
};

/**
 * @brief Ações de modificadores da barra lateral SculptPad (PACKET_TYPE_SCULPTPAD).
 */
enum SculptPadAction : uint8_t {
    SCULPTPAD_NONE             = 0x00,
    SCULPTPAD_HOLD_SHIFT_DOWN  = 0x01,  ///< Polegar encostou em Shift (Smooth Brush)
    SCULPTPAD_HOLD_SHIFT_UP    = 0x02,  ///< Polegar soltou Shift
    SCULPTPAD_HOLD_CTRL_DOWN   = 0x03,  ///< Polegar encostou em Ctrl (Invert/Sub Brush)
    SCULPTPAD_HOLD_CTRL_UP     = 0x04,  ///< Polegar soltou Ctrl
    SCULPTPAD_HOLD_ALT_DOWN    = 0x05,  ///< Polegar encostou em Alt
    SCULPTPAD_HOLD_ALT_UP      = 0x06,  ///< Polegar soltou Alt
    SCULPTPAD_TRIGGER_RADIUS   = 0x07,  ///< Disparo do slider de raio (tecla 'F')
    SCULPTPAD_TRIGGER_STRENGTH = 0x08,  ///< Disparo do slider de força ('Shift + F')
    SCULPTPAD_TRIGGER_REMESH   = 0x09,  ///< Voxel Remesh ('Ctrl + R')
    SCULPTPAD_TRIGGER_TRANSFER = 0x0A   ///< Transfer Mode / Alternar Objeto ('Alt + Q')
};

/**
 * @brief Comandos de controle de vídeo (PACKET_TYPE_VIDEO_CONTROL).
 */
enum VideoControlCommand : uint8_t {
    VIDEO_CMD_NONE             = 0x00,
    VIDEO_CMD_REQUEST_IDR      = 0x01   ///< Solicitação de quadro-chave IDR imediato ao encoder
};

/**
 * @brief Pacote binário ultracompacto e de alta frequência (16 bytes fixos).
 *
 * Transmitido via socket TCP/USB em alta taxa (até 240Hz para S-Pen).
 * Totalmente alinhado a 16 bytes para descarte de overhead de transporte.
 */
struct InputPacket {
    uint8_t  packetType;   ///< Tipo do pacote (PacketType)
    uint8_t  flags;        ///< Bitmask de estado (PacketFlags)
    uint16_t x;            ///< Posição X normalizada (0 a 65535)
    uint16_t y;            ///< Posição Y normalizada (0 a 65535)
    uint16_t pressure;     ///< Pressão da S-Pen normalizada (0 a 4096)
    int16_t  tiltX;        ///< Inclinação horizontal da caneta (-90° a +90°)
    int16_t  tiltY;        ///< Inclinação vertical da caneta (-90° a +90°)
    uint32_t payload;      ///< Dados auxiliares: deltas (dx, dy), códigos de gesto ou ação
};

#pragma pack(pop)

// Garantia matemática em tempo de compilação do tamanho exato de 16 bytes
static_assert(sizeof(InputPacket) == 16, "ERRO CRITICO: InputPacket deve ter exatamente 16 bytes.");

#endif // STAB_PROTOCOL_H

