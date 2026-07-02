namespace Loot
{
    struct LTDData
    {
        float Weight;
        FName LootPackage;
        float NumLootPackageDrops;
        std::vector<int32> LootPackageCategoryMinArray;

        LTDData()
        {
        }

        LTDData(FFortLootTierData* Data)
        {
            Weight = Data->Weight;
            LootPackage = Data->LootPackage;
            NumLootPackageDrops = Data->NumLootPackageDrops;
            for (auto thing : Data->LootPackageCategoryMinArray)
                LootPackageCategoryMinArray.push_back(thing);
        }
    };

    struct LPData
    {
        float Weight;
        int32 Count;
        FName LootPackageCall;
        UFortItemDefinition* ItemDefinition;

        LPData()
        {
        }

        LPData(FFortLootPackageData* Data)
        {
            Weight = Data->Weight;
            Count = Data->Count;
            LootPackageCall = UKismetStringLibrary::Conv_StringToName(Data->LootPackageCall);
            ItemDefinition = Utils::GetSoftPtr(Data->ItemDefinition);
        }
    };

    template <typename T>
    struct WeightedContainer
    {
        std::vector<T> Items;
        float TotalWeight;

        bool IsValid()
        {
            return Items.size() > 0 && TotalWeight > 0.0f;
        }

        int32 Num()
        {
            return Items.size();
        }

        void Add(T thing)
        {
            if (thing.Weight <= 0.0f)
                return;

            Items.push_back(thing);
            TotalWeight += thing.Weight;
        }

        T GetRandomItem()
        {
            float Randy = UKismetMathLibrary::RandomFloatInRange(0, TotalWeight);
            float Total = 0.0f;

            for (auto Item : Items)
            {
                Total += Item.Weight;
                if (Total >= Randy)
                {
                    return Item;
                }
            }

            return Items[0];
        }
    };

    std::unordered_map<FName, WeightedContainer<LTDData>> LTDContainers;
    std::unordered_map<FName, WeightedContainer<LPData>> LPContainers;

    void AddLTD(UDataTable* LTD)
    {
        if (!LTD)
            return;

        for (auto& thing : LTD->RowMap)
        {
            auto Data = (FFortLootTierData*)thing.Value();
            LTDContainers[Data->TierGroup].Add(LTDData(Data));
        }
    }

    void AddLP(UDataTable* LP)
    {
        if (!LP)
            return;

        for (auto& thing : LP->RowMap)
        {
            auto Data = (FFortLootPackageData*)thing.Value();
            LPContainers[Data->LootPackageID].Add(LPData(Data));
        }
    }

    void PickLootDrops(TArray<FFortItemEntry>& OutLootToDrop, FName TierGroupName, int32 ForcedLootTier)
    {
        auto GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;

        for (auto& thing : GameMode->RedirectAthenaLootTierGroups)
        {
            if (thing.Key() == TierGroupName)
                TierGroupName = thing.Value();
        }

        if (!LTDContainers.contains(TierGroupName))
            return;

        auto LootTier = LTDContainers[TierGroupName];
        if (!LootTier.IsValid())
            return;

        auto LTI = LootTier.GetRandomItem();

        if (!LPContainers.contains(LTI.LootPackage))
            return;

        auto BaseLootPackage = LPContainers[LTI.LootPackage];
        auto IsWorldList = LTI.LootPackage.ToString().starts_with("WorldList");
        if (IsWorldList)
        {
            for (int i = 0; i < LTI.NumLootPackageDrops; i++)
            {
                auto toadd = BaseLootPackage.GetRandomItem();
                if (toadd.Count <= 0)
                    continue;

                OutLootToDrop.Add(UFortKismetLibrary::CreateItemEntry(toadd.ItemDefinition, toadd.Count, 0));
            }
        }
        else
        {
            int Added = 0;
            for (int i = 0; i < BaseLootPackage.Num(); i++)
            {
                if (Added >= LTI.NumLootPackageDrops)
                    break;

                for (int j = 0; j < LTI.LootPackageCategoryMinArray[i]; j++)
                {
                    auto LPC = BaseLootPackage.Items[i].LootPackageCall;
                    if (!LPContainers.contains(LPC))
                        return;

                    auto RealLootPackage = LPContainers[LPC];
                    if (RealLootPackage.Num() <= 0)
                        continue;

                    Added++;
                    auto toadd = RealLootPackage.GetRandomItem();
                    if (toadd.Count <= 0)
                        continue;
                    auto ItemDef = toadd.ItemDefinition;
                    OutLootToDrop.Add(UFortKismetLibrary::CreateItemEntry(ItemDef, toadd.Count, 0));

                    static auto FWPSAD = UObject::FindObject<UFortWeaponPickupSpawnAmmoData>("FortWeaponPickupSpawnAmmoData FortWeaponPickupSpawnAmmoData.FortWeaponPickupSpawnAmmoData");

                    if (!ItemDef->IsA(UFortWeaponItemDefinition::StaticClass()))
                        continue;

                    if (!FWPSAD)
                        continue;

                    auto WeaponDef = (UFortWeaponItemDefinition*)ItemDef;
                    auto AmmoData = Utils::GetSoftPtr(WeaponDef->AmmoData);
                    if (!AmmoData)
                        continue;

                    auto& AmmoTags = AmmoData->GameplayTags;
                    auto& WPACA = FWPSAD->WeaponPickupAmmoCountArray;
                    for (auto& WPACD : WPACA)
                    {
                        if (UBlueprintGameplayTagLibrary::HasTag(AmmoTags, WPACD.AmmoItemDefinitionTag, true))
                        {
                            OutLootToDrop.Add(UFortKismetLibrary::CreateItemEntry(AmmoData, UFortScalableFloatUtils::GetValueAtLevel(WPACD.SpawnCount, 0.0f), 0));
                            break;
                        }
                    }
                }
            }
        }


    }

    void PickLootDropsHook(UObject* Obj, FFrame* Stack, bool* Ret)
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

        PickLootDrops(OutLootToDrop, TierGroupName, ForcedLootTier);

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

    bool ContainerSpawnLoot(ABuildingContainer* Container)
    {
        TArray<FFortItemEntry> Loot;
        PickLootDrops(Loot, Container->SearchLootTierGroup, -1);
        auto Position = UKismetMathLibrary::TransformLocation(Container->GetTransform(), Container->LootSpawnLocation_Athena);
        int32 NumSteps = round(Container->LootTossConeHalfAngle_Athena / 18.0f);
        for (auto& Entry : Loot)
        {
            auto Pickup = Utils::SpawnActor<AFortPickupAthena>(Position);
            Pickup->PrimaryPickupItemEntry = Entry;
            Pickup->OnRep_PrimaryPickupItemEntry();
            UFortKismetLibrary::TossPickupFromContainer(Container, Container, Pickup, NumSteps, UKismetMathLibrary::RandomInteger(NumSteps), Container->LootTossConeHalfAngle_Athena, Container->LootTossDirection_Athena, Container->LootTossSpeed_Athena, false);
        }
        Loot.Free();

        return true;
    }

    void Init()
    {
        auto GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
        auto Playlist = GameState->CurrentPlaylistInfo.BasePlaylist;

        AddLTD(Utils::GetSoftPtr(Playlist->LootTierData));
        AddLP(Utils::GetSoftPtr(Playlist->LootPackages));

        for (auto GameFeatureData : GameFeatures::Active)
        {
            if (!GameFeatureData->IsA(UFortGameFeatureData::StaticClass()))
                continue;

            auto Data = (UFortGameFeatureData*)GameFeatureData;

            bool AddedLoot = false;
            for (auto& thing : Data->PlaylistOverrideLootTableData)
            {
                if (UBlueprintGameplayTagLibrary::HasTag(Playlist->GameplayTagContainer, thing.Key(), true))
                {
                    AddedLoot = true;
                    AddLTD(Utils::GetSoftPtr(thing.Value().LootTierData));
                    AddLP(Utils::GetSoftPtr(thing.Value().LootPackageData));
                    break;
                }
            }

            if (!AddedLoot)
            {
                AddLTD(Utils::GetSoftPtr(Data->DefaultLootTableData.LootTierData));
                AddLP(Utils::GetSoftPtr(Data->DefaultLootTableData.LootPackageData));
            }
        }

        Hook::UFunc("Function FortniteGame.FortKismetLibrary.PickLootDrops", PickLootDropsHook);
        Hook::UFunc("Function FortniteGame.FortKismetLibrary.K2_SpawnPickupInWorld", K2_SpawnPickupInWorld);

        Hook::Function(Utils::Offset(0x6300760), ContainerSpawnLoot);

        auto FloorLoot1 = UObject::FindClassFast("Tiered_Athena_FloorLoot_01_C");
        auto FloorLoot2 = UObject::FindClassFast("Tiered_Athena_FloorLoot_Warmup_C");

        auto Containers = Utils::GetAllActorsOfClass<ABuildingContainer>();
        for (auto Container : Containers)
        {
            if (Container->IsA(FloorLoot1) || Container->IsA(FloorLoot2))
                ContainerSpawnLoot(Container);
        }
    }
}
