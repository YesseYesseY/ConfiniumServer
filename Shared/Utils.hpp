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

    // int32 GetNumActorsOfClass(const class UObject* WorldContextObject, TSubclassOf<class AActor> ActorClass);
    template <typename T>
    int32 GetNumActorsOfClass()
    {
        return UFortKismetLibrary::GetNumActorsOfClass(UWorld::GetWorld(), T::StaticClass());
    }
}
