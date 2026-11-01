#include "HudLineart.h"

void HudLineart::DrawStylus(HDC hdc, int x, int y) {
    HPEN pen = CreatePen(PS_SOLID, 2, RGB(0, 229, 255));
    HGDIOBJ old = SelectObject(hdc, pen);
    // Corpo da caneta e ponta
    POINT pts[5] = { {x, y + 16}, {x + 12, y + 4}, {x + 16, y + 8}, {x + 4, y + 20}, {x, y + 16} };
    Polyline(hdc, pts, 5);
    MoveToEx(hdc, x, y + 16, NULL); LineTo(hdc, x - 3, y + 19);
    SelectObject(hdc, old);
    DeleteObject(pen);
}

void HudLineart::DrawDisplay(HDC hdc, int x, int y) {
    HPEN pen = CreatePen(PS_SOLID, 2, RGB(0, 230, 118));
    HGDIOBJ old = SelectObject(hdc, pen);
    // Monitor e base
    Rectangle(hdc, x, y, x + 20, y + 14);
    MoveToEx(hdc, x + 10, y + 14, NULL); LineTo(hdc, x + 10, y + 18);
    MoveToEx(hdc, x + 5, y + 18, NULL);  LineTo(hdc, x + 15, y + 18);
    SelectObject(hdc, old);
    DeleteObject(pen);
}

void HudLineart::DrawBlenderCube(HDC hdc, int x, int y) {
    HPEN pen = CreatePen(PS_SOLID, 2, RGB(255, 179, 0));
    HGDIOBJ old = SelectObject(hdc, pen);
    // Cubo isométrico
    MoveToEx(hdc, x + 10, y, NULL);      LineTo(hdc, x + 20, y + 5);
    LineTo(hdc, x + 10, y + 10);         LineTo(hdc, x, y + 5);
    LineTo(hdc, x + 10, y);
    MoveToEx(hdc, x, y + 5, NULL);       LineTo(hdc, x, y + 15);
    LineTo(hdc, x + 10, y + 20);         LineTo(hdc, x + 20, y + 15);
    LineTo(hdc, x + 20, y + 5);
    MoveToEx(hdc, x + 10, y + 10, NULL); LineTo(hdc, x + 10, y + 20);
    SelectObject(hdc, old);
    DeleteObject(pen);
}

void HudLineart::DrawOmnitomeEmblem(HDC hdc, int x, int y) {
    HPEN pen = CreatePen(PS_SOLID, 2, RGB(0, 229, 255));
    HGDIOBJ old = SelectObject(hdc, pen);
    // Emblema geométrico Omnitome (hexágono estilizado com prisma, sem estrelas)
    POINT hex[7] = {
        {x + 10, y}, {x + 20, y + 5}, {x + 20, y + 15},
        {x + 10, y + 20}, {x, y + 15}, {x, y + 5}, {x + 10, y}
    };
    Polyline(hdc, hex, 7);
    MoveToEx(hdc, x + 10, y, NULL);      LineTo(hdc, x + 10, y + 20);
    MoveToEx(hdc, x, y + 10, NULL);      LineTo(hdc, x + 20, y + 10);
    SelectObject(hdc, old);
    DeleteObject(pen);
}

