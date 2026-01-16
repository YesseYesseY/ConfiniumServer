#include <Minhook.h>

namespace Hook
{
    template <typename T = void*>
    void Function(uintptr_t Addr, void* Hook, T* Original = nullptr)
    {
        MH_CreateHook((LPVOID)Addr, Hook, (LPVOID*)Original);
        MH_EnableHook((LPVOID)Addr);
    }

    template <typename T2 = void*>
    void VTable(void** vTable, int32 Index, void* Hook, T2* Original = nullptr)
    {
        auto Addr = (LPVOID)(int64(vTable) + (Index * 8));
        if (Original)
            *Original = (T2)vTable[Index];
        DWORD yes;
        VirtualProtect(Addr, 8, PAGE_EXECUTE_READWRITE, &yes);
        vTable[Index] = Hook;
        VirtualProtect(Addr, 8, yes, &yes);
    }

    template <typename T, typename T2 = void*>
    void VTable(int32 Index, void* Hook, T2* Original = nullptr)
    {
        VTable((void**)T::GetDefaultObj()->VTable, Index, Hook, Original);
    }

    template <typename T, typename T2 = void*>
    void AllVTables(int32 Index, void* Hook, T2* Original = nullptr)
    {
        for (int i = 0; i < UObject::GObjects->Num(); i++)
        {
            auto Object = UObject::GObjects->GetByIndex(i);
            if (!Object || !Object->HasTypeFlag(EClassCastFlags::Class)) continue;

            auto Class = (UClass*)Object;
            if (Class->IsSubclassOf(T::StaticClass()) && Class->DefaultObject)
            {
                VTable((void**)Class->DefaultObject->VTable, Index, Hook, Original);
            }
        }
    }

    template <typename T = void*>
    void UFunc(UFunction* Func, void* Hook, T* Original = nullptr)
    {
        if (Original)
            *Original = (T)Func->ExecFunction;

        Func->ExecFunction = (UFunction::FNativeFuncPtr)Hook;
    }

    template <typename T = void*>
    void UFunc(std::string FuncName, void* Hook, T* Original = nullptr)
    {
        auto Func = UObject::FindObject<UFunction>(FuncName, EClassCastFlags::Function);
        if (Func)
            UFunc(Func, Hook, Original);
    }
}

struct FOutputDevice
{
    void** VTable;
	bool bSuppressEventTag;
	bool bAutoEmitLineTerminator;
};

struct FFrame : FOutputDevice
{
	UFunction* Node;
	UObject* Object;
	uint8* Code;
	uint8* Locals;

	FProperty* MostRecentProperty;
	uint8* MostRecentPropertyAddress;
	uint8* MostRecentPropertyContainer;

    void Step(void* a1)
    {
        if (Code)
        {
            static void (*step)(void* a1, void* a2, void* a3) = decltype(step)(Utils::Offset(0xC46B20));
            step(this, Object, a1);
        }
        else
        {
            auto This = (int64)this;
            *(int64*)(This + 128) = *(int64*)(*(int64*)(This + 128) + 32i64);
            static void (*sep)(void*, void*) = decltype(sep)(Utils::Offset(0xCFD36C));
            sep(this, a1);
        }
    }

    void End()
    {
        Code += !!Code;
    }
};
