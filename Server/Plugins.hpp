namespace Plugins
{
    void ProcessGameData(UCurveTable* Table)
    {
        if (!Table || !Table->IsA(UCurveTable::StaticClass()))
            return;

        auto GameData = UObject::FindObject<UCurveTable>("CurveTable AthenaGameData.AthenaGameData");
        auto RowMap = *(TMap<FName, FSimpleCurve*>*)(int64(GameData) + 0x30);
        for (auto& thing : RowMap)
        {
            Globals::GameData[thing.Key().ToString()] = thing.Value()->Keys[0].Value;
        }
    }

    void Init()
    {
        auto Subsystem = Utils::FindFirstNonDefaultObject<UGameFeaturesSubsystem>();
        auto GameMode = (AFortGameModeBR*)UWorld::GetWorld()->AuthorityGameMode;
        auto GameState = (AFortGameStateBR*)GameMode->GameState;
        auto Playlist = GameState->CurrentPlaylistInfo.BasePlaylist;

        for (auto thing : Subsystem->GameFeaturePluginStateMachines)
        {
            auto GFD = (UFortGameFeatureData*)thing.Value()->StateProperties.GameFeatureData;
            if (!GFD->IsA(UFortGameFeatureData::StaticClass()))
                continue;

            bool foundit = false;
            for (auto thing : GFD->PlaylistOverrideGameData)
            {
                if (UBlueprintGameplayTagLibrary::HasTag(Playlist->GameplayTagContainer, thing.Key(), true))
                {
                    foundit = true;
                    // ProcessGameData(Utils::GetSoftPtr(thing.Value()));
                    ProcessGameData(thing.Value().Get());
                    break;
                }
            }

            if (foundit)
                continue;

            // ProcessGameData(Utils::GetSoftPtr(GFD->DefaultGameData));
            ProcessGameData(GFD->DefaultGameData.Get());
        }
    }
}
