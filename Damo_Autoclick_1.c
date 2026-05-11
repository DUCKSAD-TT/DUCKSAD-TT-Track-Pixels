#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

#define PW_RENDERFULLCONTENT 0x00000002

typedef struct
{
    int x, y;
    int width, height;
} Region;

void movemouseto(int x, int y)
{
    INPUT input = {0};
    input.type = INPUT_MOUSE;
    input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
    input.mi.dx = x * 65535 / GetSystemMetrics(SM_CXSCREEN);
    input.mi.dy = y * 65535 / GetSystemMetrics(SM_CYSCREEN);
    SendInput(1, &input, sizeof(INPUT));
}

void leftclick()
{
    INPUT input[2] = {0};

    input[0].type = INPUT_MOUSE;
    input[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;

    input[1].type = INPUT_MOUSE;
    input[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;

    SendInput(2, input, sizeof(INPUT));
}

void backgroundclick(HWND hwnd, int x, int y)
{
    LPARAM lParam = MAKELPARAM(x, y);

    PostMessage(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, lParam);
    Sleep(10);
    PostMessage(hwnd, WM_LBUTTONUP, 0, lParam);
}

HBITMAP captureRegion(HWND hwnd, Region r)
{
    HDC hdcWindow = GetWindowDC(hwnd);
    HDC hdcMem = CreateCompatibleDC(hdcWindow);

    RECT rect;
    GetWindowRect(hwnd, &rect);
    int fullW = rect.right - rect.left;
    int fullH = rect.bottom - rect.top;

    // capture
    HBITMAP hFull = CreateCompatibleBitmap(hdcWindow, fullW, fullH);
    SelectObject(hdcMem, hFull);

    PrintWindow(hwnd, hdcMem, PW_RENDERFULLCONTENT);

    HDC hdcCrop = CreateCompatibleDC(hdcWindow);
    HBITMAP hCrop = CreateCompatibleBitmap(hdcWindow, r.width, r.height);
    SelectObject(hdcCrop, hCrop);
    BitBlt(hdcCrop, 0, 0, r.width, r.height, hdcMem, r.x, r.y, SRCCOPY);

    DeleteDC(hdcCrop);
    DeleteDC(hdcMem);
    DeleteObject(hFull);
    ReleaseDC(hwnd, hdcWindow);
    return hCrop;
}

COLORREF *getBitmapPixels(HBITMAP hBitmap, int width, int height)
{
    COLORREF *pixels = malloc(width * height * sizeof(COLORREF));
    BITMAPINFOHEADER bi = {0};
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = width;
    bi.biHeight = -height;
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = BI_RGB;
    HDC hdc = GetDC(NULL);
    GetDIBits(hdc, hBitmap, 0, height, pixels, (BITMAPINFO *)&bi, DIB_RGB_COLORS);
    ReleaseDC(NULL, hdc);
    return pixels;
}

BOOL findColorPosition(COLORREF *pixels, Region r, COLORREF targetColor, int tolerance, int *foundX, int *foundY)
{
    int tR = GetRValue(targetColor);
    int tG = GetGValue(targetColor);
    int tB = GetBValue(targetColor);
    for (int y = 0; y < r.height; y++)
    {
        for (int x = 0; x < r.width; x++)
        {
            COLORREF c = pixels[y * r.width + x];
            int check = tB + 50;
            if (abs(GetRValue(c) - tR) <= tolerance && abs(GetGValue(c) - tG) <= tolerance && abs(tR - tG) <= 3 && (abs(GetBValue(c) - tB) <= 2 || abs(GetBValue(c) - check) <= 2))
            {
                *foundX = x + r.x;
                *foundY = y + r.y;
                return TRUE;
            }
        }
    }
    return FALSE;
}

int main()
{
    HWND hwnd = FindWindow(NULL, "Roblox");
    if (hwnd == NULL)
    {
        printf("Not Found Roblox Page :<\n");
        return 1;
    }
    printf("Found Roblox Page :>\n");
    RECT rect;
    GetWindowRect(hwnd, &rect);
    int realW = rect.right - rect.left;
    int realH = rect.bottom - rect.top;
    printf("Real Format: %d x %d\n", realW, realH);

    int regionW = (int)(realW * 0.55);
    int regionH = (int)(realH * 0.55);

    Region searchArea = {
        .x = (realW - regionW) / 2, // เริ่มจากกลางหน้าต่าง
        .y = (realH - regionH) / 2,
        .width = regionW,
        .height = regionH};
    Sleep(1000);
    printf("Process Area: x=%d y=%d w=%d h=%d\n", searchArea.x, searchArea.y, searchArea.width, searchArea.height);
    printf("\n[\tGuid This Program\t]\n\n1.Pick up your Tideclaw.\n2.Move your character to location to be excavated.\n3.Press [F3] to start process.\n");
    printf("\n[\tWARNING\t\t]\n\nThe working process is just automatic pressing.\nMove your charecter\t[None]\nAuto Sell\t\t[None]\n");
    printf("\n<--- [Keybinds] --->\n[F2] start auto click\n[F3] Find color at mouse local\n[F10] exit program\n");
    printf("Wait order eiei :>\n");
    

    COLORREF targetcolor = RGB(60, 60, 253);
    int tolerance = 33;
    BOOL keyheld = FALSE;
    BOOL isrunning = FALSE;
    printf("\nRGB(%d,%d,%d)\ntolerance : %d\n", GetRValue(targetcolor), GetGValue(targetcolor), GetBValue(targetcolor), tolerance);
    DWORD lastclicktime = GetTickCount();

    while (1)
    {
        if (GetAsyncKeyState(VK_F10) & 0x8000)
        {
            printf("Bye!");
            break;
        }
        // กด F3 เพื่อดูสีใต้เมาส์ใน region
        if (GetAsyncKeyState(VK_F3) & 0x8000)
        {
            POINT p;
            GetCursorPos(&p);

            // แปลงพิกัดหน้าจอ → พิกัดภายในหน้าต่าง
            RECT rect2;
            GetWindowRect(hwnd, &rect2);
            int localX = p.x - rect2.left;
            int localY = p.y - rect2.top;

            // capture แล้วอ่านสีตรงนั้น
            HBITMAP hSnap = captureRegion(hwnd, searchArea);
            COLORREF *snapP = getBitmapPixels(hSnap, searchArea.width, searchArea.height);

            if (localX >= 0 && localX < searchArea.width &&
                localY >= 0 && localY < searchArea.height)
            {
                COLORREF c = snapP[localY * searchArea.width + localX];
                printf("local(%d,%d) = RGB(%d,%d,%d)\n",
                       localX, localY,
                       GetRValue(c), GetGValue(c), GetBValue(c));
            }
            else
            {
                printf("mouse out region\n");
            }

            free(snapP);
            DeleteObject(hSnap);
            Sleep(300);
        }
        if (GetAsyncKeyState(VK_F2) & 0x8000)
        {
            if (!keyheld)
            {
                keyheld = TRUE;
                isrunning = !isrunning;
                printf(isrunning ? "[ON]\n" : "[OFF]\n");
            }
        }
        else
            keyheld = FALSE;
        if (isrunning)
        {
            HBITMAP hBitmap = captureRegion(hwnd, searchArea);
            COLORREF *pixels = getBitmapPixels(hBitmap, searchArea.width, searchArea.height);

            int foundX, foundY;
            if (findColorPosition(pixels, searchArea, targetcolor, tolerance, &foundX, &foundY))
            {
                RECT winRect;
                GetWindowRect(hwnd, &winRect);
                int screenX = foundX + winRect.left;
                int screenY = foundY + winRect.top;

                movemouseto(screenX, screenY);
                leftclick();
                lastclicktime = GetTickCount();
                Sleep(300);
            }
            else
            {
                DWORD now = GetTickCount();
                if (now - lastclicktime >= 4000)
                {
                    leftclick();
                    lastclicktime = GetTickCount();
                }
            }

            free(pixels);
            pixels = NULL;
            DeleteObject(hBitmap);
            // Sleep(5);
        }
        else
        {
            lastclicktime = GetTickCount();
            Sleep(10);
        }
    }
    return 0;
}