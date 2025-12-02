// Fill out your copyright notice in the Description page of Project Settings.
// 상호작용을 실행할 캐릭터들에 달아서 사용합니다.
// Interactable을 SearchInterval마다 한번씩 SphereTrace로 찾아 가장 가까운 Interactable과 상호작용할 준비를 합니다.
// 상호작용이 시작될 때 InteractWithCurrentInteractable을 호출해주시고 (Searching정지 + Interactable에 신호)
// 상호작용이 끝나서 새로운 Interactable을 찾아야 할 때 EndInteraction을 호출해주세요.(Searching 재개 + Interactable에 신호)

// JMS: 로직들이 서버에서만 실행됩니다.(BeginPlay 참고)
// JMS: InputMapping을 버리고 Tag기반으로 바뀌었습니다.
// JMS: 인터랙션 시작 어빌리티는 반드시 Activation Required Tag에 필요한 Interactable_~태그를 만들어 붙여주세요
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"
#include "InteractorComponent.generated.h"


class UAbilitySystemComponent;
class IAbilitySystemInterface;
class USphereComponent;
class IInteractor;
class IInteractable;
class UInputMappingContext;
struct FMotionWarpingTarget;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class DBDPROJECT_API UInteractorComponent : public USphereComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UInteractorComponent();
	virtual void BeginPlay() override;
	
	UFUNCTION(BlueprintCallable)
	void InteractWithCurrentInteractable();
	UFUNCTION(BlueprintCallable)
	void EndInteraction();

	void SearchingEnabled(bool bEnabled);

protected:

	UFUNCTION()
	void CheckNearbyInteractable();

	void RegisterOverlappedInteractable(IInteractable* Interactable);
	void UnRegisterOverlappedInteractable(IInteractable* Interactable);
	
	IInteractor* GetOwnerAsInteractor();
	UAbilitySystemComponent* GetOwnerAbilitySystemComponent();

private:
	IInteractable* CurrentInteractable;
	// JMS: CurrentInteractable이 이전 값과 달라질 경우에만 RPC를 호출하여 업데이트 해보려고 합니다
	UFUNCTION(Client, Reliable)
	void Client_CurrentInteractableChanged(AActor* NewInteractable);

	UPROPERTY()
	UInputMappingContext* RegisteredInputMappingContext;
	
	void UpdateTag();

	bool bIsSearchingEnabled;
	float SearchInterval;
	FTimerHandle SearchTimerHandle;

	UPROPERTY(EditDefaultsOnly, Category = "Interactor")
	float SearchRadius;

	/*
	 *KMJ::수정하지 마세요...
	 *0917: IInteractable*은 서버랑 연동이 안 되서 연동용으로 만들었습니다
	*/
	//UPROPERTY(ReplicatedUsing=OnRep_CurrentInteractableActor)
	AActor* CurrentInteractableActor;
	//UFUNCTION()
	//void OnRep_CurrentInteractableActor();
public:
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	// JMS: Replicate되기 전에는 Null일 수 있으므로 예외처리를 해주세요
	UFUNCTION(BlueprintPure)
	AActor* GetCurrentInteractableActor();

	
#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
