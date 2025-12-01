#include <Windows.h>
#include <iostream>

#include <Utils.hpp>
#include <Hook.hpp>

bool (*UWorldExecOriginal)(__int64 a1, __int64 a2, const wchar_t* cmd, __int64 a4);
bool UWorldExecHook(__int64 a1, __int64 a2, const wchar_t* cmd, __int64 a4)
{
    if (wcscmp(cmd, L"givemecheats") == 0)
    {
        auto PlayerController = Utils::GetLocalPlayerController();
        PlayerController->CheatManager = Utils::SpawnObject<UCheatManager>(PlayerController);
        return true;
    }

    return UWorldExecOriginal(a1, a2, cmd, a4);
}

DWORD MainThread(HMODULE Module)
{
        AllocConsole();
        FILE* Dummy;
        freopen_s(&Dummy, "CONOUT$", "w", stdout);
        freopen_s(&Dummy, "CONIN$", "r", stdin);

        auto GameViewport = UEngine::GetEngine()->GameViewport;
        GameViewport->ViewportConsole = Utils::SpawnObject<UConsole>(GameViewport);

        MH_Initialize();
        Hook::Function(InSDKUtils::GetImageBase() + 0x15FAF64, UWorldExecHook, &UWorldExecOriginal);

        /*
        while (!(GetAsyncKeyState(VK_PRIOR) & 1)) Sleep(100);

        UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), L"givemecheats", nullptr);

        while (!(GetAsyncKeyState(VK_PRIOR) & 1)) Sleep(100);

        auto PlayerController = UWorld::GetWorld()->OwningGameInstance->LocalPlayers[0]->PlayerController;
        UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), L"toggledebugcamera", PlayerController);
        */

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
