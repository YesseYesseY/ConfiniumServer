#include <Windows.h>
#include <iostream>

#include <Utils.hpp>
#include <Hook.hpp>

#include <SDK/GE_OutsideSafeZoneDamage_classes.hpp>

void MarkArrayDirty(FFastArraySerializer* arr)
{
    static void (*InternalMarkArrayDirty)(FFastArraySerializer*) = decltype(InternalMarkArrayDirty)(InSDKUtils::GetImageBase() + 0x1496200);
    InternalMarkArrayDirty(arr);
}

namespace Abilities
{
    // I miss K2_GiveAbility from later ue5 :(
    void GiveAbility(UAbilitySystemComponent* Component, UClass* AbilityClass)
    {
        static FGameplayAbilitySpecHandle (*NativeFunc)(UAbilitySystemComponent* Component, FGameplayAbilitySpecHandle*, FGameplayAbilitySpec& AbilitySpec) = decltype(NativeFunc)(InSDKUtils::GetImageBase() + 0x12A5F48);

        FGameplayAbilitySpec spec = { -1, -1, -1, rand(), (UGameplayAbility*)AbilityClass->DefaultObject, 1, -1 };

        NativeFunc(Component, &spec.Handle, spec);
    }

    void InternalServerTryActivateAbility(UAbilitySystemComponent* Component, FGameplayAbilitySpecHandle Handle, bool InputPressed, FPredictionKey& PredictionKey, FGameplayEventData* TriggerEventData)
    {
        static FGameplayAbilitySpec* (*FindAbilitySpecFromHandle)(UAbilitySystemComponent* Component, FGameplayAbilitySpecHandle Handle) = decltype(FindAbilitySpecFromHandle)(InSDKUtils::GetImageBase() + 0x1494C78);
        FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(Component, Handle);
        if (!Spec)
        {
            Component->ClientActivateAbilityFailed(Handle, PredictionKey.Current);
            return;
        }

        const UGameplayAbility* AbilityToActivate = Spec->Ability;

        if (!AbilityToActivate)
        {
            Component->ClientActivateAbilityFailed(Handle, PredictionKey.Current);
            return;
        }

        
        if (AbilityToActivate->NetSecurityPolicy == EGameplayAbilityNetSecurityPolicy::ServerOnlyExecution ||
            AbilityToActivate->NetSecurityPolicy == EGameplayAbilityNetSecurityPolicy::ServerOnly)
        {
            Component->ClientActivateAbilityFailed(Handle, PredictionKey.Current);
            return;
        }

        // TODO? ConsumeAllReplicatedData
        
        UGameplayAbility* InstancedAbility = nullptr;
        Spec->InputPressed = true;

        static bool (*InternalTryActivateAbility)(UAbilitySystemComponent*, FGameplayAbilitySpecHandle, FPredictionKey, UGameplayAbility**, void*, FGameplayEventData*) = decltype(InternalTryActivateAbility)(InSDKUtils::GetImageBase() + 0x4EA33E4);
        if (InternalTryActivateAbility(Component, Handle, PredictionKey, &InstancedAbility, nullptr, TriggerEventData))
        {
        }
        else
        {
            Component->ClientActivateAbilityFailed(Handle, PredictionKey.Current);
            Spec->InputPressed = false;
            MarkArrayDirty(&Component->ActivatableAbilities);
        }
    }
}

int64 (*TickFlushOriginal)(UNetDriver* NetDriver);
int64 TickFlushHook(UNetDriver* NetDriver)
{
    if (NetDriver->ReplicationDriver)
    {
        static int64 (*ServerReplicateActors)(UReplicationDriver*) = decltype(ServerReplicateActors)(InSDKUtils::GetImageBase() + 0x55AA9E0);
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

        bool (*InitHost)(AOnlineBeaconHost*) = decltype(InitHost)(InSDKUtils::GetImageBase() + 0x51E94E4);
        bool (*PauseBeaconRequests)(AOnlineBeaconHost*, bool) = decltype(PauseBeaconRequests)(InSDKUtils::GetImageBase() + 0x679CA38);
        bool (*InitListen)(UNetDriver*, void*, FURL&, bool, FString&) = decltype(InitListen)(InSDKUtils::GetImageBase() + (0x51E98A0));
        void (*SetWorld)(UNetDriver*, UWorld*) = decltype(SetWorld)(InSDKUtils::GetImageBase() + (0xC2BB9C));

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

APawn* SpawnDefaultPawnForHook(AFortGameModeAthena* GameMode, AController* NewPlayer, AActor* StartSpot)
{
    auto PlayerState = (AFortPlayerStateAthena*)NewPlayer->PlayerState;
    PlayerState->AbilitySystemComponent->RemoveActiveGameplayEffectBySourceEffect(UGE_OutsideSafeZoneDamage_C::StaticClass(), nullptr, 1);

    auto GAS_AthenaPlayer = UObject::FindObject<UFortAbilitySet>("FortAbilitySet GAS_AthenaPlayer.GAS_AthenaPlayer");
    for (int i = 0; i < GAS_AthenaPlayer->GameplayAbilities.Num(); i++)
    {
        Abilities::GiveAbility(PlayerState->AbilitySystemComponent, GAS_AthenaPlayer->GameplayAbilities[i]);
    }

    return GameMode->SpawnDefaultPawnAtTransform(NewPlayer, StartSpot->GetTransform());
}

void ServerCheatHook(AFortPlayerControllerAthena* PlayerController, const FString& Msg)
{
    auto msg = Msg.ToWString();

    if (msg.starts_with(L"server "))
    {
        auto cmd = msg.substr(7);
        UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), cmd.c_str(), nullptr);
    }
    else if (msg == L"testthing")
    {
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

    Hook::Function(InSDKUtils::GetImageBase() + 0xC440A0, TickFlushHook, &TickFlushOriginal);
    Hook::Function(InSDKUtils::GetImageBase() + 0x7B69280, KickPlayerHook);
    Hook::Function(InSDKUtils::GetImageBase() + 0xD141FC, GetNetModeHook);
    Hook::Function(InSDKUtils::GetImageBase() + 0x15F7BDC, ReturnHook);

    Hook::VTable<AFortGameModeAthena>(2192 / 8, ReadyToStartMatchHook, &ReadyToStartMatchOriginal);
    Hook::VTable<AFortGameModeAthena>(1720 / 8, SpawnDefaultPawnForHook);
    Hook::VTable<AFortPlayerControllerAthena>(2312 / 8, Utils::GetVTable<AFortPlayerController>()[2312 / 8]); // ServerAcknowledgePossession
    Hook::VTable<AFortPlayerControllerAthena>(3880 / 8, ServerCheatHook);
    Hook::VTable<UFortAbilitySystemComponentAthena>(2120 / 8, Abilities::InternalServerTryActivateAbility);
    Hook::VTable<UFortControllerComponent_Aircraft>(1256 / 8, ServerAttemptAircraftJumpHook);

    *(bool*)(InSDKUtils::GetImageBase() + 0xB6E20FD) = false; // GIsClient
    *(bool*)(InSDKUtils::GetImageBase() + 0xB6E20FF) = true; // GIsServer
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
