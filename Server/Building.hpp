namespace Building
{
    bool CanPlaceBuild(ABuildingSMActor* Build, const FCreateBuildingActorData& CBD, TArray<ABuildingActor*>& Existing)
    {
        // TODO Existing
        return true;

        // This works fine but doesn't include every highlighted prop in Existing
        // static auto SupportSystem = Utils::FindFirstNonDefaultObject<UBuildingStructuralSupportSystem>();
        // EFortBuildPreviewMarkerOptionalAdjustment thing;
        // return SupportSystem->K2_CanAddBuildingActorToGrid(UWorld::GetWorld(), Build, CBD.BuildLoc, CBD.BuildRot, CBD.bMirrored, &Existing, &thing, false) == EFortStructuralGridQueryResults::CanAdd;
    }

    void ServerCreateBuildingActor(AFortPlayerControllerAthena* PlayerController, const FCreateBuildingActorData& CreateBuildingData)
    {
        static auto GameState = (AFortGameStateBR*)UWorld::GetWorld()->AuthorityGameMode->GameState;
        auto BuildClass = GameState->AllPlayerBuildableClasses[CreateBuildingData.BuildingClassHandle];
        if (BuildClass)
        {
            auto Build = Utils::SpawnActorClass<ABuildingSMActor>(BuildClass, CreateBuildingData.BuildLoc, CreateBuildingData.BuildRot);

            static TArray<ABuildingActor*> Existing;
            if (CanPlaceBuild(Build, CreateBuildingData, Existing))
            {
                for (auto Actor : Existing)
                    Actor->K2_DestroyActor();

                Build->InitializeKismetSpawnedBuildingActor(Build, PlayerController, true, nullptr);

                Inventory::RemoveItem(PlayerController, UFortKismetLibrary::K2_GetResourceItemDefinition(Build->ResourceType), 10);
            }
            else
            {
                Build->SilentDie(false);
            }

            Existing.Clear();
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
        BuildingActorToRepair->RepairBuilding(PlayerController, Cost); // TODO Remove cost from inventory
    }
}
