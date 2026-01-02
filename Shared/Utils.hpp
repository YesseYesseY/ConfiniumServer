#include <SDK/FortniteGame_classes.hpp>

using namespace SDK;
    
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
	T* SpawnActorClass(UClass* ActorClass, FVector Location = { 0,0,0 }, FRotator Rotaion = { 0,0,0 }, FVector Scale = { 1,1,1 }, AActor* Owner = nullptr, ESpawnActorCollisionHandlingMethod ESACHM = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn)
	{
		FTransform translivesmatter = {};
		translivesmatter.Translation = Location;
		translivesmatter.Rotation = UKismetMathLibrary::Conv_RotatorToQuaternion(Rotaion);
		translivesmatter.Scale3D = Scale;
		return SpawnActorClass<T>(ActorClass, translivesmatter, Owner, ESACHM);
	}

	template <typename T>
	T* SpawnActor(FVector Location = { 0,0,0 }, FRotator Rotaion = { 0,0,0 }, FVector Scale = { 1,1,1 }, AActor* Owner = nullptr, ESpawnActorCollisionHandlingMethod ESACHM = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn)
	{
		return SpawnActorClass<T>(T::StaticClass(), Location, Rotaion, Scale, Owner, ESACHM);
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

    template <typename T>
    T* GetSoftPtr(TSoftObjectPtr<T> SoftPtr)
    {
        auto ret = SoftPtr.Get();

        if (!ret)
            ret = (T*)UKismetSystemLibrary::LoadAsset_Blocking((TSoftObjectPtr<UObject>)SoftPtr);

        return (T*)ret;
    }

    UClass* GetSoftPtr(TSoftClassPtr<UClass>& SoftPtr)
    {
        auto ret = SoftPtr.Get();

        if (!ret)
            ret = UKismetSystemLibrary::LoadClassAsset_Blocking(SoftPtr);

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
}
