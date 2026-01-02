namespace Inventory
{
    void GiveItem(AFortPlayerControllerAthena* PlayerController, UFortWorldItemDefinition* ItemDef, int32 Count = -1)
    {
        if (Count == 0 || !ItemDef)
            return;

        if (Count == -1)
            Count = UFortScalableFloatUtils::GetValueAsInteger(ItemDef->MaxStackSize, 0);

        auto Item = (UFortWorldItem*)ItemDef->CreateTemporaryItemInstanceBP(Count, 1);

        PlayerController->WorldInventory->Inventory.ReplicatedEntries.Add(Item->ItemEntry);
        PlayerController->WorldInventory->Inventory.ItemInstances.Add(Item);
    }

    void Update(AFortPlayerControllerAthena* PlayerController)
    {
        PlayerController->WorldInventory->HandleInventoryLocalUpdate();
        Utils::MarkArrayDirty(&PlayerController->WorldInventory->Inventory);
    }

    void ServerExecuteInventoryItem(AFortPlayerControllerAthena* PlayerController, const FGuid& ItemGuid)
    {
        if (PlayerController->IsInAircraft())
            return;

        for (auto Entry : PlayerController->WorldInventory->Inventory.ReplicatedEntries)
        {
            if (UKismetGuidLibrary::EqualEqual_GuidGuid(Entry.ItemGuid, ItemGuid))
            {
                auto Pawn = (AFortPlayerPawnAthena*)PlayerController->Pawn;
                Pawn->EquipWeaponDefinition((UFortWeaponItemDefinition*)Entry.ItemDefinition, ItemGuid, {}, false);
                break;
            }
        }
    }
}
