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

        auto BuildClass = PlayerController->BroadcastRemoteClientInfo->RemoteBuildableClass;
        if (BuildClass)
        {
            auto Build = Utils::SpawnActorClass<ABuildingSMActor>(BuildClass, CreateBuildingData.BuildLoc, CreateBuildingData.BuildRot);

            static TArray<ABuildingActor*> Existing;
            if (CanPlaceBuild(Build, CreateBuildingData, Existing))
            {
                for (auto Actor : Existing)
                    Actor->K2_DestroyActor();

                Build->InitializeKismetSpawnedBuildingActor(Build, PlayerController, true, nullptr);
            }
            else
            {
                Build->SilentDie(false);
            }

            Existing.Clear();
        }
    }
}
