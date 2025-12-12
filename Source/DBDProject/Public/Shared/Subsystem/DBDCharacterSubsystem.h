// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "GameplayTagContainer.h"
#include "Subsystems/WorldSubsystem.h"
#include "DBDCharacterSubsystem.generated.h"

class APoolEntry_ScratchMark;
class UGameplayEffect;
struct FGameplayTagContainer;
struct FGameplayTag;
class ADBDCharacter;
/**
 * 
 */
class AKillerCharacter;
class ASurvivorCharacter;

USTRUCT()
struct FDBDCharacterAuraInfo
{
	GENERATED_BODY()

	UPROPERTY()
	ADBDCharacter* DBDCharacter;
	UPROPERTY()
	TSet<UObject*> AuraInstigators;
	int32 StencilValue;

	bool operator==(const FDBDCharacterAuraInfo& right) const;
	FDBDCharacterAuraInfo(ADBDCharacter* InDBDCharacter);
	FDBDCharacterAuraInfo() = default;
};

uint32 GetTypeHash(const FDBDCharacterAuraInfo& AuraInfo);

UCLASS()
class DBDPROJECT_API UDBDCharacterSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	void RegisterKiller(AKillerCharacter* KillerCharacter);
	void UnregisterKiller(AKillerCharacter* KillerCharacter);
	void RegisterSurvivor(ASurvivorCharacter* SurvivorCharacter);
	void UnregisterSurvivor(ASurvivorCharacter* SurvivorCharacter);

	void PrintAllCharacter();
	UFUNCTION(BlueprintPure)
	AKillerCharacter* GetKiller() const;
	UFUNCTION(BlueprintPure)
	ASurvivorCharacter* GetSurvivorByIndex(int32 Index) const;
	UFUNCTION(BlueprintPure)
	TArray<ASurvivorCharacter*> GetSurvivors();

	// JMS : 거리와 조건 태그들을 활용해 생존자의 오라를 활성화(EX: SelfCare, Bond)
	void EnableSurvivorAuraWithDistanceAndTag(UObject* AuraInstigator, ADBDCharacter* EffectOwner, float Distance,
	                                          FGameplayTagContainer RequiredTags = FGameplayTagContainer(),
	                                          FGameplayTagContainer BlockedTags = FGameplayTagContainer());
	// JMS : 일정 거리 내의 생존자 숫자에 따라 GameplayEffect 레벨을 다르게 하여 EffectTarget에게 적용(EX: Prove Thyself)
	void ApplyGEWithSurvivorWithinDistance(ADBDCharacter* Target, float Distance, TSubclassOf<UGameplayEffect> GE);

	// JMS : 일정 거리 내의 다른 생존자들에게 GameplayEffect를 적용(EX: Leader)
	void ApplyGEToSurvivorsWithinDistance(ADBDCharacter* EffectOwner, float Distance, TSubclassOf<UGameplayEffect> GE);

	// JMS : 생존자 발자국 활성화
	void EnableScratchMarkOnEverySurvivor();
	void EnableScratchMarkOnCurrentSurvivor(ASurvivorCharacter* Survivor);


	// MMJ : 캐릭터가 스폰될때마다 서버권한으로 태그체인지델리게이트에 바인딩하는 함수
	void BindingPlayerCharacter(ADBDCharacter* Player);
	void UnBindingPlayerCharacter(ADBDCharacter* Player);

private:
	UPROPERTY()
	TObjectPtr<AKillerCharacter> Killer;
	TArray<FDBDCharacterAuraInfo> CharacterAuraInfoContainer;
	UPROPERTY()
	TArray<TObjectPtr<ASurvivorCharacter>> Survivors;
#pragma region CharacterAuraSystem
	void RequestAuraRefresh();
	void RefreshAura();
	float AuraRefreshInterval = 0.05f;
	bool bIsAuraRefreshing = false;
	FTimerHandle AuraRefreshTimerHandle;
#pragma endregion
	// JMS : EnableSurvivorAuraWithDistanceAndTag 구현을 위한 멤버들

	TArray<FTimerHandle> AuraConditionTimerHandles;
	float AuraConditionCheckInterval = 0.1f;
	void UpdateAuraInfo(UObject* AuraInstigator, ADBDCharacter* EffectOwner, float Distance,
	                                              FGameplayTagContainer RequiredTags,
	                                              FGameplayTagContainer BlockedTags);
	// JMS : ~EnableSurvivorAuraWithDistanceAndTag 구현을 위한 멤버들

	// JMS : ApplyGEWithSurvivorWithinDistance 구현을 위한 멤버들
	float ApplyGEToSelfDistanceCheckInterval = 0.1f;
	TArray<FTimerHandle> DistanceCheckTimerHandles;
	void CheckSurvivorDistanceAndApplyEffect(ADBDCharacter* Target, float Distance, TSubclassOf<UGameplayEffect> GE);
	// JMS : ~ApplyGEWithSurvivorWithinDistance 구현을 위한 멤버들

	// JMS : ApplyGEToSurvivorsWithinDistance 구현을 위한 멤버들
	float ApplyGEToOthersDistanceCheckInterval = 0.1f;
	void CheckDistanceAndRefreshEffectToOthers(ADBDCharacter* EffectOwner, float Distance,
	                                           TSubclassOf<UGameplayEffect> GE);
	// JMS : ~ApplyGEToSurvivorsWithinDistance 구현을 위한 멤버들

	// JMS : 발자국 구현

	FTimerHandle LeaveScratchMarkTimerHandle;
	FTimerDelegate LeaveScratchMarkTimerDelegate;
	float LeaveScratchMarkInterval = 0.7f;
	UFUNCTION()
	void LeaveScratchMarkOnSurvivorSprint(ASurvivorCharacter* Survivor, int32 NewCount);
	void SpawnScratchMarkOnSurvivorLocation(ASurvivorCharacter* Survivor);

	// JMS : ObjectPool을 사용하지 않는 경우
	void DestroySpawnedScratchMark(APoolEntry_ScratchMark* SpawnedScratchMark);
	// JMS : ~발자국 구현
};
