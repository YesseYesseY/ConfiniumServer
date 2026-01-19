namespace Vehicles
{
    void ActivateSpawners()
    {
        auto Spawners = Utils::GetAllActorsOfClass<AFortAthenaVehicleSpawner>();

        for (auto Spawner : Spawners)
        {
            auto VehicleClass = Spawner->GetVehicleClass(); // This loads the vehicle, GetSoftPtr doesn't for some reason
            auto VID = Spawner->bIsVehicleItemDefCached ? Spawner->CachedFortVehicleItemDef : Utils::GetSoftPtr(Spawner->FortVehicleItemDef);
            if (!VID)
                continue;

            bool SpawnIt = Spawner->bForceSpawnAlways;
            if (!SpawnIt)
            {
                auto Min = Globals::GetGameData(VID->VehicleMinSpawnPercent, 0.0f);
                auto Max = Globals::GetGameData(VID->VehicleMaxSpawnPercent, 0.0f);
                SpawnIt = Utils::RandomMinMax(Min, Max);
            }

            if (SpawnIt)
            {
                auto Vehicle = Spawner->SpawnVehicleWithConstruction(VehicleClass, Spawner->GetTransform());

                // Inoperable
                {
                    auto Min = Globals::GetGameData(VID->MinPercentInoperable, 0.0f);
                    auto Max = Globals::GetGameData(VID->MaxPercentInoperable, 0.0f);
                    if (Vehicle->IsA(AFortDagwoodVehicle::StaticClass()) && Utils::RandomMinMax(Min, Max))
                    {
                        auto Dagwood = (AFortDagwoodVehicle*)Vehicle;
                        Dagwood->bIsInoperable = true;
                        Dagwood->OnRep_IsInoperable();
                    }
                }

                // TODO Mods
                //      Vehicle Weapons
            }
        }

        Spawners.Free();
    }

    void ServerMove(AFortPhysicsPawn* Pawn, const FReplicatedPhysicsPawnState& InState)
    {
        auto Component = (UPrimitiveComponent*)Pawn->GetComponentByClass(UPrimitiveComponent::StaticClass());
        Component->K2_SetWorldLocationAndRotation((FVector)InState.Translation, UKismetMathLibrary::Quat_Rotator(InState.Rotation), false, nullptr, true);
        Component->SetAllPhysicsLinearVelocity(InState.LinearVelocity, false);
        Component->SetAllPhysicsAngularVelocityInDegrees(InState.AngularVelocity, false);
    }
}
