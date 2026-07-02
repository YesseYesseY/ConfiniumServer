namespace Plugins
{
    std::vector<UGameFeatureData*> Active;

    void Init()
    {
        auto Subsystem = Utils::FindFirstNonDefaultObject<UGameFeaturesSubsystem>();
        auto GameMode = (AFortGameModeBR*)UWorld::GetWorld()->AuthorityGameMode;
        auto GameState = (AFortGameStateBR*)GameMode->GameState;
        auto Playlist = GameState->CurrentPlaylistInfo.BasePlaylist;

        for (auto& thing : Subsystem->GameFeaturePluginStateMachines)
        {
            if (*(uint8*)(int64(thing.Value()) + 0x40) == 25)
                Active.push_back(thing.Value()->StateProperties.GameFeatureData);
        }

        MsgBox("{}", Active.size());
    }
}
