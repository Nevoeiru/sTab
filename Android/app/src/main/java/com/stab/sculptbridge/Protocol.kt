package com.stab.sculptbridge

import java.nio.ByteBuffer
import java.nio.ByteOrder

/**
 * Protocolo binário sTab (SculptBridge) - Espelho 1:1 do Protocol.h em C++.
 * Todos os pacotes possuem exatamente 16 bytes e operam em Little-Endian.
 */
object Protocol {
    // Tipos de Pacotes (packetType)
    const val PACKET_TYPE_PEN: Byte           = 0x01
    const val PACKET_TYPE_TOUCH_GESTURE: Byte = 0x02
    const val PACKET_TYPE_SCULPTPAD: Byte     = 0x03
    const val PACKET_TYPE_VIDEO_CONTROL: Byte = 0x04

    // Flags de Estado (flags)
    const val FLAG_NONE: Byte                 = 0x00
    const val FLAG_PEN_HOVER: Byte            = 0x01  // S-Pen no raio de aproximação (~15mm)
    const val FLAG_PEN_CONTACT: Byte          = 0x02  // Ponta da S-Pen tocando o vidro
    const val FLAG_PEN_BARREL_BUTTON: Byte    = 0x04  // Botão lateral pressionado (Mapeado como RMB)
    const val FLAG_PEN_INVERTED_ERASER: Byte  = 0x08  // Ponta invertida / borracha
    const val FLAG_TOUCH_GESTURE_ACTIVE: Byte = 0x10  // Gesto de dedo ativo na viewport

    // Tipos de Gestos de Dedo (payload byte 0)
    const val GESTURE_NONE: Byte              = 0x00
    const val GESTURE_ORBIT_DRAG: Byte        = 0x01  // 1 dedo arrastando -> MMB Orbit no Blender
    const val GESTURE_PAN_DRAG: Byte          = 0x02  // 2 dedos arrastando -> Shift + MMB Pan no Blender
    const val GESTURE_PINCH_ZOOM: Byte        = 0x03  // Pinça 2 dedos -> Zoom contínuo na viewport
    const val GESTURE_TAP_TWO_FINGERS: Byte   = 0x04  // 2 toques rápidos -> Undo (Ctrl+Z)
    const val GESTURE_TAP_THREE_FINGERS: Byte = 0x05  // 3 toques rápidos -> Redo (Ctrl+Shift+Z)
    const val GESTURE_STATE_END: Byte         = 0x06  // Finalização do gesto

    // Ações do SculptPad (payload byte 0)
    const val SCULPTPAD_NONE: Byte            = 0x00
    const val SCULPTPAD_HOLD_SHIFT_DOWN: Byte = 0x01  // Smooth Brush
    const val SCULPTPAD_HOLD_SHIFT_UP: Byte   = 0x02
    const val SCULPTPAD_HOLD_CTRL_DOWN: Byte  = 0x03  // Invert/Sub Brush
    const val SCULPTPAD_HOLD_CTRL_UP: Byte    = 0x04
    const val SCULPTPAD_HOLD_ALT_DOWN: Byte   = 0x05
    const val SCULPTPAD_HOLD_ALT_UP: Byte     = 0x06
    const val SCULPTPAD_TRIGGER_RADIUS: Byte  = 0x07  // 'F'
    const val SCULPTPAD_TRIGGER_STRENGTH: Byte= 0x08  // 'Shift + F'
    const val SCULPTPAD_TRIGGER_REMESH: Byte  = 0x09  // 'Ctrl + R'
    const val SCULPTPAD_TRIGGER_TRANSFER: Byte= 0x0A  // 'Alt + Q'
    const val SCULPTPAD_TOGGLE_DISPLAY_MODE: Byte = 0x0B // Alterna entre Estendido e Espelhado
    const val SCULPTPAD_KEY_A: Byte           = 0x10  // Tecla 'A' (Mask Edit)
    const val SCULPTPAD_KEY_F: Byte           = 0x11  // Tecla 'F' (Tamanho do Pincel)
    const val SCULPTPAD_KEY_Z: Byte           = 0x12  // Tecla 'Z' (Modo de Render / Shading Pie)
    const val SCULPTPAD_KEY_N: Byte           = 0x13  // Tecla 'N' (Transformações)
    const val SCULPTPAD_KEY_COMMA: Byte       = 0x14  // Tecla '<' (Orientação)
    const val SCULPTPAD_KEY_PERIOD: Byte      = 0x15  // Tecla '>' (Pivô)
    const val SCULPTPAD_KEY_QUOTE: Byte       = 0x16  // Tecla '\'' (Visualizações)
    const val SCULPTPAD_KEY_TAB: Byte         = 0x17  // Tecla 'Tab' (Edit / Sculpt)
    const val SCULPTPAD_COMBO_CTRL_S: Byte    = 0x18  // 'Ctrl + S' (Salvar)
    const val SCULPTPAD_COMBO_SHIFT_Z: Byte   = 0x19  // 'Shift + Z' (Arestas / Wireframe)
    const val SCULPTPAD_COMBO_SHIFT_F: Byte   = 0x1A  // 'Shift + F' (Força do Pincel)
    const val SCULPTPAD_UNDO: Byte            = 0x1B  // 'Ctrl + Z' (Desfazer)
    const val SCULPTPAD_REDO: Byte            = 0x1C  // 'Ctrl + Shift + Z' (Refazer)

    // Comandos de Controle de Vídeo
    const val VIDEO_CMD_REQUEST_IDR: Byte     = 0x01  // Solicitação imediata de quadro-chave

    const val PACKET_SIZE: Int                = 16

    /**
     * Gera um pacote de controle de vídeo de 16 bytes para solicitar IDR frame ao Windows.
     */
    fun createVideoControlPacket(command: Byte): InputPacket {
        return InputPacket(
            packetType = PACKET_TYPE_VIDEO_CONTROL,
            flags = FLAG_NONE,
            x = 0,
            y = 0,
            pressure = 0,
            tiltX = 0,
            tiltY = 0,
            payload = command.toInt() and 0xFF
        )
    }
}

/**
 * Pacote binário ultraleve de 16 bytes enviado ao servidor Windows via socket USB.
 */
data class InputPacket(
    val packetType: Byte,
    val flags: Byte,
    val x: Int,           // 0..65535 (UInt16)
    val y: Int,           // 0..65535 (UInt16)
    val pressure: Int,    // 0..4096 (UInt16)
    val tiltX: Short,     // -90..+90 graus (ou delta dx)
    val tiltY: Short,     // -90..+90 graus (ou delta dy)
    val payload: Int      // Ação/Gesto/Comando
) {
    /**
     * Serializa a struct diretamente em 16 bytes Little-Endian.
     */
    fun toByteArray(): ByteArray {
        val buffer = ByteBuffer.allocate(Protocol.PACKET_SIZE).order(ByteOrder.LITTLE_ENDIAN)
        buffer.put(packetType)
        buffer.put(flags)
        buffer.putShort((x and 0xFFFF).toShort())
        buffer.putShort((y and 0xFFFF).toShort())
        buffer.putShort((pressure and 0xFFFF).toShort())
        buffer.putShort(tiltX)
        buffer.putShort(tiltY)
        buffer.putInt(payload)
        return buffer.array()
    }
}

