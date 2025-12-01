#include <SDK/FortniteGame_classes.hpp>

using namespace SDK;
    
namespace Utils
{
    template <typename T>
    T* SpawnObject(UObject* Outer)
    {
        return (T*)UGameplayStatics::SpawnObject(T::StaticClass(), Outer);
    }

    UPlayerController* GetLocalPlayerController()
    {
        return UWorld::GetWorld()->OwningGameInstance->LocalPlayers[0]->PlayerController;
    }
}
