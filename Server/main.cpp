#include <Windows.h>
#include <iostream>

DWORD MainThread(HMODULE Module)
{
        AllocConsole();
        FILE* Dummy;
        freopen_s(&Dummy, "CONOUT$", "w", stdout);
        freopen_s(&Dummy, "CONIN$", "r", stdin);



        return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
        switch (reason)
        {
                case DLL_PROCESS_ATTACH:
                CreateThread(0, 0, (LPTHREAD_START_ROUTINE)MainThread, hModule, 0, 0);
                break;
        }

        return TRUE;
}
