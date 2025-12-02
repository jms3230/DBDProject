// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Shared/GAS/GA/DBDGameplayAbility.h"
#include "SurvivorGameplayAbility.generated.h"

class ASurvivorItem;
class AKillerCharacter;
class UInteractableComponent;
class ASurvivorCharacter;
class USkillCheckComponent;
class USurvivorAbilitySystemComponent;
class UInteractorComponent;
/**
 * 
 */
UCLASS()
class DBDPROJECT_API USurvivorGameplayAbility : public UDBDGameplayAbility
{
	GENERATED_BODY()

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                        const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
	                        bool bWasCancelled) override;
	UFUNCTION(BlueprintPure, Category="SurvivorGameplayAbility")
	UInteractableComponent* GetInteractableComponentFromActorInfo() const;

	UFUNCTION(BlueprintPure, Category="SurvivorGameplayAbility")
	USkillCheckComponent* GetSkillCheckComponentFromActorInfo() const;

	UFUNCTION(BlueprintPure, Category="SurvivorGameplayAbility")
	USurvivorAbilitySystemComponent* GetSurvivorAbilitySystemComponentFromActorInfo() const;
	UFUNCTION(BlueprintPure, Category="SurvivorGameplayAbility")
	UAnimInstance* GetAnimInstance() const;

	ASurvivorCharacter* GetSurvivorCharacterFromActorInfo() const;

	AKillerCharacter* GetKillerCharacterFromObserver() const;

	ASurvivorItem* GetEquippedItemFromActorInfo() const;
#pragma region UI
protected:
	virtual void UpdateWidgetData();
	
	float ProgressPercentage = 0.f;
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	FText AbilityDescription;
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	bool bShowProgressWidgetOnActivated = false;
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	bool bShowNameOnlyWidgetOnActivated = false;
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	float WidgetUpdateInterval = 0.01f;
private:
	FTimerHandle WidgetUpdateTimerHandle;
#pragma endregion 
#pragma region CollisionSettings

protected:
	void SetIgnoreOtherCharacterCollision();

private:
	bool bIsIgnoreCharacterSet = false;
#pragma endregion
#pragma region Transform Settings

protected:
	void LookAt(AActor* TargetActor) const;
	void LookAtKiller();
	void AttachToKiller(FName SocketName);
#pragma endregion
#pragma region VisualSettings
	UPROPERTY(EditDefaultsOnly, Category = "SurvivorGameplayAbility")
	bool bHideItemMesh = false;
#pragma endregion
};
