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

    void PickLootDrops_AddToOut(TArray<FFortItemEntry>& OutLootToDrop, UFortItemDefinition* ItemDef, int32 Count)
    {
        auto MaxStackSize = UFortScalableFloatUtils::GetValueAsInteger(ItemDef->MaxStackSize, 0.0f);

        for (auto& Entry : OutLootToDrop)
        {
            if (Count <= 0)
                break;

            if (Entry.ItemDefinition != ItemDef)
                continue;

            if (Entry.Count >= MaxStackSize)
                continue;

            auto ToAdd = min(MaxStackSize - Entry.Count, Count);
            Entry.Count += ToAdd;
            Count -= ToAdd;
        }

        while (Count > 0)
        {
            auto ToDrop = min(MaxStackSize, Count);
            OutLootToDrop.Add(UFortKismetLibrary::CreateItemEntry(ItemDef, ToDrop, 0));
            Count -= ToDrop;
        }
    }

    void PickLootDrops_AddToOut(TArray<FFortItemEntry>& OutLootToDrop, LPData& Data)
    {
        PickLootDrops_AddToOut(OutLootToDrop, Data.ItemDefinition, Data.Count);
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

                PickLootDrops_AddToOut(OutLootToDrop, toadd);
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
                    PickLootDrops_AddToOut(OutLootToDrop, toadd);

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
                            PickLootDrops_AddToOut(OutLootToDrop, AmmoData, UFortScalableFloatUtils::GetValueAtLevel(WPACD.SpawnCount, 0.0f));
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

        *Ret = true; // According to AthenaFunctionLibrary_C::SpawnLootFromTable when PickLootDrops returns false it prints NoLootPackageFound.
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
        auto Position = UKismetMathLibrary::TransformLocation(Container->GetTransform(), Container->LootSpawnLocation_Athena + Container->LootSpawnLocation);
        auto a2 = Container->LootTossConeHalfAngle_Athena;
        int32 NumSteps = floor(Container->LootTossConeHalfAngle_Athena / 18.0f);
        for (auto& Entry : Loot)
        {
            auto Pickup = Utils::SpawnActor<AFortPickupAthena>(Position);
            Pickup->PrimaryPickupItemEntry = Entry;
            Pickup->OnRep_PrimaryPickupItemEntry();
            int32 Step = UKismetMathLibrary::RandomInteger(NumSteps);
            UFortKismetLibrary::TossPickupFromContainer(Container, Container, Pickup, NumSteps, Step, Container->LootTossConeHalfAngle_Athena, Container->LootTossDirection_Athena, Container->LootTossSpeed_Athena, Container->bForceHidePickupMinimapIndicator);
        }
        Loot.Free();

        Container->bAlreadySearched = true;
        Container->OnRep_bAlreadySearched();

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
            for (int i = 0; i < Container->PotentialRandomUpgrades.Num(); i++)
            {
                auto& Upgrade = Container->PotentialRandomUpgrades[i];
                if (!UFortScalableFloatUtils::GetValueAsBool(Upgrade.Enabled, 0.0f))
                    continue;

                auto SpawnChance = UFortScalableFloatUtils::GetValueAtLevel(Upgrade.ChanceToApplyPerContainer, 0.0f);
                if (!UKismetMathLibrary::RandomBoolWithWeight(SpawnChance / 100.0f))
                    continue;

                Container->ChosenRandomUpgrade = i;
                Container->OnRep_ChosenRandomUpgrade();

                Container->ReplicatedLootTier = i + 2;
                Container->OnRep_LootTier();

                Container->SearchLootTierGroup = Upgrade.LootTierGroupIfApplied;

                break;
            }

            if (Container->bStartAlreadySearched_Athena)
                ContainerSpawnLoot(Container);
        }
    }
}
