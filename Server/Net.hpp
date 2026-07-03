namespace Net
{
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

    void Listen()
    {
        bool (*InitHost)(AOnlineBeaconHost*) = decltype(InitHost)(Utils::Offset(0x51E94E4));
        bool (*PauseBeaconRequests)(AOnlineBeaconHost*, bool) = decltype(PauseBeaconRequests)(Utils::Offset(0x679CA38));
        bool (*InitListen)(UNetDriver*, void*, FURL&, bool, FString&) = decltype(InitListen)(Utils::Offset(0x51E98A0));
        void (*SetWorld)(UNetDriver*, UWorld*) = decltype(SetWorld)(Utils::Offset(0xC2BB9C));

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
    }

    void Init()
    {
        Hook::Function(Utils::Offset(0xC440A0), TickFlushHook, &TickFlushOriginal);
        Hook::Function(Utils::Offset(0x7B69280), KickPlayerHook);
        Hook::Function(Utils::Offset(0xD141FC), GetNetModeHook);
    }
}
