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
    
        static bool PauseZoneThingy = true;
        if (!PauseZoneThingy)
        {
            PauseZoneThingy = true;
            UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), L"pausesafezone", nullptr);
        }

        Pawn->HealthSet->Health.CurrentValue = Pawn->HealthSet->Health.Maximum;
    }

    void ServerCheatHook(AFortPlayerController* PlayerController, const FString& Msg)
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
        else if (msg == L"test")
        {
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

    void SafezoneCheckThing(AFortPlayerPawn* Pawn, bool a2)
    {
        if (Pawn->bIsInsideSafeZone != a2)
        {
            Pawn->bIsInsideSafeZone = a2;

            auto ASC = Pawn->AbilitySystemComponent;
            if (ASC)
            {
                auto v5 = Pawn->bIsInsideSafeZone;
                static auto Tags = UBlueprintGameplayTagLibrary::MakeGameplayTagContainerFromTag({ UKismetStringLibrary::Conv_StringToName(L"Gameplay.InsideSafeZone") });
                if (a2)
                    UAbilitySystemBlueprintLibrary::AddLooseGameplayTags(Pawn, Tags);
                else
                    UAbilitySystemBlueprintLibrary::RemoveLooseGameplayTags(Pawn, Tags);
            }
        }

        static void (*ProcessMulticastDelegate)(void*, void*) = decltype(ProcessMulticastDelegate)(InSDKUtils::GetImageBase() + 0xD0F12C);
        bool arg = Pawn->bIsInsideSafeZone != false;
        ProcessMulticastDelegate(&Pawn->OnSafeZoneOccupancyChangedEvent, &arg);
    }

    void ServerTeleportToPlaygroundIslandDock(AFortPlayerControllerAthena* Controller)
    {
        static auto CreativeStarts = Utils::GetAllActorsOfClass<AFortPlayerStartCreative>();

        auto Start = CreativeStarts[UKismetMathLibrary::RandomInteger(CreativeStarts.Num())];
        auto Pawn = Controller->Pawn;
        if (!Pawn || !Start)
            return;

        Pawn->K2_TeleportTo(Start->K2_GetActorLocation(), Start->K2_GetActorRotation());
    }

    void PortalTeleportPlayer(AFortAthenaCreativePortal* Portal, FFrame* Stack)
    {
        AFortPlayerPawn* PlayerPawn;
        FRotator TeleportRotation;

        Stack->Step(&PlayerPawn);
        Stack->Step(&TeleportRotation);
        Stack->End();

        PlayerPawn->K2_TeleportTo(Portal->TeleportLocation, TeleportRotation);
    }

    void GetPlayerViewPoint(APlayerController* Controller, FVector& Location, FRotator& Rotation)
    {
        static FName NAME_Spectating = UKismetStringLibrary::Conv_StringToName(L"Spectating");
        if (Controller->StateName == NAME_Spectating)
        {
            Location = Controller->LastSpectatorSyncLocation;
            Rotation = Controller->LastSpectatorSyncRotation;
        }
        else if (Controller->Pawn)
        {
            Location = Controller->Pawn->K2_GetActorLocation();
            Rotation = Controller->GetControlRotation();
        }
        else
        {
            Location = Controller->K2_GetActorLocation();
            Rotation = Controller->K2_GetActorRotation();
        }
    }

    void ServerCraftSchematic(AFortPlayerController* Controller, FString& ItemId, int32 PostCraftSlot, int32 CraftAmount, EFortItemTier RequestedTier, bool bIsQuickCrafted)
    {
        auto Item = Controller->GetAccountItem(ItemId);
        auto SchematicDef = (UFortSchematicItemDefinition*)Item->ItemDefinition;
        auto ItemDef = SchematicDef->GetResultWorldItemDefinition();
        Inventory::GiveItem(Controller, ItemDef, SchematicDef->GetQuantityProduced(), Item->Level);
    }

    void Init()
    {
        Hook::AllVTables<AFortPlayerControllerZone>(2312 / 8, Utils::GetVTable<AFortPlayerController>()[2312 / 8]); // ServerAcknowledgePossession
        Hook::AllVTables<AFortPlayerController>(3880 / 8, ServerCheatHook);
        Hook::AllVTables<AFortPlayerController>(4504 / 8, ServerCraftSchematic);
        Hook::VTable<AFortPlayerControllerAthena>(10800 / 8, ServerTeleportToPlaygroundIslandDock); // This is not ServerTeleportToPlaygroundIslandDock i don't know why i named it that lol
        Hook::VTable<AFortPlayerControllerAthena>(1920 / 8, GetPlayerViewPoint);
        Hook::VTable<AFortPlayerPawnAthena>(4616 / 8, SafezoneCheckThing);
        Hook::VTable<UFortControllerComponent_Aircraft>(1256 / 8, ServerAttemptAircraftJumpHook);
        Hook::UFunc("Function FortniteGame.FortMissionLibrary.TeleportPlayerPawn", TeleportPlayerPawn);
        Hook::UFunc("Function FortniteGame.FortAthenaCreativePortal.TeleportPlayer", PortalTeleportPlayer);
    }
}
