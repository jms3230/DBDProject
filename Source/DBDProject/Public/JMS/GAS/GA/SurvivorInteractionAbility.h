// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "JMS/GAS/SurvivorGameplayAbility.h"
#include "Shared/Component/InteractorComponent.h"
#include "SurvivorInteractionAbility.generated.h"

enum class ESkillCheckResult : uint8;
class IInteractable;
/**
 * 
 */
UCLASS()
class DBDPROJECT_API USurvivorInteractionAbility : public USurvivorGameplayAbility
{
	GENERATED_BODY()

public:
	USurvivorInteractionAbility();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                             const FGameplayAbilityActivationInfo ActivationInfo,
	                             const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                        const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
	                        bool bWasCancelled) override;

#pragma region Interaction

protected:
	UFUNCTION(BlueprintPure, Category="GA")
	UInteractorComponent* GetInteractorComponentFromActorInfo() const;

	template <typename T>
	T* GetCachedCurrentInteractable() const
	{
		if (CachedCurrentInteractableActor)
		{
			return Cast<T>(CachedCurrentInteractableActor);
		}
		return nullptr;
	}

	template <typename T>
	T* GetCurrentInteractable() const
	{
		if (GetInteractorComponentFromActorInfo())
		{
			return Cast<T>(GetInteractorComponentFromActorInfo()->GetCurrentInteractableActor());
		}
		return nullptr;
	}

	void InteractStart();
	void EndOnInteractableTaskFinished() const;
private:
	UPROPERTY()
	UInteractorComponent* CachedInteractorComponent;
	UPROPERTY()
	AActor* CachedCurrentInteractableActor;
	bool bInteractStarted = false;

#pragma endregion
#pragma region SkillCheck

protected:
	UFUNCTION(BlueprintCallable, Category="SurvivorInteractionAbility")
	void SetRandomSkillCheckEnabledOnClient(float Frequency = 1.0f);
	UFUNCTION()
	void OnSkillCheckEnd(ESkillCheckResult Result);
	virtual void OnSkillCheckBad();
	UFUNCTION(Server, Reliable)
	virtual void Server_SendSkillCheckResult(ESkillCheckResult Result);
private:
	void SetSkillCheckTimer(float Frequency);
	void PlaySkillCheck();
	FTimerHandle SkillCheckTimerHandle;
	FDelegateHandle SkillCheckEndDelegateHandle;
	UPROPERTY()
	USkillCheckComponent* CachedSkillCheckComponent;
	
#pragma endregion
	UFUNCTION()
	virtual void OnGetHit(FGameplayEventData Payload);
};
