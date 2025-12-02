// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "DBDObjectAuraSubsystem.generated.h"


class ADBDCharacter;
struct FStencilMap;
class ASurvivorCharacter;
class AKillerCharacter;
class ADBDObject;
class ADBDPlayerState;

/**
 * 오라를 제어하는 서브시스템
 * 사용법 : SetAuraState(오라가 켜져야할 오브젝트 또는 캐릭터, 오라를 봐야할 플레이어의 플레이어스테이트, 오라 색상, 오라가 켜져있는 시간 0 - 무제한)
 * 
 */
UCLASS()
class DBDPROJECT_API UDBDObjectAuraSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	void SetAuraState(ADBDObject* Object, APlayerState* PlayerState, int32 StencilValue, float AuraDuration = 0.f);
	void SetAuraState(ADBDCharacter* Character, APlayerState* PlayerState, int32 StencilValue, float AuraDuration = 0.f);
	void UnSetAuraState(ADBDObject* Object, const FStencilMap StencilMap);
	void UnSetAuraState(ADBDCharacter* Character, const FStencilMap StencilMap);
	void UnSetAuraState(ADBDObject* Object, APlayerState* PlayerState);
	void UnSetAuraState(ADBDObject* Object, APlayerState* PlayerState, int32 StencilValue);

	// UPROPERTY(EditAnywhere, Category = "AuraSubSystem Data")
	// UDBDMPC* AuraMPC;
	//
	// UFUNCTION()
	// UDBDMPC* GetAuraMPC();
private:
};
