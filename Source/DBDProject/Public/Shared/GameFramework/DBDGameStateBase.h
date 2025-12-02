// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/GameStateBase.h"
#include "../../MMJ/Object/GAS/ObjAbilitySystemComponent.h"
#include "../../MMJ/Object/GAS/ObjAttributeSet.h"
#include "DBDGameStateBase.generated.h"



/**
 * 
 */
UCLASS()
class DBDPROJECT_API ADBDGameStateBase : public AGameStateBase
{
	GENERATED_BODY()
public:
	ADBDGameStateBase();

#pragma region Information:
	UPROPERTY()
	class UDBDCharacterSubsystem* CharacterSubsystem;
	UPROPERTY()
	class UDBDObjectObserver* ObjectObserver;

#pragma endregion
	
protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

#pragma region EndGame:
public:
	
private:
	// 탈출 타이머 발동 시간(클라이언트 복제용)
	UPROPERTY(ReplicatedUsing = OnRep_RemainingEscapeTime)
	float RemainingEscapeTime;

	// 탈출 타이머가 느려졌는지(클라이언트 복제용)
	UPROPERTY(ReplicatedUsing = OnRep_IsSlowEscape)
	bool bIsSlowEscape = false;

public:
	// RemainingEscapeTime 업데이트
	UFUNCTION()
	void SetRemainingEscapeTime(float NewTime);

	// 탈출 타이머의 남은 시간을 리플리케이트
	UFUNCTION()
	void OnRep_RemainingEscapeTime(float NewTime);

	// bIsSlowEscape 업데이트
	UFUNCTION()
	void SetIsSlowEscape(bool bIsSlow);

	// 클라이언트의 EscapeTimerUI 이미지를 교체
	UFUNCTION()
	void OnRep_IsSlowEscape();
	
	UPROPERTY(ReplicatedUsing = OnRep_bIsGameEnded)
	bool bIsGameEnded;

	UFUNCTION()
	void SetGameEnd();

	UFUNCTION()
	void OnRep_bIsGameEnded();

	UFUNCTION(Server, Reliable)
	void MoveToEndGameLevel();

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	UUserWidget* EndGameWidget;
#pragma endregion
	
protected:
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;

public:

#pragma region GameFlow:
	UPROPERTY()
	TArray<class ASurvivorCharacter*> Survivors;
	
	UPROPERTY()
	class AKillerCharacter* Killer;
		
	UPROPERTY()
	TArray<class AObj_Generator*> Generators;
	
	UPROPERTY()
	TArray<class AObj_ExitDoor*> ExitDoors;

	UPROPERTY()
	TArray<class AObj_Hook*> Hooks;

	UPROPERTY()
	TArray<class AObj_Exit*> Exits;

	UPROPERTY(ReplicatedUsing = OnRep_RequiredGeneratorRepairCount)
	int32 RequiredGeneratorRepairCount;

	UFUNCTION()
	void OnRep_RequiredGeneratorRepairCount();

#pragma endregion

};

