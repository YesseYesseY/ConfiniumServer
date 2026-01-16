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
        if (ItemEntry->ItemDefinition->IsA(UFortDecoItemDefinition::StaticClass()))
        {
            Pawn->PickUpActor(nullptr, (UFortDecoItemDefinition*)ItemEntry->ItemDefinition);
        }
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

    int GiveItem(AFortPlayerControllerAthena* PlayerController, UFortWorldItemDefinition* ItemDef, int32 Count = -1)
    {
        if (Count == 0 || !ItemDef)
            return -1;

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
        return PlayerController->WorldInventory->Inventory.ItemInstances.Num() - 1;
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

    void GiveItemToInventoryOwner(UObject* a1, FFrame* Stack, UFortWorldItem** Ret)
    {
        TScriptInterface<class IFortInventoryOwnerInterface> InventoryOwner;
        UFortWorldItemDefinition* ItemDefinition;
        FGuid ItemVariantGuid;
        int32 NumberToGive;
        bool bNotifyPlayer;
        int32 ItemLevel;
        int32 PickupInstigatorHandle;
        bool bUseItemPickupAnalyticEvent;

        Stack->Step(&InventoryOwner);
        Stack->Step(&ItemDefinition);
        Stack->Step(&ItemVariantGuid);
        Stack->Step(&NumberToGive);
        Stack->Step(&bNotifyPlayer);
        Stack->Step(&ItemLevel);
        Stack->Step(&PickupInstigatorHandle);
        Stack->Step(&bUseItemPickupAnalyticEvent);
        Stack->End();
        
        auto PlayerController = (AFortPlayerControllerAthena*)InventoryOwner.GetObjectRef();

        auto Index = GiveItem(PlayerController, ItemDefinition, NumberToGive);
        *Ret = nullptr;

        if (Index != -1)
        {
            *Ret = PlayerController->WorldInventory->Inventory.ItemInstances[Index];
            Update(PlayerController);
        }
    }

    void Init()
    {
        Hook::Function(Utils::Offset(0x694108C), Inventory::RemoveInventoryItem);
        Hook::VTable<AFortPlayerControllerAthena>(4440 / 8, Inventory::ServerExecuteInventoryItem);
        Hook::UFunc("Function FortniteGame.FortKismetLibrary.GiveItemToInventoryOwner", GiveItemToInventoryOwner);
    }
}
