namespace Gamemode
{
    bool ServerStarted = false;
    bool (*ReadyToStartMatchOriginal)(AFortGameModeAthena* GameMode);
    bool ReadyToStartMatchHook(AFortGameModeAthena* GameMode)
    {
        auto GameState = (AFortGameStateAthena*)GameMode->GameState;

        if (!ServerStarted)
        {
            ServerStarted = true;
    
            bool (*InitHost)(AOnlineBeaconHost*) = decltype(InitHost)(Utils::Offset(0x51E94E4));
            bool (*PauseBeaconRequests)(AOnlineBeaconHost*, bool) = decltype(PauseBeaconRequests)(Utils::Offset(0x679CA38));
            bool (*InitListen)(UNetDriver*, void*, FURL&, bool, FString&) = decltype(InitListen)(Utils::Offset(0x51E98A0));
            void (*SetWorld)(UNetDriver*, UWorld*) = decltype(SetWorld)(Utils::Offset(0xC2BB9C));
    
            auto Playlist = UObject::FindObject<UFortPlaylistAthena>("FortPlaylistAthena Playlist_DefaultSolo.Playlist_DefaultSolo");
            GameState->CurrentPlaylistInfo.BasePlaylist = Playlist;
            GameState->CurrentPlaylistInfo.OverridePlaylist = Playlist;
            GameState->CurrentPlaylistInfo.PlaylistReplicationKey++;
            GameState->OnRep_CurrentPlaylistInfo();
    
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
    
        if (GameMode->NumPlayers > 0)
        {
            auto Time = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());
            GameMode->WarmupCountdownDuration = 10;
            GameMode->WarmupEarlyCountdownDuration = 10;
            GameState->WarmupCountdownEndTime = Time + 10;
            GameState->WarmupCountdownStartTime = Time;
            GameFeatures::Init();
            Loot::Init();
            Vehicles::ActivateSpawners();
            return true;
        }

        return false;
    }

    APawn* SpawnDefaultPawnForHook(AFortGameModeAthena* GameMode, AFortPlayerControllerAthena* PlayerController, AActor* StartSpot)
    {
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
        Inventory::GiveItem(PlayerController, Utils::FindObjectFast<UFortTrapItemDefinition>("TID_Floor_Player_Launch_Pad_Athena"));
        // Inventory::GiveItem(PlayerController, Utils::FindObjectFast<UFortContextTrapItemDefinition>("TID_Context_Reinforced_Athena"));
        Inventory::Update(PlayerController);
    
        static void (*ApplyCharacterCustomization)(AFortPlayerStateAthena*, AFortPlayerPawnAthena*) = decltype(ApplyCharacterCustomization)(Utils::Offset(0x6979050));
    
        auto Pawn = (AFortPlayerPawnAthena*)GameMode->SpawnDefaultPawnAtTransform(PlayerController, StartSpot->GetTransform());
        ApplyCharacterCustomization(PlayerState, Pawn);
        return Pawn;
    }

    void (*StartNewSafeZonePhaseOriginal)(AFortGameModeAthena* GameMode, int a2);
    void StartNewSafeZonePhase(AFortGameModeAthena* GameMode, int a2)
    {
        StartNewSafeZonePhaseOriginal(GameMode, a2);

        auto GameState = (AFortGameStateAthena*)GameMode->GameState;

        auto& WaitTimes = *(TArray<float>*)(int64(GameState->MapInfo) + 0x828);
        auto& ShrinkTimes = *(TArray<float>*)(int64(GameState->MapInfo) + 0x838);
        GameMode->SafeZoneIndicator->SafeZoneStartShrinkTime = GameMode->SafeZoneIndicator->SafeZoneFinishShrinkTime + WaitTimes[GameMode->SafeZonePhase];
        GameMode->SafeZoneIndicator->SafeZoneFinishShrinkTime = GameMode->SafeZoneIndicator->SafeZoneStartShrinkTime + ShrinkTimes[GameMode->SafeZonePhase];
    }

    // Hooking this bypasses waiting for navmesh on BP_CalendarDynamicPOISelect_C
    void GetPlaylistEnableBotsHook(UObject* Obj, FFrame* Stack, bool* Ret)
    {
        Stack->End();
        *Ret = false;
    }
    
    void Init()
    {
        Hook::VTable<AFortGameModeAthena>(2192 / 8, ReadyToStartMatchHook, &ReadyToStartMatchOriginal);
        Hook::VTable<AFortGameModeAthena>(1720 / 8, SpawnDefaultPawnForHook);

        Hook::Function(InSDKUtils::GetImageBase() + 0x6109094, StartNewSafeZonePhase, &StartNewSafeZonePhaseOriginal);

        Hook::UFunc("Function FortniteGame.FortGameModeAthena.GetPlaylistEnableBots", GetPlaylistEnableBotsHook);
    }
}
