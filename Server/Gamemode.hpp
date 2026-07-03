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
    
            auto Playlist = UObject::FindObject<UFortPlaylistAthena>(
                    "FortPlaylistAthena Playlist_DefaultSolo.Playlist_DefaultSolo"
                    // "FortPlaylistAthena Playlist_BattleLab.Playlist_BattleLab"
                    );
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
            auto Playlist = GameState->CurrentPlaylistInfo.BasePlaylist;
            for (auto& Level : Playlist->AdditionalLevels)
                GameState->AdditionalPlaylistLevelsStreamed.Add({ Level.ObjectID.AssetPathName, false });

            for (auto& Level : Playlist->AdditionalLevelsServerOnly)
                GameState->AdditionalPlaylistLevelsStreamed.Add({ Level.ObjectID.AssetPathName, true });

            GameState->OnRep_AdditionalPlaylistLevelsStreamed();

            auto Time = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());
            GameMode->WarmupCountdownDuration = 10;
            GameMode->WarmupEarlyCountdownDuration = 10;
            GameState->WarmupCountdownEndTime = Time + GameMode->WarmupCountdownDuration;
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
        static void (*ApplyCharacterCustomization)(AFortPlayerStateAthena*, AFortPlayerPawnAthena*) = decltype(ApplyCharacterCustomization)(Utils::Offset(0x6979050));

        auto PlayerState = (AFortPlayerStateAthena*)PlayerController->PlayerState;
        auto Pawn = (AFortPlayerPawnAthena*)GameMode->SpawnDefaultPawnAtTransform(PlayerController, StartSpot->GetTransform());

        ApplyCharacterCustomization(PlayerState, Pawn);

        return Pawn;
    }

    void (*HandleStartingNewPlayerOriginal)(AFortGameModeAthena* GameMode, AFortPlayerController* Controller);
    void HandleStartingNewPlayerHook(AFortGameModeAthena* GameMode, AFortPlayerController* Controller)
    {
        HandleStartingNewPlayerOriginal(GameMode, Controller);

        auto PlayerState = (AFortPlayerStateAthena*)Controller->PlayerState;
        auto AssetManager = Utils::GetAssetManager();

        static std::vector<UFortWorldItemDefinition*> StartingItems = {
            Utils::GetSoftPtr(AssetManager->GameDataCommon->EditToolItem),
            (UFortWorldItemDefinition*)GameMode->StartingItems[0].Item,
            (UFortWorldItemDefinition*)GameMode->StartingItems[1].Item,
            (UFortWorldItemDefinition*)GameMode->StartingItems[2].Item,
            (UFortWorldItemDefinition*)GameMode->StartingItems[3].Item,
            (UFortWorldItemDefinition*)GameMode->StartingItems[4].Item,
            Utils::GetSoftPtr(AssetManager->GameDataCosmetics->FallbackPickaxe)->WeaponDefinition,

            // Above = Real Starting Items, Below = Extra Stuff

            Utils::GetSoftPtr(AssetManager->GameDataCommon->WoodItemDefinition),
            Utils::GetSoftPtr(AssetManager->GameDataCommon->StoneItemDefinition),
            Utils::GetSoftPtr(AssetManager->GameDataCommon->MetalItemDefinition),
            Utils::GetSoftPtr(AssetManager->GameDataBR->DefaultGlobalCurrencyItemDefinition),
            Utils::FindObjectFast<UFortWeaponRangedItemDefinition>("WID_Shotgun_CoreBurst_Athena_SR"),
            Utils::FindObjectFast<UFortWeaponRangedItemDefinition>("AthenaAmmoDataShells"),
            Utils::FindObjectFast<UFortWeaponRangedItemDefinition>("AthenaAmmoDataBulletsLight"),
            Utils::FindObjectFast<UFortWeaponRangedItemDefinition>("AthenaAmmoDataBulletsMedium"),
            Utils::FindObjectFast<UFortWeaponRangedItemDefinition>("AthenaAmmoDataBulletsHeavy"),
            Utils::FindObjectFast<UFortWeaponRangedItemDefinition>("AmmoDataRockets"),
            Utils::FindObjectFast<UFortTrapItemDefinition>("TID_Floor_Player_Launch_Pad_Athena")
        };

        static auto GAS_AthenaPlayer = Utils::GetSoftPtr(AssetManager->GameDataBR->PlayerAbilitySetBR);

        for (int i = 0; i < GAS_AthenaPlayer->GameplayAbilities.Num(); i++)
            Abilities::GiveAbility(PlayerState->AbilitySystemComponent, GAS_AthenaPlayer->GameplayAbilities[i]);

        for (auto ItemDef : StartingItems)
            Inventory::GiveItem(Controller, ItemDef);
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
        Hook::VTable<AFortGameModeAthena>(1768 / 8, HandleStartingNewPlayerHook, &HandleStartingNewPlayerOriginal);

        Hook::Function(InSDKUtils::GetImageBase() + 0x6109094, StartNewSafeZonePhase, &StartNewSafeZonePhaseOriginal);

        Hook::UFunc("Function FortniteGame.FortGameModeAthena.GetPlaylistEnableBots", GetPlaylistEnableBotsHook);
    }
}
