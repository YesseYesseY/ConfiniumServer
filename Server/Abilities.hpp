namespace Abilities
{
    // I miss K2_GiveAbility from later ue5 :(
    void GiveAbility(UAbilitySystemComponent* Component, UClass* AbilityClass)
    {
        static FGameplayAbilitySpecHandle (*NativeFunc)(UAbilitySystemComponent* Component, FGameplayAbilitySpecHandle*, FGameplayAbilitySpec& AbilitySpec) = decltype(NativeFunc)(Utils::Offset(0x12A5F48));

        FGameplayAbilitySpec spec = { -1, -1, -1, rand(), (UGameplayAbility*)AbilityClass->DefaultObject, 1, -1 };

        NativeFunc(Component, &spec.Handle, spec);
    }

    void InternalServerTryActivateAbility(UAbilitySystemComponent* Component, FGameplayAbilitySpecHandle Handle, bool InputPressed, FPredictionKey& PredictionKey, FGameplayEventData* TriggerEventData)
    {
        static FGameplayAbilitySpec* (*FindAbilitySpecFromHandle)(UAbilitySystemComponent* Component, FGameplayAbilitySpecHandle Handle) = decltype(FindAbilitySpecFromHandle)(Utils::Offset(0x1494C78));
        FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(Component, Handle);
        if (!Spec)
        {
            Component->ClientActivateAbilityFailed(Handle, PredictionKey.Current);
            return;
        }

        const UGameplayAbility* AbilityToActivate = Spec->Ability;

        if (!AbilityToActivate)
        {
            Component->ClientActivateAbilityFailed(Handle, PredictionKey.Current);
            return;
        }

        if (AbilityToActivate->NetSecurityPolicy == EGameplayAbilityNetSecurityPolicy::ServerOnlyExecution ||
            AbilityToActivate->NetSecurityPolicy == EGameplayAbilityNetSecurityPolicy::ServerOnly)
        {
            Component->ClientActivateAbilityFailed(Handle, PredictionKey.Current);
            return;
        }

        // TODO? ConsumeAllReplicatedData

        UGameplayAbility* InstancedAbility = nullptr;
        Spec->InputPressed = true;

        static bool (*InternalTryActivateAbility)(UAbilitySystemComponent*, FGameplayAbilitySpecHandle, FPredictionKey, UGameplayAbility**, void*, FGameplayEventData*) = decltype(InternalTryActivateAbility)(Utils::Offset(0x4EA33E4));
        if (InternalTryActivateAbility(Component, Handle, PredictionKey, &InstancedAbility, nullptr, TriggerEventData))
        {
        }
        else
        {
            Component->ClientActivateAbilityFailed(Handle, PredictionKey.Current);
            Spec->InputPressed = false;
            Utils::MarkArrayDirty(&Component->ActivatableAbilities);
        }
    }
}
