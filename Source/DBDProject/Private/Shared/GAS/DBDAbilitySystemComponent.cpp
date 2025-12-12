// Fill out your copyright notice in the Description page of Project Settings.


#include "Shared/GAS/DBDAbilitySystemComponent.h"

#include "Shared/DataAsset/DA_DBDASCData.h"

UDBDAbilitySystemComponent::UDBDAbilitySystemComponent()
{
	
}

void UDBDAbilitySystemComponent::AuthApplyGameplayEffect(TSubclassOf<UGameplayEffect> GameplayEffect, int level)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		FGameplayEffectSpecHandle EffectSpecHandle = MakeOutgoingSpec(GameplayEffect, level, MakeEffectContext());
		ApplyGameplayEffectSpecToSelf(*EffectSpecHandle.Data.Get());
	}
}

void UDBDAbilitySystemComponent::ServerSideInit()
{
	InitializeBaseAttributes();
	ApplyInitializeEffects();
	OperatingInitializedAbilities();
}

void UDBDAbilitySystemComponent::OperatingInitializedAbilities()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	if (DBDASCData)
	{
		for (const TSubclassOf<UGameplayAbility>& GAClass : DBDASCData->InitializedAbilities)
		{
			FGameplayAbilitySpec Spec = FGameplayAbilitySpec(GAClass, 1, INDEX_NONE, nullptr);
			GiveAbilityAndActivateOnce(Spec, nullptr);
		}

		for (const TSubclassOf<UGameplayAbility>& GAClass : DBDASCData->PassiveAbilities)
		{
			GiveAbility(FGameplayAbilitySpec(GAClass, 1, INDEX_NONE, nullptr));
		}
	}
	GrantInputAbilities();
}

void UDBDAbilitySystemComponent::ApplyInitializeEffects()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	for (const TSubclassOf<UGameplayEffect>& GEClass : DBDASCData->InitialEffects)
	{
		FGameplayEffectSpecHandle EffectSpecHandle = MakeOutgoingSpec(GEClass, 1, MakeEffectContext());
		ApplyGameplayEffectSpecToSelf(*EffectSpecHandle.Data.Get());
	}
}
