namespace Loot
{
    template <typename T>
    struct WeightedContainer
    {
        std::vector<T*> Items;
        float TotalWeight;

        bool IsValid()
        {
            return Items.size() > 0 && TotalWeight > 0.0f;
        }

        void Add(T* thing)
        {
            if (!thing || thing->Weight <= 0.0f)
                return;

            Items.push_back(thing);
            TotalWeight += thing->Weight;
        }

        T* GetRandomItem()
        {
            float Randy = UKismetMathLibrary::RandomFloatInRange(0, TotalWeight);
            float Total = 0.0f;

            for (auto Item : Items)
            {
                Total += Item->Weight;
                if (Total >= Randy)
                {
                    return Item;
                }
            }

            return nullptr;
        }
    };

    std::unordered_map<FName, WeightedContainer<FFortLootTierData>> LTDContainers;
    std::unordered_map<FName, WeightedContainer<FFortLootPackageData>> LPContainers;

    void PickLootDrops(UObject* Obj, FFrame* Stack, bool* Ret)
    {
        UObject* WorldContextObject;
        Stack->Step(&WorldContextObject);

        TArray<FFortItemEntry> OutLootToDropTemp;
        TArray<FFortItemEntry>& OutLootToDrop = Stack->StepRef<TArray<FFortItemEntry>>(&OutLootToDropTemp);

        FName TierGroupName;
        Stack->Step(&TierGroupName);

        int32 WorldLevel;
        Stack->Step(&WorldLevel);

        int32 ForcedLootTier;
        Stack->Step(&ForcedLootTier);

        Stack->End();

        

        static auto TempItemDef = Utils::FindObjectFast<UFortItemDefinition>("Athena_ShockGrenade");
        OutLootToDrop.Add(UFortKismetLibrary::CreateItemEntry(TempItemDef, 2, 0));
        *Ret = true;
    }

    void K2_SpawnPickupInWorld(UObject* Obj, FFrame* Stack, AFortPickup** Ret)
    {
        UObject* WorldContextObject;
        UFortWorldItemDefinition* ItemDefinition;
        int32 NumberToSpawn;
        FVector Position;
        FVector Direction;
        int32 OverrideMaxStackCount;
        bool bToss;
        bool bRandomRotation;
        bool bBlockedFromAutoPickup;
        int32 PickupInstigatorHandle;
        EFortPickupSourceTypeFlag SourceType;
        EFortPickupSpawnSource Source;
        AFortPlayerController* OptionalOwnerPC;
        bool bPickupOnlyRelevantToOwner;

        Stack->Step(&WorldContextObject);
        Stack->Step(&ItemDefinition);
        Stack->Step(&NumberToSpawn);
        Stack->Step(&Position);
        Stack->Step(&Direction);
        Stack->Step(&OverrideMaxStackCount);
        Stack->Step(&bToss);
        Stack->Step(&bRandomRotation);
        Stack->Step(&bBlockedFromAutoPickup);
        Stack->Step(&PickupInstigatorHandle);
        Stack->Step(&SourceType);
        Stack->Step(&Source);
        Stack->Step(&OptionalOwnerPC);
        Stack->Step(&bPickupOnlyRelevantToOwner);
        Stack->End();
        
        auto Pickup = Utils::SpawnActor<AFortPickupAthena>(Position);
        Pickup->PrimaryPickupItemEntry.ItemDefinition = ItemDefinition;
        Pickup->PrimaryPickupItemEntry.Count = NumberToSpawn;
        Pickup->OnRep_PrimaryPickupItemEntry();
        Pickup->bRandomRotation = bRandomRotation;
        auto Pawn = OptionalOwnerPC ? (AFortPlayerPawnAthena*)OptionalOwnerPC->Pawn : nullptr;
        Pickup->TossPickup(Position, Pawn, OverrideMaxStackCount, bToss, true, SourceType, Source);
        *Ret = Pickup;
    }

    void AddLTD(UDataTable* LTD)
    {
        if (!LTD)
            return;

        for (auto& thing : LTD->RowMap)
        {
            auto Data = (FFortLootTierData*)thing.Value();
            LTDContainers[Data->TierGroup].Add(Data);
        }
    }

    void AddLP(UDataTable* LP)
    {
        if (!LP)
            return;

        for (auto& thing : LP->RowMap)
        {
            auto Data = (FFortLootPackageData*)thing.Value();
            LPContainers[Data->LootPackageID].Add(Data);
        }
    }

    void Init()
    {
        auto GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
        auto Playlist = GameState->CurrentPlaylistInfo.BasePlaylist;

        AddLTD(Utils::GetSoftPtr(Playlist->LootTierData));
        AddLP(Utils::GetSoftPtr(Playlist->LootPackages));

        Hook::UFunc("Function FortniteGame.FortKismetLibrary.PickLootDrops", PickLootDrops);
        Hook::UFunc("Function FortniteGame.FortKismetLibrary.K2_SpawnPickupInWorld", K2_SpawnPickupInWorld);
    }
}
