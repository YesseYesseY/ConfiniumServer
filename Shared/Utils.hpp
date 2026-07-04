#include <SDK/FortniteGame_classes.hpp>

using namespace SDK;

#define MsgBox(...) MessageBoxA(NULL, std::format(__VA_ARGS__).c_str(), "ConfiniumServer", MB_OK)

namespace Utils
{
    template <typename T>
    T* SpawnObject(UObject* Outer)
    {
        return (T*)UGameplayStatics::SpawnObject(T::StaticClass(), Outer);
    }

	template <typename T>
	T* SpawnActorClass(UClass* ActorClass, FTransform trans, AActor* Owner = nullptr, ESpawnActorCollisionHandlingMethod ESACHM = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn)
	{
		AActor* begin = UGameplayStatics::BeginDeferredActorSpawnFromClass(UWorld::GetWorld(), ActorClass, trans, ESACHM, Owner);
		if (begin)
		{
			return (T*)UGameplayStatics::FinishSpawningActor(begin, trans);
		}
		return nullptr;
	}

	template <typename T>
	T* SpawnActorClass(UClass* ActorClass, FVector Location = { 0,0,0 }, FRotator Rotation = { 0,0,0 }, AActor* Owner = nullptr, FVector Scale = { 1,1,1 }, ESpawnActorCollisionHandlingMethod ESACHM = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn)
	{
		FTransform translivesmatter = {};
		translivesmatter.Translation = Location;
		translivesmatter.Rotation = UKismetMathLibrary::Conv_RotatorToQuaternion(Rotation);
		translivesmatter.Scale3D = Scale;
		return SpawnActorClass<T>(ActorClass, translivesmatter, Owner, ESACHM);
	}

	template <typename T>
	T* SpawnActor(FVector Location = { 0,0,0 }, FRotator Rotaion = { 0,0,0 }, FVector Scale = { 1,1,1 }, AActor* Owner = nullptr, ESpawnActorCollisionHandlingMethod ESACHM = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn)
	{
		return SpawnActorClass<T>(T::StaticClass(), Location, Rotaion, Owner, Scale, ESACHM);
	}

    APlayerController* GetLocalPlayerController()
    {
        return UWorld::GetWorld()->OwningGameInstance->LocalPlayers[0]->PlayerController;
    }

    template <typename T>
    int32 GetNumActorsOfClass()
    {
        return UFortKismetLibrary::GetNumActorsOfClass(UWorld::GetWorld(), T::StaticClass());
    }

    template <typename T>
    TArray<T*> GetAllActorsOfClass()
    {
        TArray<AActor*> ret;
        UGameplayStatics::GetAllActorsOfClass(UWorld::GetWorld(), T::StaticClass(), &ret);
        return *(TArray<T*>*)(&ret);
    }

    template <typename T>
    void** GetVTable()
    {
        return (void**)T::GetDefaultObj()->VTable;
    }

    inline uintptr_t Offset(uintptr_t off)
    {
        return InSDKUtils::GetImageBase() + off;
    }

    UFortAssetManager* GetAssetManager()
    {
        return (UFortAssetManager*)UEngine::GetEngine()->AssetManager;
    }

    void MarkArrayDirty(FFastArraySerializer* arr)
    {
        static void (*InternalMarkArrayDirty)(FFastArraySerializer*) = decltype(InternalMarkArrayDirty)(Utils::Offset(0x1496200));
        InternalMarkArrayDirty(arr);
    }

    void MarkItemDirty(FFastArraySerializer* arr, FFastArraySerializerItem* item)
    {
        static void (*InternalMarkItemDirty)(FFastArraySerializer*, FFastArraySerializerItem*) = decltype(InternalMarkItemDirty)(Utils::Offset(0x12a6388));
        InternalMarkItemDirty(arr, item);
    }

    template <typename T>
    T* GetSoftPtr(TSoftObjectPtr<T> SoftPtr)
    {
        T* ret = nullptr;

        if (SoftPtr.WeakPtr.ObjectIndex > 0)
        {
            ret = SoftPtr.Get();
        }

        if (!ret)
        {
            ret = (T*)UKismetSystemLibrary::LoadAsset_Blocking((TSoftObjectPtr<UObject>)SoftPtr);
        }

        return (T*)ret;
    }

    UClass* GetSoftPtr(TSoftClassPtr<UClass>& SoftPtr)
    {
        UClass* ret = nullptr;

        if (SoftPtr.WeakPtr.ObjectIndex)
        {
            ret = SoftPtr.Get();
        }

        if (!ret)
        {
            ret = UKismetSystemLibrary::LoadClassAsset_Blocking((TSoftClassPtr<UClass>)SoftPtr);
        }

        return ret;
    }

    template <typename T>
    T* FindFirstNonDefaultObject()
    {
        for (int i = 0; i < UObject::GObjects->Num(); i++)
        {
            auto Object = UObject::GObjects->GetByIndex(i);
            if (!Object || Object->IsDefaultObject()) continue;

            if (Object->IsA(T::StaticClass()))
                return (T*)Object;
        }
    }

    template <typename T>
    T* FindDataTableRow(UDataTable* DataTable, FName Name)
    {
        for (auto thing : DataTable->RowMap)
        {
            if (thing.Key() == Name)
            {
                return (T*)thing.Value();
            }
        }

        return nullptr;
    }

    FFortRangedWeaponStats* GetRangedStats(UFortWeaponRangedItemDefinition* ItemDef)
    {
        return FindDataTableRow<FFortRangedWeaponStats>(ItemDef->WeaponStatHandle.DataTable, ItemDef->WeaponStatHandle.RowName);
    }

    template <typename T = UObject>
    T* FindObjectFast(const std::string& Name, EClassCastFlags RequiredType = EClassCastFlags::None, EClassCastFlags ExcludedType = EClassCastFlags::Package)
    {
        for (int i = 0; i < UObject::GObjects->Num(); ++i)
        {
            UObject* Object = UObject::GObjects->GetByIndex(i);
        
            if (!Object || Object->HasTypeFlag(ExcludedType))
                continue;

            if (Object->HasTypeFlag(RequiredType) && Object->GetName() == Name)
                return (T*)Object;
        }

        return nullptr;
    }

    bool RandomMinMax(float Min, float Max)
    {
        auto Chance = UKismetMathLibrary::RandomFloatInRange(Min, Max);
        if (Chance <= 0.0f)
            return false;

        return UKismetMathLibrary::RandomFloatInRange(0.0f, 100.0f) <= Chance;
    }

    std::string GetName(UObject* Obj)
    {
        return Obj ? Obj->Name.GetRawString() : "None";
    }

    std::string GetFullName(UObject* Obj)
    {
        if (Obj && Obj->Class)
        {
            std::string Temp;

            for (UObject* NextOuter = Obj->Outer; NextOuter; NextOuter = NextOuter->Outer)
            {
                Temp = GetName(NextOuter) + "." + Temp;
            }

            std::string Name = GetName(Obj->Class);
            Name += " ";
            Name += Temp;
            Name += GetName(Obj);

            return Name;
        }

        return "None";
    }

    UClass* GetPickupClass()
    {
        static UClass* Ret = UFortKismetLibrary::GetSubGame(UWorld::GetWorld()) == ESubGame::Athena ? AFortPickupAthena::StaticClass() : AFortPickup::StaticClass();
        return Ret;
    }
}

template<>
struct std::hash<FName>
{
    std::size_t operator()(const FName& name) const noexcept
    {
        return std::hash<uint64_t>{}(*(uint64_t*)this);
    }
};
