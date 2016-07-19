/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org/>
*/

#include <windows.h>
#include <stdio.h>
#include <tlhelp32.h>

// http://stackoverflow.com/questions/865152/how-can-i-get-a-process-handle-by-its-name-in-c

HANDLE GetProcessByName(TCHAR* name)
{
    DWORD pid = 0;

    // Create toolhelp snapshot.
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 process;
    ZeroMemory(&process, sizeof(process));
    process.dwSize = sizeof(process);

    // Walkthrough all processes.
    if (Process32First(snapshot, &process))
    {
        do
        {
            // Compare process.szExeFile based on format of name, i.e., trim file path
            // trim .exe if necessary, etc.
            if (stricmp(process.szExeFile, name) == 0)
            {
                pid = process.th32ProcessID;
                break;
            }
        } while(Process32Next(snapshot, &process));
    }

    CloseHandle(snapshot);

    if (pid != 0)
    {
        return OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    }

    // Not found


    return NULL;
}

char o_t[40] = "";
HWND hwnd;
DWORD oldplayerstate = 0;
DWORD playerstate = 0;
DWORD deathctr = 0;
DWORD deathctrtotal = 0;
DWORD bustedctr = 0;
DWORD bustedctrtotal = 0;

void write_txt() {
    char n_t[40];
    char wasted_t[20];
    char busted_t[20];
    if (deathctr == deathctrtotal)
        snprintf(wasted_t, sizeof(wasted_t), "Wasted: %d", deathctr);
    else
        snprintf(wasted_t, sizeof(wasted_t), "Wasted: %d (%d)", deathctr, deathctrtotal);
    if (bustedctr == bustedctrtotal)
        snprintf(busted_t, sizeof(busted_t), "Busted: %d", bustedctr);
    else
        snprintf(busted_t, sizeof(busted_t), "Busted: %d (%d)", bustedctr, bustedctrtotal);
    snprintf(n_t, sizeof(n_t), "%-19s\n%-19s", wasted_t, busted_t);
    if (strcmp(o_t, n_t) != 0) {
        strcpy(o_t, n_t);
        InvalidateRect(hwnd, NULL, TRUE);
    }
}

typedef struct {
    BYTE s;
    BYTE unk1;
    BYTE m;
    BYTE h;
    BYTE unk2; // day?
    BYTE unk3;
    BYTE unk4;
    BYTE unk5;
    DWORD t_ms;
} daytime;

DWORD WINAPI dc_thread(LPVOID lpParameter) {
    HANDLE proc;
    do {
        Sleep(40);
        LPCVOID playerptr;
        daytime d;
        while(
            (proc = GetProcessByName("gta_sa.exe")) &&
            ReadProcessMemory(proc, (LPCVOID)0xB70150, &d, sizeof(daytime), NULL)
        ) {
            if (
                ReadProcessMemory(proc, (LPCVOID)0xB6F5F0, &playerptr, sizeof(playerptr), NULL) &&
                ReadProcessMemory(proc, playerptr+0x530, &playerstate, sizeof(playerstate), NULL)
            ) {
                if (playerstate != oldplayerstate) {
                    oldplayerstate = playerstate;
                    if (playerstate == 55) {
                        deathctr++;
                        deathctrtotal++;
                    }
                    if (playerstate == 63) {
                        bustedctr++;
                        bustedctrtotal++;
                    }
                    write_txt();
                }
            }
            CloseHandle(proc);
            if (d.t_ms == 0 && d.h == 8 && d.m == 0 && d.s == 0 && d.unk2 == 1) {
                deathctr = 0;
                bustedctr = 0;
                write_txt();
            }
            Sleep(40);
        }
        if (proc) {
            CloseHandle(proc);
            write_txt();
        }
    } while(1);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static HFONT hf;
    if (!hf) {
        AddFontResourceEx("DejaVuSansMono.ttf",
                          FR_PRIVATE,
                          NULL);
        hf = CreateFont(30, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "DejaVu Sans Mono");
    }
    switch(message)
    {
    case WM_CHAR:
    {
        if(wParam==VK_ESCAPE)
            SendMessage(hwnd,WM_CLOSE,0,0);
        return 0;
    }
    case WM_PAINT:
    {
        COLORREF g_white = RGB(255, 255, 255);
        COLORREF g_black = RGB(1, 1, 1);
        PAINTSTRUCT ps;
        HDC hDC;
        hDC=BeginPaint(hwnd,&ps);
        HFONT hfOld = SelectObject(hDC, hf);
        SetBkMode(hDC, TRANSPARENT);
        int o[]= {-2, -1, 1, 2, 0};
        RECT hwnd_rc;
        GetClientRect(hwnd, &hwnd_rc);
        for(int x=0; x<5; x++) {
            for(int y=0; y<5; y++) {
                RECT rc;
                SetRect(&rc, hwnd_rc.left + o[x] + 2, hwnd_rc.top + o[y] + 2, hwnd_rc.right, hwnd_rc.bottom);
                SetTextColor(hDC, o[x]==0&&o[y]==0?g_white:g_black);
                DrawText(hDC,o_t,strlen(o_t),&rc,DT_LEFT | DT_EXTERNALLEADING);
            }
        }
        SelectObject(hDC, hfOld);
        EndPaint(hwnd,&ps);
        return 0;
    }
    case WM_DESTROY:
    {
        PostQuitMessage(0);
        return 0;
    }
    }

    return DefWindowProc (hwnd, message, wParam, lParam);
}


int WINAPI WinMain(HINSTANCE hIns, HINSTANCE hPrev, LPSTR lpszArgument, int nCmdShow)
{
    char szClassName[]="GTA SA Death/Bust Counter";
    WNDCLASSEX wc;
    MSG messages;

    wc.hInstance=hIns;
    wc.lpszClassName=szClassName;
    wc.lpfnWndProc = WndProc;
    wc.style = CS_DBLCLKS;
    wc.cbSize = sizeof (WNDCLASSEX);
    wc.hIcon = LoadIcon (NULL, IDI_APPLICATION);
    wc.hIconSm = LoadIcon (NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor (NULL, IDC_ARROW);
    wc.lpszMenuName = NULL;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hbrBackground=CreateSolidBrush(RGB(0, 0, 0));
    RegisterClassEx(&wc);
    hwnd=CreateWindow(szClassName,szClassName,WS_OVERLAPPEDWINDOW,0,0,316,94,HWND_DESKTOP,NULL,hIns,NULL);
    ShowWindow(hwnd, nCmdShow);
    HANDLE thread = CreateThread(0, 0, dc_thread, NULL, 0, NULL);
    while(GetMessage(&messages, NULL, 0, 0))
    {
        TranslateMessage(&messages);
        DispatchMessage(&messages);
    }
    CloseHandle(thread);
    return messages.wParam;
}
