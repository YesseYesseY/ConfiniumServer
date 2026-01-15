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

    void Init()
    {
        Hook::Function(Utils::Offset(0xC440A0), TickFlushHook, &TickFlushOriginal);
        Hook::Function(Utils::Offset(0x7B69280), KickPlayerHook);
        Hook::Function(Utils::Offset(0xD141FC), GetNetModeHook);
    }
}
