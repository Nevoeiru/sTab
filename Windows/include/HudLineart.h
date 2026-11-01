#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

/**
 * @brief Utilitários modulares para desenho de ícones Lineart e geométricos na HUD Win32.
 * Atende às diretrizes estéticas: estilo lineart puro, cores neon e ausência de estrelas de 4 pontas.
 */
class HudLineart {
public:
    static void DrawStylus(HDC hdc, int x, int y);
    static void DrawDisplay(HDC hdc, int x, int y);
    static void DrawBlenderCube(HDC hdc, int x, int y);
    static void DrawOmnitomeEmblem(HDC hdc, int x, int y);
};

