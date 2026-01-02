namespace Vehicles
{
    void ActivateSpawners()
    {
        // TODO When GameData is implemented calculate SpawnChance, Inoperable, etc.
        auto Spawners = Utils::GetAllActorsOfClass<AFortAthenaVehicleSpawner>();

        auto VMS = Utils::FindFirstNonDefaultObject<UFortVehicleModSubsystem>();

        for (auto Spawner : Spawners)
        {
            Spawner->SpawnVehicleWithConstruction(Spawner->GetVehicleClass(), Spawner->GetTransform());
            // TODO Mods
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
