namespace Player
{
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

    void Init()
    {
        Hook::VTable<AFortPlayerControllerAthena>(2312 / 8, Utils::GetVTable<AFortPlayerController>()[2312 / 8]); // ServerAcknowledgePossession
        Hook::VTable<AFortPlayerControllerAthena>(3880 / 8, ServerCheatHook);
        Hook::VTable<UFortControllerComponent_Aircraft>(1256 / 8, ServerAttemptAircraftJumpHook);
    }
}
