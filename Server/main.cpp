#include <Windows.h>
#include <iostream>
#include <print>
#include <fstream>

#include <Utils.hpp>
#include <Hook.hpp>

#include <SDK/GE_OutsideSafeZoneDamage_classes.hpp>
#include <SDK/GameplayTags_classes.hpp>
#include <SDK/ValetRuntime_classes.hpp>

#include "Globals.hpp"
#include "Plugins.hpp"
#include "Inventory.hpp"
#include "Abilities.hpp"
#include "Vehicles.hpp"
#include "Building.hpp"
#include "Net.hpp"
#include "Gamemode.hpp"
#include "Player.hpp"

void ReturnHook()
{
    return;
}

DWORD MainThread(HMODULE Module)
{
    AllocConsole();
    FILE* Dummy;
    freopen_s(&Dummy, "CONOUT$", "w", stdout);
    freopen_s(&Dummy, "CONIN$", "r", stdin);

    MH_Initialize();
    
    UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), L"log LogFortUIDirector None", nullptr);
    UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), L"log LogFort VeryVerbose", nullptr);

    FMemory::Init((void*)(Utils::Offset(0x1958FF4)));

    Hook::Function(Utils::Offset(0x15F7BDC), ReturnHook); // RequestExit

    Building::Init();
    Inventory::Init();
    Net::Init();
    Gamemode::Init();
    Player::Init();

    Hook::VTable<UFortAbilitySystemComponentAthena>(2120 / 8, Abilities::InternalServerTryActivateAbility);

    Hook::AllVTables<AFortPhysicsPawn>(2056 / 8, Vehicles::ServerMove);

    *(bool*)(Utils::Offset(0xB6E20FD)) = false; // GIsClient
    *(bool*)(Utils::Offset(0xB6E20FF)) = true; // GIsServer
    UWorld::GetWorld()->OwningGameInstance->LocalPlayers.Remove(0);
    UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), L"open Artemis_Terrain", nullptr);

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
