#include <Windows.h>
#include <iostream>
#include <print>
#include <fstream>

#include <Utils.hpp>
#include <Hook.hpp>

#include <SDK/GE_OutsideSafeZoneDamage_classes.hpp>

#include "Inventory.hpp"
#include "Abilities.hpp"
#include "Vehicles.hpp"
#include "Building.hpp"

int64 (*TickFlushOriginal)(UNetDriver* NetDriver);
int64 TickFlushHook(UNetDriver* NetDriver)
{
    if (NetDriver->ReplicationDriver)
    {
        static int64 (*ServerReplicateActors)(UReplicationDriver*) = decltype(ServerReplicateActors)(Utils::Offset(0x55AA9E0));
        ServerReplicateActors(NetDriver->ReplicationDriver);
    }

    return TickFlushOriginal(NetDriver);
}

bool KickPlayerHook(int64 a1, int64 a2, int64 a3)
{
    return false;
}

int64 GetNetModeHook(int64 a1)
{
    return 1;
}

bool ServerStarted = false;
bool (*ReadyToStartMatchOriginal)(AFortGameModeAthena* GameMode);
bool ReadyToStartMatchHook(AFortGameModeAthena* GameMode)
{
    if (!ServerStarted)
    {
        ServerStarted = true;

        bool (*InitHost)(AOnlineBeaconHost*) = decltype(InitHost)(Utils::Offset(0x51E94E4));
        bool (*PauseBeaconRequests)(AOnlineBeaconHost*, bool) = decltype(PauseBeaconRequests)(Utils::Offset(0x679CA38));
        bool (*InitListen)(UNetDriver*, void*, FURL&, bool, FString&) = decltype(InitListen)(Utils::Offset(0x51E98A0));
        void (*SetWorld)(UNetDriver*, UWorld*) = decltype(SetWorld)(Utils::Offset(0xC2BB9C));

        auto GameState = (AFortGameStateAthena*)GameMode->GameState;
        auto Playlist = UObject::FindObject<UFortPlaylistAthena>("FortPlaylistAthena Playlist_DefaultSolo.Playlist_DefaultSolo");
        Playlist->LastSafeZoneIndex = 0;
        GameState->CurrentPlaylistInfo.BasePlaylist = Playlist;
        GameState->CurrentPlaylistInfo.OverridePlaylist = Playlist;
        GameState->CurrentPlaylistInfo.PlaylistReplicationKey++;
        GameState->OnRep_CurrentPlaylistInfo();

        GameState->GamePhase = EAthenaGamePhase::Warmup;
        GameState->OnRep_GamePhase(EAthenaGamePhase::Setup);

        GameMode->WarmupRequiredPlayerCount = 1;
        GameMode->WarmupCountdownDuration = INT32_MAX;
        GameMode->WarmupEarlyCountdownDuration = INT32_MAX;
        GameState->WarmupCountdownEndTime = INT32_MAX; 
        GameState->WarmupCountdownStartTime = 0;

        auto Beacon = Utils::SpawnActor<AFortOnlineBeaconHost>();
        Beacon->ListenPort = 7777;
        InitHost(Beacon);
        PauseBeaconRequests(Beacon, false);

        auto World = UWorld::GetWorld();
        auto NetDriver = Beacon->NetDriver;
        NetDriver->World = World;
        World->NetDriver = NetDriver;
        NetDriver->NetDriverName = UKismetStringLibrary::Conv_StringToName(L"GameNetDriver");

        FString error;
        FURL url = {};
        url.Port = 7776;
        InitListen(NetDriver, World, url, false, error);

        SetWorld(NetDriver, World);
        World->LevelCollections[0].NetDriver = NetDriver;
        World->LevelCollections[1].NetDriver = NetDriver;

        GameMode->bWorldIsReady = true;
    }

    return ReadyToStartMatchOriginal(GameMode);
}

APawn* SpawnDefaultPawnForHook(AFortGameModeAthena* GameMode, AFortPlayerControllerAthena* PlayerController, AActor* StartSpot)
{
    static bool InitedStuff = false;
    if (!InitedStuff)
    {
        InitedStuff = true;

        Vehicles::ActivateSpawners();
    }

    auto PlayerState = (AFortPlayerStateAthena*)PlayerController->PlayerState;
    PlayerState->AbilitySystemComponent->RemoveActiveGameplayEffectBySourceEffect(UGE_OutsideSafeZoneDamage_C::StaticClass(), nullptr, 1);

    auto AssetManager = Utils::GetAssetManager();

    auto GAS_AthenaPlayer = Utils::GetSoftPtr(AssetManager->GameDataBR->PlayerAbilitySetBR);
    for (int i = 0; i < GAS_AthenaPlayer->GameplayAbilities.Num(); i++)
        Abilities::GiveAbility(PlayerState->AbilitySystemComponent, GAS_AthenaPlayer->GameplayAbilities[i]);

    Inventory::GiveItem(PlayerController, Utils::GetSoftPtr(AssetManager->GameDataCommon->EditToolItem));

    for (int i = 0; i < 5; i++)
        Inventory::GiveItem(PlayerController, (UFortWorldItemDefinition*)GameMode->StartingItems[i].Item, GameMode->StartingItems[i].Count);

    Inventory::GiveItem(PlayerController, Utils::GetSoftPtr(AssetManager->GameDataCosmetics->FallbackPickaxe)->WeaponDefinition);
    Inventory::GiveItem(PlayerController, Utils::GetSoftPtr(AssetManager->GameDataCommon->WoodItemDefinition));
    Inventory::GiveItem(PlayerController, Utils::GetSoftPtr(AssetManager->GameDataCommon->StoneItemDefinition));
    Inventory::GiveItem(PlayerController, Utils::GetSoftPtr(AssetManager->GameDataCommon->MetalItemDefinition));
    Inventory::GiveItem(PlayerController, Utils::GetSoftPtr(AssetManager->GameDataBR->DefaultGlobalCurrencyItemDefinition));
    Inventory::GiveItem(PlayerController, Utils::FindObjectFast<UFortWeaponRangedItemDefinition>("WID_Shotgun_CoreBurst_Athena_SR"));
    Inventory::GiveItem(PlayerController, Utils::FindObjectFast<UFortWeaponRangedItemDefinition>("AthenaAmmoDataShells"));
    Inventory::GiveItem(PlayerController, Utils::FindObjectFast<UFortWeaponRangedItemDefinition>("AthenaAmmoDataBulletsLight"));
    Inventory::GiveItem(PlayerController, Utils::FindObjectFast<UFortWeaponRangedItemDefinition>("AthenaAmmoDataBulletsMedium"));
    Inventory::GiveItem(PlayerController, Utils::FindObjectFast<UFortWeaponRangedItemDefinition>("AthenaAmmoDataBulletsHeavy"));
    Inventory::GiveItem(PlayerController, Utils::FindObjectFast<UFortWeaponRangedItemDefinition>("AmmoDataRockets"));
    // Inventory::GiveItem(PlayerController, Utils::FindObjectFast<UFortContextTrapItemDefinition>("TID_Context_Reinforced_Athena"));
    Inventory::Update(PlayerController);

    static void (*ApplyCharacterCustomization)(AFortPlayerStateAthena*, AFortPlayerPawnAthena*) = decltype(ApplyCharacterCustomization)(Utils::Offset(0x6979050));

    auto Pawn = (AFortPlayerPawnAthena*)GameMode->SpawnDefaultPawnAtTransform(PlayerController, StartSpot->GetTransform());
    ApplyCharacterCustomization(PlayerState, Pawn);
    return Pawn;
}

void ServerCheatHook(AFortPlayerControllerAthena* PlayerController, const FString& Msg)
{
    auto msg = Msg.ToWString();

    if (msg.starts_with(L"server "))
    {
        auto cmd = msg.substr(7);
        UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), cmd.c_str(), nullptr);
    }
    else if (msg == L"dumpobjects")
    {
        std::ofstream outfile("objects.txt");
        for (int i = 0; i < UObject::GObjects->Num(); i++)
        {
            auto Object = UObject::GObjects->GetByIndex(i);
            if (!Object) continue;

            outfile << Object->GetFullName() << '\n';
        }
        outfile.close();
    }
    else if (msg == L"testthing")
    {
        auto GameData = UObject::FindObject<UCurveTable>("CurveTable AthenaGameData.AthenaGameData");
        auto RowMap = *(TMap<FName, FRealCurve*>*)(int64(GameData) + 0x30);
        for (auto& thing : RowMap)
        {
            MessageBox("{}", thing.Key().ToString());
        }
    }
}

void ServerAttemptAircraftJumpHook(UFortControllerComponent_Aircraft* Component, FRotator& ClientRotation)
{
    auto GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
    auto PlayerController = (AFortPlayerControllerAthena*)Component->GetOwner();
    auto PlayerState = (AFortPlayerStateAthena*)PlayerController->PlayerState;
    auto Pawn = GameMode->SpawnDefaultPawnAtTransform(PlayerController, Component->CurrentAircraft->GetTransform());
    PlayerController->Possess(Pawn);
    PlayerController->ClientSetRotation(ClientRotation, false);

    PlayerState->AbilitySystemComponent->RemoveActiveGameplayEffectBySourceEffect(UGE_OutsideSafeZoneDamage_C::StaticClass(), nullptr, 1);

    static bool PauseZoneThingy = true;
    if (!PauseZoneThingy)
    {
        PauseZoneThingy = true;
        UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), L"pausesafezone", nullptr);
    }
}

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

    Hook::Function(Utils::Offset(0xC440A0), TickFlushHook, &TickFlushOriginal);
    Hook::Function(Utils::Offset(0x7B69280), KickPlayerHook);
    Hook::Function(Utils::Offset(0xD141FC), GetNetModeHook);
    Hook::Function(Utils::Offset(0x15F7BDC), ReturnHook);


    Hook::VTable<AFortGameModeAthena>(2192 / 8, ReadyToStartMatchHook, &ReadyToStartMatchOriginal);
    Hook::VTable<AFortGameModeAthena>(1720 / 8, SpawnDefaultPawnForHook);
    Hook::VTable<AFortPlayerControllerAthena>(2312 / 8, Utils::GetVTable<AFortPlayerController>()[2312 / 8]); // ServerAcknowledgePossession
    Hook::VTable<AFortPlayerControllerAthena>(3880 / 8, ServerCheatHook);

    Building::Init();
    Inventory::Init();

    Hook::VTable<UFortAbilitySystemComponentAthena>(2120 / 8, Abilities::InternalServerTryActivateAbility);
    Hook::VTable<UFortControllerComponent_Aircraft>(1256 / 8, ServerAttemptAircraftJumpHook);

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
