namespace Building
{
    bool CanPlaceBuild(UClass* BuildClass, FVector Location, FRotator Rotation, bool bMirrored, TArray<ABuildingActor*>& Existing)
    {
        static auto SupportSystem = Utils::FindFirstNonDefaultObject<UBuildingStructuralSupportSystem>();
        EFortBuildPreviewMarkerOptionalAdjustment thing;
        return SupportSystem->CanAddBuildingActorClassToGrid(UWorld::GetWorld(), BuildClass, Location, Rotation, bMirrored, &Existing, &thing, false) == EFortStructuralGridQueryResults::CanAdd;
    }

    ABuildingSMActor* CreateBuildingActor(UClass* BuildClass, FVector Location, FRotator Rotation, bool bMirrored, AFortPlayerController* PlayerController)
    {
        ABuildingSMActor* Build = nullptr;
        TArray<ABuildingActor*> Existing;
        if (CanPlaceBuild(BuildClass, Location, Rotation, bMirrored, Existing))
        {
            Build = Utils::SpawnActorClass<ABuildingSMActor>(BuildClass, Location, Rotation);
            for (auto Actor : Existing)
                Actor->K2_DestroyActor();

            Build->InitializeKismetSpawnedBuildingActor(Build, PlayerController, true, nullptr);

            Inventory::RemoveItem(PlayerController, UFortKismetLibrary::K2_GetResourceItemDefinition(Build->ResourceType), 10);
        }
        Existing.Free();
        return Build;
    }

    void ServerCreateBuildingActor(AFortPlayerController* PlayerController, FCreateBuildingActorData& CreateBuildingData)
    {
        static auto GameState = (AFortGameStateZone*)UWorld::GetWorld()->AuthorityGameMode->GameState;
        auto BuildClass = GameState->AllPlayerBuildableClasses[CreateBuildingData.BuildingClassHandle];
        if (BuildClass)
        {
            CreateBuildingActor(BuildClass, CreateBuildingData.BuildLoc, CreateBuildingData.BuildRot, CreateBuildingData.bMirrored, PlayerController);
        }
    }

    void ServerBeginEditingBuildingActor(AFortPlayerController* PlayerController, ABuildingSMActor* BuildingActorToEdit)
    {
        BuildingActorToEdit->EditingPlayer = (AFortPlayerStateZone*)PlayerController->PlayerState;
        BuildingActorToEdit->OnRep_EditingPlayer();

        static auto EditToolItem = Utils::GetSoftPtr(Utils::GetAssetManager()->GameDataCommon->EditToolItem);
        for (auto Entry : PlayerController->WorldInventory->Inventory.ReplicatedEntries)
        {
            if (Entry.ItemDefinition = EditToolItem)
            {
                auto Pawn = (AFortPlayerPawn*)PlayerController->Pawn;
                Pawn->EquipWeaponDefinition(EditToolItem, Entry.ItemGuid, {}, false);
                break;
            }
        }
    }

    void ServerEndEditingBuildingActor(AFortPlayerController* PlayerController, ABuildingSMActor* BuildingActorToStopEditing)
    {
        BuildingActorToStopEditing->EditingPlayer = nullptr;
        BuildingActorToStopEditing->OnRep_EditingPlayer();
    }

    void ServerEditBuildingActor(AFortPlayerControllerAthena* PlayerController, ABuildingSMActor* BuildingActorToEdit, TSubclassOf<ABuildingSMActor> NewBuildingClass, uint8 RotationIterations, bool bMirrored)
    {
        ABuildingSMActor* (*ReplaceBuildingActor)(ABuildingSMActor*, EBuildingReplacementType, TSubclassOf<ABuildingSMActor>, int, int, bool, AFortPlayerControllerAthena*)
            = decltype(ReplaceBuildingActor)(Utils::Offset(0x632DAD0));
        
        ReplaceBuildingActor(BuildingActorToEdit, EBuildingReplacementType::BRT_Edited, NewBuildingClass, 0, RotationIterations, bMirrored, PlayerController);
    }

    void ServerRepairBuildingActor(AFortPlayerController* PlayerController, ABuildingSMActor* BuildingActorToRepair)
    {
        auto Cost = UKismetMathLibrary::FFloor(UKismetMathLibrary::Lerp(7, 0, BuildingActorToRepair->GetHealthPercent()));
        BuildingActorToRepair->RepairBuilding(PlayerController, Cost);
        Inventory::RemoveItem(PlayerController, UFortKismetLibrary::K2_GetResourceItemDefinition(BuildingActorToRepair->ResourceType), Cost);
    }

    void ServerSpawnDeco(AFortDecoTool* Tool, const FVector& Location, const FRotator& Rotation, ABuildingSMActor* AttachedActor, EBuildingAttachmentType InBuildingAttachmentType)
    {
        auto Pawn = (AFortPlayerPawnAthena*)Tool->GetOwner();
        auto PlayerController = (AFortPlayerControllerAthena*)Pawn->Controller;
        auto TrapClass = Utils::GetSoftPtr(((UFortDecoItemDefinition*)Tool->ItemDefinition)->BlueprintClass);
        auto Trap = Utils::SpawnActorClass<ABuildingSMActor>(TrapClass, Location, Rotation, AttachedActor);
        Trap->SetParentActorToAttachTo(AttachedActor);
        Trap->BuildingAttachmentType = InBuildingAttachmentType;
        Trap->InitializeKismetSpawnedBuildingActor(AttachedActor, PlayerController, true, nullptr);
        Inventory::RemoveItem(PlayerController, (UFortDecoItemDefinition*)Tool->ItemDefinition, 1);
    }

    void ServerCreateBuildingAndSpawnDeco(AFortDecoTool* Tool, const FVector& BuildingLocation, const FRotator& BuildingRotation, const FVector& Location, 
            const FRotator& Rotation, EBuildingAttachmentType InBuildingAttachmentType, bool bSpawnDecoOnExtraPiece, const FVector& BuildingExtraPieceLocation)
    {
        static UClass* Builds[9] = { 
            UObject::FindClassFast("PBWA_W1_Floor_C"), UObject::FindClassFast("PBWA_W1_Solid_C"), UObject::FindClassFast("PBWA_W1_StairW_C"),
            UObject::FindClassFast("PBWA_S1_Floor_C"), UObject::FindClassFast("PBWA_S1_Solid_C"), UObject::FindClassFast("PBWA_S1_StairW_C"),
            UObject::FindClassFast("PBWA_M1_Floor_C"), UObject::FindClassFast("PBWA_M1_Solid_C"), UObject::FindClassFast("PBWA_M1_StairW_C")
        };

        auto Pawn = (AFortPlayerPawnAthena*)Tool->GetOwner();
        auto PlayerController = (AFortPlayerControllerAthena*)Pawn->Controller;
        auto BuildClass = Builds[((uint8)PlayerController->CurrentResourceType * 3) + ((uint8)InBuildingAttachmentType <= 1 ? (uint8)InBuildingAttachmentType : 2)];
        ABuildingSMActor* Build;
        if (BuildClass)
        {
            Build = CreateBuildingActor(BuildClass, BuildingLocation, BuildingRotation, false, PlayerController);
        }

        ServerSpawnDeco(Tool, Location, Rotation, Build, InBuildingAttachmentType);
    }

    void SetDynamicFoundationEnabled(ABuildingFoundation* Foundation, FFrame* Stack)
    {
        bool Enabled;
        Stack->Step(&Enabled);
        Stack->End();

        auto State = Enabled ? EDynamicFoundationEnabledState::Enabled : EDynamicFoundationEnabledState::Disabled;

        Foundation->FoundationEnabledState = State;
        Foundation->DynamicFoundationRepData.EnabledState = State;
        Foundation->OnRep_DynamicFoundationRepData();
    }

    void SetDynamicFoundationTransform(ABuildingFoundation* Foundation, FFrame* Stack)
    {
        FTransform NewTransform;
        Stack->Step(&NewTransform);
        Stack->End();

        Foundation->DynamicFoundationTransform = NewTransform;
        Foundation->DynamicFoundationRepData.Translation = NewTransform.Translation;
        Foundation->DynamicFoundationRepData.Rotation = UKismetMathLibrary::Quat_Rotator(NewTransform.Rotation);
        Foundation->OnRep_DynamicFoundationRepData();
    }

    void Init()
    {
        Hook::AllVTables<AFortPlayerController>(4704 / 8, ServerCreateBuildingActor);
        Hook::AllVTables<AFortPlayerController>(4760 / 8, ServerBeginEditingBuildingActor);
        Hook::AllVTables<AFortPlayerController>(4744 / 8, ServerEndEditingBuildingActor);
        Hook::AllVTables<AFortPlayerController>(4720 / 8, ServerEditBuildingActor);
        Hook::AllVTables<AFortPlayerController>(4672 / 8, ServerRepairBuildingActor);

        // 2952 is not null it calls 2984 and that one is null
        Hook::AllVTables<AFortDecoTool>(2952 / 8, ServerSpawnDeco);
        Hook::AllVTables<AFortDecoTool>(2936 / 8, ServerCreateBuildingAndSpawnDeco);

        Hook::UFunc("Function FortniteGame.BuildingFoundation.SetDynamicFoundationEnabled", SetDynamicFoundationEnabled);
        Hook::UFunc("Function FortniteGame.BuildingFoundation.SetDynamicFoundationTransform", SetDynamicFoundationTransform);
    }
}
