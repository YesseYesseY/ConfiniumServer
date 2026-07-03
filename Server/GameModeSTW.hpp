namespace GameModeSTW
{
    bool ReadyToStartMatchHook(AFortGameModePvE* GameMode)
    {
        static bool Inited = false;
        if (!Inited)
        {
            Inited = true;

            // TODO STW Doesn't use repgraph
            GameMode->bEnableReplicationGraph = true;

            Net::Listen();

            GameMode->bWorldIsReady = true;
        }

        if (GameMode->NumPlayers > 0)
        {
            return true;
        }

        return false;
    }

    APawn* SpawnDefaultPawnForHook(AFortGameModePvE* GameMode, AFortPlayerController* PlayerController, AActor* StartSpot)
    {
        auto translivesmatter = StartSpot->GetTransform();
        if (translivesmatter.Translation.IsZero())
            translivesmatter.Translation.Z = 10000.0f;
        return GameMode->SpawnDefaultPawnAtTransform(PlayerController, translivesmatter);
    }

    void (*HandleStartingNewPlayerOriginal)(AFortGameModePvE* GameMode, AFortPlayerController* Controller);
    void HandleStartingNewPlayerHook(AFortGameModePvE* GameMode, AFortPlayerController* Controller)
    {
        auto PlayerState = (AFortPlayerState*)Controller->PlayerState;

        auto AssetManager = Utils::GetAssetManager();

        HandleStartingNewPlayerOriginal(GameMode, Controller);

        static auto AbilitySet = Utils::GetSoftPtr(AssetManager->GameDataSTW->GenericPlayerAbilitySet);
        Abilities::GiveAbilitySet(PlayerState->AbilitySystemComponent, AbilitySet);

        static std::vector<UFortItemDefinition*> StartingItems = {
            Utils::GetSoftPtr(AssetManager->GameDataCommon->EditToolItem),
            Utils::GetSoftPtr(AssetManager->GameDataSTW->DefaultInventoryList[0].ItemDefinition),
            Utils::GetSoftPtr(AssetManager->GameDataSTW->DefaultInventoryList[1].ItemDefinition),
            Utils::GetSoftPtr(AssetManager->GameDataSTW->DefaultInventoryList[2].ItemDefinition),
            Utils::GetSoftPtr(AssetManager->GameDataSTW->DefaultInventoryList[3].ItemDefinition),
            Utils::GetSoftPtr(AssetManager->GameDataSTW->HarvestingTools[0]),

            Utils::GetSoftPtr(AssetManager->GameDataCommon->WoodItemDefinition),
            Utils::GetSoftPtr(AssetManager->GameDataCommon->StoneItemDefinition),
            Utils::GetSoftPtr(AssetManager->GameDataCommon->MetalItemDefinition)
        };

        for (auto Item : StartingItems)
            Inventory::GiveItem(Controller, (UFortWorldItemDefinition*)Item);
    }

    void Init()
    {
        Hook::VTable<AFortGameModePvE>(2192 / 8, ReadyToStartMatchHook);
        Hook::VTable<AFortGameModePvE>(1720 / 8, SpawnDefaultPawnForHook);
        Hook::VTable<AFortGameModePvE>(1768 / 8, HandleStartingNewPlayerHook, &HandleStartingNewPlayerOriginal);
    }
}
