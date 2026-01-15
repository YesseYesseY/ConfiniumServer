namespace Building
{
    bool CanPlaceBuild(UClass* BuildClass, const FCreateBuildingActorData& CBD, TArray<ABuildingActor*>& Existing)
    {
        static auto SupportSystem = Utils::FindFirstNonDefaultObject<UBuildingStructuralSupportSystem>();
        EFortBuildPreviewMarkerOptionalAdjustment thing;
        return SupportSystem->CanAddBuildingActorClassToGrid(UWorld::GetWorld(), BuildClass, CBD.BuildLoc, CBD.BuildRot, CBD.bMirrored, &Existing, &thing, false) == EFortStructuralGridQueryResults::CanAdd;
    }

    void ServerCreateBuildingActor(AFortPlayerControllerAthena* PlayerController, const FCreateBuildingActorData& CreateBuildingData)
    {
        static auto GameState = (AFortGameStateBR*)UWorld::GetWorld()->AuthorityGameMode->GameState;
        auto BuildClass = GameState->AllPlayerBuildableClasses[CreateBuildingData.BuildingClassHandle];
        if (BuildClass)
        {
            TArray<ABuildingActor*> Existing;
            if (CanPlaceBuild(BuildClass, CreateBuildingData, Existing))
            {
                auto Build = Utils::SpawnActorClass<ABuildingSMActor>(BuildClass, CreateBuildingData.BuildLoc, CreateBuildingData.BuildRot);
                for (auto Actor : Existing)
                    Actor->K2_DestroyActor();

                Build->InitializeKismetSpawnedBuildingActor(Build, PlayerController, true, nullptr);

                Inventory::RemoveItem(PlayerController, UFortKismetLibrary::K2_GetResourceItemDefinition(Build->ResourceType), 10);
            }

            Existing.Free();
        }
    }

    void ServerBeginEditingBuildingActor(AFortPlayerControllerAthena* PlayerController, ABuildingSMActor* BuildingActorToEdit)
    {
        BuildingActorToEdit->EditingPlayer = (AFortPlayerStateAthena*)PlayerController->PlayerState;
        BuildingActorToEdit->OnRep_EditingPlayer();

        static auto EditToolItem = Utils::GetSoftPtr(Utils::GetAssetManager()->GameDataCommon->EditToolItem);
        for (auto Entry : PlayerController->WorldInventory->Inventory.ReplicatedEntries)
        {
            if (Entry.ItemDefinition = EditToolItem)
            {
                auto Pawn = (AFortPlayerPawnAthena*)PlayerController->Pawn;
                Pawn->EquipWeaponDefinition(EditToolItem, Entry.ItemGuid, {}, false);
                break;
            }
        }
    }

    void ServerEndEditingBuildingActor(AFortPlayerControllerAthena* PlayerController, ABuildingSMActor* BuildingActorToStopEditing)
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

    void ServerRepairBuildingActor(AFortPlayerControllerAthena* PlayerController, ABuildingSMActor* BuildingActorToRepair)
    {
        auto Cost = UKismetMathLibrary::FFloor(UKismetMathLibrary::Lerp(7, 0, BuildingActorToRepair->GetHealthPercent()));
        BuildingActorToRepair->RepairBuilding(PlayerController, Cost);
        Inventory::RemoveItem(PlayerController, UFortKismetLibrary::K2_GetResourceItemDefinition(BuildingActorToRepair->ResourceType), Cost);
    }

    void Init()
    {
        Hook::VTable<AFortPlayerControllerAthena>(4704 / 8, ServerCreateBuildingActor);
        Hook::VTable<AFortPlayerControllerAthena>(4760 / 8, ServerBeginEditingBuildingActor);
        Hook::VTable<AFortPlayerControllerAthena>(4744 / 8, ServerEndEditingBuildingActor);
        Hook::VTable<AFortPlayerControllerAthena>(4720 / 8, ServerEditBuildingActor);
        Hook::VTable<AFortPlayerControllerAthena>(4672 / 8, ServerRepairBuildingActor);
    }
}
