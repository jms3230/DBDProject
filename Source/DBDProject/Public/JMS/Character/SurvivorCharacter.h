// Fill out your copyright notice in the Description page of Project Settings.
// TODO: WidgetComponent 제거 후 HUD 클래스 만들어서 관리
// TODO: Tag, Attribute이벤트 ASC 관련 클래스로 이동
#pragma once

#include "CoreMinimal.h"
#include "InputActionValue.h"
#include "Shared/DBDStruct.h"
#include "Shared/Character/DBDCharacter.h"
#include "SkeletalMergingLibrary.h"
#include "SurvivorCharacter.generated.h"

struct FGameplayEventData;
class UDA_SurvivorMontage;
class UDBDObjectPoolComponent;
class ADBDObjectPool;
class APoolEntry_ScratchMark;
class AGenericObjectPool;
class UGameplayEffect;
class FSkeletalMeshMerge;
class UDBDMotionWarpingComponent;
class UMotionWarpingComponent;
class AObj_Hook;
struct FOnAttributeChangeData;
class UDBDHUD;
class UWidgetComponent;
class ADBDPlayerState;
class ASurvivorItem;
class USurvivorInteractableComponent;
class UInteractorComponent;
class USkillCheckComponent;
class USurvivorAttributeSet;
class USurvivorAbilitySystemComponent;
class USpringArmComponent;
class UCameraComponent;
class UDA_SurvivorASCData;
class UInputMappingContext;
class ADBDObject;
struct FGameplayTag;
enum class ESurvivorAbilityInputID : uint8;
class UDA_SurvivorInput;
struct FInputActionValue;
class UAnimInstance;
class USoundCue;
/**
 * 
 */


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FActionDelegate);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSprintTagUpdateDelegate, ASurvivorCharacter*, Survivor, int32, NewCount);

UCLASS()
class DBDPROJECT_API ASurvivorCharacter : public ADBDCharacter
{
	GENERATED_BODY()

public:
	ASurvivorCharacter(const FObjectInitializer& ObjectInitializer);

	// IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	// IInteractable
	virtual UInteractableComponent* GetInteractableComponent() const;
	// IInteractor
	virtual EPlayerRole GetInteractorRole() const;

	USkillCheckComponent* GetSkillCheckComponent() const;

	UPROPERTY(EditDefaultsOnly, Category="DataControl")
	UDA_SurvivorMontage* SurvivorMontageData;

private:
	UPROPERTY(VisibleDefaultsOnly, Category="View")
	USurvivorAbilitySystemComponent* SurvivorAbilitySystemComponent;

	UPROPERTY()
	USurvivorAttributeSet* SurvivorAttributeSet;

public:
	//KMJ_1021_Delegate 바인딩을 위해 만듬
	USurvivorAttributeSet* GetSurvivorAttributeSet() { return SurvivorAttributeSet; }

private:
	UPROPERTY(VisibleDefaultsOnly, Category="Components")
	USkillCheckComponent* SkillCheckComponent_BPCorruption;

	UPROPERTY(VisibleDefaultsOnly, Category="Components")
	USurvivorInteractableComponent* SurvivorInteractableComponent;

	UPROPERTY(VisibleDefaultsOnly, Category="View")
	USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleDefaultsOnly, Category="View")
	UCameraComponent* FollowCam;

	UPROPERTY(EditDefaultsOnly, Category="DataControl", meta=(AllowPrivateAccess="true"))
	UDA_SurvivorInput* InputData;

	// UPROPERTY(VisibleAnywhere, Category="UI")
	// UDBDHUD* TempPrototypeWidget;

	UPROPERTY(EditAnywhere, Category="Temp")
	TSubclassOf<UDBDHUD> TempPrototypeWidgetClass;
#pragma region ScratchMark

private:
	UPROPERTY(EditDefaultsOnly, Category = "Footprint Scratchmark", meta=(AllowPrivateAccess="true"))
	TSubclassOf<APoolEntry_ScratchMark> ScratchMarkClass;

	UPROPERTY(VisibleAnywhere, Category = "Footprint Scratchmark")
	UDBDObjectPoolComponent* ScratchMarkPool;

public:
	APoolEntry_ScratchMark* GetScratchMarkFromPool();
#pragma endregion
#pragma region Hook

protected:
	UPROPERTY(VisibleAnywhere, ReplicatedUsing=OnRep_CurrentHook, Category="Hook")
	TObjectPtr<AObj_Hook> CurrentHook;
	UFUNCTION()
	void OnRep_CurrentHook(AObj_Hook* OldHook);

	void RegisterHook(AObj_Hook* Hook);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hook")
	UAnimMontage* HookInMontage;

public:
	AObj_Hook* GetCurrentHook() const;
#pragma endregion

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Animation")
	UAnimMontage* DeathMontage;

protected:
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void PawnClientRestart() override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	void BindGASChangeDelegate();
	// GAS - TagDelegate
#pragma region AttributeChangeEvent

protected:
	void HealProgressChanged(const FOnAttributeChangeData& OnAttributeChangeData);
	void DyingHPChanged(const FOnAttributeChangeData& OnAttributeChangeData);
	void MovementSpeedChanged(const FOnAttributeChangeData& OnAttributeChangeData);
	void SprintSpeedChanged(const FOnAttributeChangeData& OnAttributeChangeData);
#pragma endregion
#pragma region TagChangeEvent

protected:
	void DeathTagUpdate(const FGameplayTag Tag, int32 NewCount);
	void EscapeTagUpdate(const FGameplayTag Tag, int32 NewCount);
	void SprintTagUpdate(const FGameplayTag Tag, int32 NewCount);
	void CapturedTagUpdate(const FGameplayTag Tag, int32 NewCount);
	void HookPhase2TagUpdate(const FGameplayTag Tag, int32 NewCount);

public:
	FSprintTagUpdateDelegate SprintTagUpdateDelegate;
#pragma endregion
#pragma region Death
	void AfterDeathMontage();
#pragma endregion
protected:
	ADBDPlayerState* GetDBDPlayerState() const;

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

private:
	void AbilityInput(const FInputActionValue& InputActionValue, ESurvivorAbilityInputID InputID);
	UPROPERTY()
	FInputActionValue PreviousInputActionValue;

	void LookAction(const FInputActionValue& InputActionValue);
	void MoveAction(const FInputActionValue& InputActionValue);

	FVector GetLookRightDirection() const;
	FVector GetLookForwardDirection() const;
	FVector GetMoveForwardDirection() const;

public:
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
#pragma region ItemControl

public:
	// 캐릭터의 Equip, Drop에서 월드에 아이템을 배치하는 역할, 아이템 클래스에서 Owner의 태그를 업데이트하는 역할
	// 서버에서 실행
	void EquipItem(ASurvivorItem* ItemToEquip);
	// 서버에서 실행
	void DropItem(ASurvivorItem* ItemToPickUp);
	// 서버에서 실행
	void PickUpItem(ASurvivorItem* ItemToPickUp);
	// 서버에서 실행
	void StartUsingItem();
	UFUNCTION(Server, Reliable)
	void Server_ApplyUsingEffectToItem();
	void EndUsingItem();
	UFUNCTION(Server, Reliable)
	void Server_RemoveUsingEffectFromItem();

	UFUNCTION(Client, Reliable)
	void Client_UpdateCurrentItem(ASurvivorItem* NewItem, UItemAddonComponent* Addon1, UItemAddonComponent* Addon2);
	// JMS: UI수정: 델리게이트는 Component에서 관리
	// FEquippedItemChangedDelegate EquippedItemChangedDelegate;
	
	ASurvivorItem* GetEquippedItem() const;
	UPROPERTY(EditDefaultsOnly, Category="Item")
	FName ItemSocketName;

	// JMS: UI수정: Initialize도 컴포넌트에서 관리
	// void InitItemHUD();

protected:
	UPROPERTY(ReplicatedUsing=OnRep_EquippedItem,VisibleDefaultsOnly, Category="Item")
	TObjectPtr<ASurvivorItem> EquippedItem;
	// JMS: UI수정: Initialize도 컴포넌트에서 관리
	// void InitializeStartItemAfterWaitForReplicated();
	UFUNCTION()
	void OnRep_EquippedItem(ASurvivorItem* OldItem);
	UPROPERTY(EditDefaultsOnly, Category="Item")
	TSubclassOf<UGameplayEffect> GEforUseItem;
#pragma endregion

#pragma region Misc

public:
	// Movement Mode만 None, Walk 전환
	UFUNCTION(BlueprintCallable, Category = "SurvivorCharacterMisc")
	void MoveEnabled(bool bEnable);
	// Collision, Movement Mode, Gravity 활성/비활성 전환
	UFUNCTION(BlueprintCallable, Category = "SurvivorCharacterMisc")
	void SetCollisionAndGravityEnabled(bool bEnable);
	UFUNCTION(BlueprintPure, Category = "SurvivorCharacterMisc")
	bool IsCollisionAndGravityEnabled() const;
	UFUNCTION(BlueprintPure, Category = "SurvivorCharacterMisc")
	bool GetIsReadyToStart() const;
private:
	bool bIsCollisionAndGravityEnabled = true;
	bool bIsReadyToStart = false;
	void SetReady();
#pragma endregion
#pragma region StateControl

public:
	UFUNCTION(BlueprintCallable)
	void AttackSurvivor();
	UFUNCTION(BlueprintCallable)
	void SetSurvivorInjured();
	UFUNCTION(BlueprintCallable)
	void SetSurvivorDying();
	UFUNCTION(BlueprintCallable)
	void SetSurvivorNormal();
	UFUNCTION(BlueprintCallable)
	void CaptureSurvivor();
	UFUNCTION(BlueprintCallable)
	void AuthHookSurvivor(AObj_Hook* Hook);
	UFUNCTION(BlueprintCallable)
	void ReleaseSurvivor();
	UFUNCTION(BlueprintCallable)
	void SetSurvivorEscaped();
	UFUNCTION(BlueprintCallable)
	void SetSurvivorDead();

	void SetSurvivorMoving(bool IsMoving);
#pragma endregion

	void ServerSideInit();
	void ClientSideInit();
	bool IsLocallyControlledByPlayer() const;
#pragma region Skin

private:
	UPROPERTY(EditAnywhere, Category = "Skin")
	USkeletalMeshComponent* Skin;
	UPROPERTY(EditAnywhere, Category = "Skin")
	FSkeletalMeshMergeParams SkeletalMeshMergeParamsForSkin;

public:
	UFUNCTION(BlueprintCallable)
	USkeletalMeshComponent* GetSkin();
#pragma endregion

protected:
	void InitializeEquippedItem(FSurvivorItemInstanceInfo InitialItemInfo);
#pragma region TagControl

protected:
	UFUNCTION(BlueprintCallable)
	void AddUniqueTag(FGameplayTag Tag);
	UFUNCTION(BlueprintCallable)
	void AddTag(FGameplayTag Tag);

	UFUNCTION(Server, Reliable)
	void Server_AddUniqueTag(const FGameplayTag& Tag);
	UFUNCTION(Server, Reliable)
	void Server_AddTag(const FGameplayTag& Tag);

	UFUNCTION(BlueprintCallable)
	void RemoveTag(FGameplayTag Tag);
	UFUNCTION(BlueprintCallable)
	void RemoveTagAll(FGameplayTag Tag);

	UFUNCTION(Server, Reliable)
	void Server_RemoveTag(const FGameplayTag& Tag);
	UFUNCTION(Server, Reliable)
	void Server_RemoveTagAll(const FGameplayTag& Tag);

	UFUNCTION(BlueprintCallable)
	void PrintHasTag(FGameplayTag Tag);

	UFUNCTION(Server, Reliable)
	void Server_PrintHasTag(const FGameplayTag& Tag);

public:
	UFUNCTION(BlueprintCallable)
	void SendGameplayTagEventToSelf(FGameplayTag Tag, FGameplayEventData EventData);

	UFUNCTION(Server, Reliable)
	void Server_SendGameplayTagEventToSelf(const FGameplayTag& Tag, FGameplayEventData EventData);
#pragma endregion
#pragma region AuraControl
public:
	virtual void EnableAura(int32 StencilValue) override;
	virtual void DisableAura() override;
#pragma endregion
#pragma region ActionDelegates

public:
	void OnQuickAction();
	FActionDelegate OnQuickActionPlayed;
	void OnSprintStarted();
	FActionDelegate OnSprintStartedDelegate;
#pragma endregion

#pragma region MotionControl

public:
	void MoveToKiller(FName KillerSocket = FName(TEXT("joint_Char")));
	UFUNCTION(BlueprintCallable, Category = "SurvivorCharacter MotionControl")
	void HideItemMesh(bool bHide);
	UFUNCTION(BlueprintCallable, Category = "SurvivorCharacter MotionControl")
	void Anim_DisableMoveStart();
	UFUNCTION(BlueprintCallable, Category = "SurvivorCharacter MotionControl")
	void Anim_DisableMoveEnd();
	UFUNCTION(BlueprintCallable, Category = "SurvivorCharacter MotionControl")
	void SetMoveIgnoreKiller(bool bIgnore);
	UFUNCTION(BlueprintCallable, Category = "SurvivorCharacter MotionControl")
	void SetMoveIgnoreSurvivors(bool bIgnore);
protected:
	UFUNCTION(Client, Unreliable)
	void Client_DisableMoveStart();
	UFUNCTION(Client, Unreliable)
	void Client_DisableMoveEnd();

private:
	UPROPERTY(ReplicatedUsing=OnRep_IsMoveEnabled)
	bool bIsMoveEnabled = true;
	UFUNCTION()
	void OnRep_IsMoveEnabled(bool OldIsMoveEnabled);
#pragma endregion

#pragma region HeartBeat
protected:
	UPROPERTY(EditDefaultsOnly, Category = "HeartBeat")
	USoundCue* HeartBeatSound;
	UPROPERTY(EditDefaultsOnly, Category = "HeartBeat")
	float HeartBeatInterval = 0.5f;
	float HeartBeatVolume = 0.f;
	FTimerHandle HeartBeatTimerHandle;
	bool bIsPlayingHeartBeat = false;
	void PlayHeartBeatIfKillerNearby();
#pragma endregion 
};
