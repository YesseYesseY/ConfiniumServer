namespace Inventory
{
    void Update(AFortPlayerControllerAthena* PlayerController, FFortItemEntry* ItemEntry = nullptr)
    {
        PlayerController->WorldInventory->HandleInventoryLocalUpdate();
        if (ItemEntry)
        {
            Utils::MarkItemDirty(&PlayerController->WorldInventory->Inventory, ItemEntry);
        }
        else
        {
            Utils::MarkArrayDirty(&PlayerController->WorldInventory->Inventory);
        }
    }

    FFortItemEntry* FindItemEntry(AFortPlayerControllerAthena* PlayerController, const FGuid& ItemGuid, int* Index = nullptr)
    {
        for (int i = 0; i < PlayerController->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
        {
            auto& ItemEntry = PlayerController->WorldInventory->Inventory.ReplicatedEntries[i];
            if (UKismetGuidLibrary::EqualEqual_GuidGuid(ItemEntry.ItemGuid, ItemGuid))
            {
                if (Index)
                    *Index = i;
                return &ItemEntry;
            }
        }

        return nullptr;
    }

    FFortItemEntry* FindItemEntry(AFortPlayerControllerAthena* PlayerController, UFortWorldItemDefinition* ItemDef, int* Index = nullptr)
    {
        for (int i = 0; i < PlayerController->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
        {
            auto& ItemEntry = PlayerController->WorldInventory->Inventory.ReplicatedEntries[i];
            if (ItemEntry.ItemDefinition == ItemDef)
            {
                if (Index)
                    *Index = i;

                return &ItemEntry;
            }
        }

        return nullptr;
    }

    void EquipItemEntry(AFortPlayerControllerAthena* PlayerController, FFortItemEntry* ItemEntry)
    {
        if (PlayerController->IsInAircraft())
            return;

        auto Pawn = (AFortPlayerPawnAthena*)PlayerController->Pawn;
        Pawn->EquipWeaponDefinition((UFortWeaponItemDefinition*)ItemEntry->ItemDefinition, ItemEntry->ItemGuid, {}, false);
    }

    void RemoveItem(AFortPlayerControllerAthena* PlayerController, const FGuid& ItemGuid, int32 Count)
    {
        int Index = -1;
        if (auto ItemEntry = FindItemEntry(PlayerController, ItemGuid, &Index))
        {
            if (Count >= ItemEntry->Count)
            {
                PlayerController->WorldInventory->Inventory.ReplicatedEntries.Remove(Index);
                PlayerController->WorldInventory->Inventory.ItemInstances.Remove(Index);
                Update(PlayerController);
            }
            else
            {
                ItemEntry->Count -= Count;
                Update(PlayerController, ItemEntry);
            }
        }
    }

    void RemoveItem(AFortPlayerControllerAthena* PlayerController, UFortWorldItemDefinition* ItemDef, int32 Count)
    {
        while (Count > 0)
        {
            for (int i = 0; i < PlayerController->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
            {
                auto& ItemEntry = PlayerController->WorldInventory->Inventory.ReplicatedEntries[i];

                if (ItemEntry.ItemDefinition == ItemDef)
                {
                    if (Count >= ItemEntry.Count)
                    {
                        PlayerController->WorldInventory->Inventory.ReplicatedEntries.Remove(i);
                        PlayerController->WorldInventory->Inventory.ItemInstances.Remove(i);
                        Update(PlayerController);
                        Count -= ItemEntry.Count;
                    }
                    else
                    {
                        ItemEntry.Count -= Count;
                        Update(PlayerController, &ItemEntry);
                        Count = 0;
                    }

                    break;
                }
            }
        }
    }

    void GiveItem(AFortPlayerControllerAthena* PlayerController, UFortWorldItemDefinition* ItemDef, int32 Count = -1)
    {
        if (Count == 0 || !ItemDef)
            return;

        if (Count == -1)
            Count = UFortScalableFloatUtils::GetValueAsInteger(ItemDef->MaxStackSize, 0);

        auto Item = (UFortWorldItem*)ItemDef->CreateTemporaryItemInstanceBP(Count, 1);

        if (ItemDef->IsA(UFortWeaponRangedItemDefinition::StaticClass()))
        {
            auto WeaponDef = (UFortWeaponRangedItemDefinition*)ItemDef;
            auto Stats = Utils::GetRangedStats(WeaponDef);
            Item->ItemEntry.LoadedAmmo = Stats->ClipSize;
        }

        PlayerController->WorldInventory->Inventory.ReplicatedEntries.Add(Item->ItemEntry);
        PlayerController->WorldInventory->Inventory.ItemInstances.Add(Item);
    }

    void ServerExecuteInventoryItem(AFortPlayerControllerAthena* PlayerController, const FGuid& ItemGuid)
    {
        if (auto ItemEntry = FindItemEntry(PlayerController, ItemGuid))
        {
            EquipItemEntry(PlayerController, ItemEntry);
        }
    }

    void RemoveInventoryItem(int64 a1, const FGuid& ItemGuid, int32 Count, bool bForceRemoveFromQuickBars, bool bForceRemove)
    {
        auto PlayerController = (AFortPlayerControllerAthena*)(a1 - 0x710);
        RemoveItem(PlayerController, ItemGuid, Count);
    }

    void Init()
    {
        Hook::Function(Utils::Offset(0x694108C), Inventory::RemoveInventoryItem);
        Hook::VTable<AFortPlayerControllerAthena>(4440 / 8, Inventory::ServerExecuteInventoryItem);
    }
}
