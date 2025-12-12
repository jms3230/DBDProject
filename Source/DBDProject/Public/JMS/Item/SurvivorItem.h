// Fill out your copyright notice in the Description page of Project Settings.

// TODO: 아이템 교체 시 Loadout정보 변경
#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "Shared/Interface/Interactable.h"
#include "SurvivorItem.generated.h"

struct FGameplayAbilitySpecHandle;
class UGameplayAbility;
class ASurvivorCharacter;
class USurvivorAbilitySystemComponent;
class UItemAttributeSet;
class UItemInteractableComponent;
class UItemAbilitySystemComponent;
class UItemAddonComponent;
/**
 * 
 */

UCLASS()
class DBDPROJECT_API ASurvivorItem : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	ASurvivorItem();

protected:
	UPROPERTY(EditAnywhere, Category="Item", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* Mesh;

	UPROPERTY(EditAnywhere, Category="Item", meta = (AllowPrivateAccess = "true"))
	FName ItemID;

	UPROPERTY(VisibleAnywhere, Category = "Interactable")
	UItemInteractableComponent* ItemInteractableComponent;
protected:
	UPROPERTY(VisibleAnywhere, Category = "Addon")
	UItemAddonComponent* Addon1;
	UPROPERTY(VisibleAnywhere, Category = "Addon")
	UItemAddonComponent* Addon2;

	UPROPERTY(EditDefaultsOnly, Category = "Item")
	TArray<TSubclassOf<UGameplayAbility>> UseItemAbilities;

public:
	// IInteractable
	virtual UInteractableComponent* GetInteractableComponent() const override;

	void AttachAddon1(UItemAddonComponent* InAddon1);
	void AttachAddon2(UItemAddonComponent* InAddon2);
	UFUNCTION(BlueprintNativeEvent)
	void OnInitialized();
	virtual void OnEquipItem();
	virtual void OnStartUsingItem();
	virtual void OnEndUsingItem();
	virtual void OnDropItem();

	USkeletalMeshComponent* GetMesh() const;
	UFUNCTION(BlueprintPure, Category = "Item")
	FName GetItemID() const;
	UPROPERTY(EditDefaultsOnly, Category = "Item")
	FGameplayTag ItemTag;

	UFUNCTION(BlueprintPure, Category = "Item")
	UItemAddonComponent* GetAddon1() const;
	UFUNCTION(BlueprintPure, Category = "Item")
	UItemAddonComponent* GetAddon2() const;

	void AddMaxCharge(float Amount);
	void AddCurrentCharge(float Amount);
protected:
	virtual void BeginPlay() override;
	USurvivorAbilitySystemComponent* GetOwnerSurvivorAbilitySystemComponent();
	ASurvivorCharacter* GetOwnerSurvivor();
	UPROPERTY(EditDefaultsOnly, Category = "Item")
	float MaxCharge = 20.f;
	UPROPERTY(EditDefaultsOnly, Category = "Item")
	float CurrentCharge = 20.f;
private:
	UPROPERTY()
	TArray<FGameplayAbilitySpecHandle> GrantedSpecHandles;
};
