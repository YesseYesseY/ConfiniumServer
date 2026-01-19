namespace Player
{
    void ServerAttemptAircraftJumpHook(UFortControllerComponent_Aircraft* Component, FRotator& ClientRotation)
    {
        auto GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
        auto PlayerController = (AFortPlayerControllerAthena*)Component->GetOwner();
        auto PlayerState = (AFortPlayerStateAthena*)PlayerController->PlayerState;
        auto Pawn = (AFortPlayerPawnAthena*)GameMode->SpawnDefaultPawnAtTransform(PlayerController, Component->CurrentAircraft->GetTransform());
        PlayerController->Possess(Pawn);
        PlayerController->ClientSetRotation(ClientRotation, false);
    
        PlayerState->AbilitySystemComponent->RemoveActiveGameplayEffectBySourceEffect(UGE_OutsideSafeZoneDamage_C::StaticClass(), nullptr, 1);
    
        static bool PauseZoneThingy = true;
        if (!PauseZoneThingy)
        {
            PauseZoneThingy = true;
            UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), L"pausesafezone", nullptr);
        }

        Pawn->HealthSet->Health.CurrentValue = Pawn->HealthSet->Health.Maximum;
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
        else if (msg == L"uwu")
        {
            auto Spade = UObject::FindObject<AActor>("Apollo_Farm_Shovel_Spade_01_C Artemis_POI_Cattus_b7cecd75.Artemis_POI_Cattus.PersistentLevel.Apollo_Farm_Shovel_Spade_01_C_7");
            PlayerController->Pawn->K2_TeleportTo(UKismetMathLibrary::Add_VectorVector(Spade->K2_GetActorLocation(), {0, 0, 10000 }), {});
        }
        else if (msg == L"testthing")
        {
            auto Subsystem = Utils::FindFirstNonDefaultObject<UGameFeaturesSubsystem>();
            for (auto thing : Subsystem->GameFeaturePluginStateMachines)
            {
                MessageBox("{}", thing.Value()->StateProperties.GameFeatureData->GetFullName());
            }

            // auto GameData = UObject::FindObject<UCurveTable>("CurveTable AthenaGameData.AthenaGameData");
            // auto RowMap = *(TMap<FName, FRealCurve*>*)(int64(GameData) + 0x30);
            // for (auto& thing : RowMap)
            // {
            //     MessageBox("{}", thing.Key().ToString());
            // }
        }
    }

    void TeleportPlayerPawn(UObject* obj, FFrame* Stack, bool* Ret)
    {
        UObject* WorldContextObject;
        AFortPlayerPawn* PlayerPawn;
        FVector DestLocation;
        FRotator DestRotation;
        bool bIgnoreCollision;
        bool bIgnoreSupplementalKillVolumeSweep;
        bool ReturnValue;

        Stack->Step(&WorldContextObject);
        Stack->Step(&PlayerPawn);
        Stack->Step(&DestLocation);
        Stack->Step(&DestRotation);
        Stack->Step(&bIgnoreCollision);
        Stack->Step(&bIgnoreSupplementalKillVolumeSweep);
        Stack->End();

        *Ret = PlayerPawn->K2_TeleportTo(DestLocation, DestRotation);
    }

    void Init()
    {
        Hook::VTable<AFortPlayerControllerAthena>(2312 / 8, Utils::GetVTable<AFortPlayerController>()[2312 / 8]); // ServerAcknowledgePossession
        Hook::VTable<AFortPlayerControllerAthena>(3880 / 8, ServerCheatHook);
        Hook::VTable<UFortControllerComponent_Aircraft>(1256 / 8, ServerAttemptAircraftJumpHook);
        Hook::UFunc("Function FortniteGame.FortMissionLibrary.TeleportPlayerPawn", TeleportPlayerPawn);
    }
}
