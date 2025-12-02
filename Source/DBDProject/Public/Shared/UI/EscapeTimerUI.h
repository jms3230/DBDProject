// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DBDProgressBarBase.h"
#include "GameplayTagContainer.h"
#include "EscapeTimerUI.generated.h"

struct FGameplayTag;
/**
 * 
 */
UCLASS()
class DBDPROJECT_API UEscapeTimerUI : public UDBDProgressBarBase
{
	GENERATED_BODY()

public:
	UEscapeTimerUI();

	// 생존자가 탈출에 시간이 오래 걸릴경우(생존자가 한명이 누웠다던지, 걸렸다던지 등) 시간초가 느리게 흐르는데 그때 ProgressBar의 이미지들이 바뀜
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* SlowProgressBackground;
	// 생존자가 탈출에 시간이 오래 걸릴경우(생존자가 한명이 누웠다던지, 걸렸다던지 등) 시간초가 느리게 흐르는데 그때 ProgressBar의 이미지들이 바뀜
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTexture2D* SlowProgressFill;

	UPROPERTY(EditAnywhere, meta = (BindWidget))
	UImage* ZeroProgressMarking;

	// 타이머 UI의 이미지를 bIsSlow값에 따라 변경 bIsSlow = 타이머가 느려졌는지 판단 -> CharacterSubsystem에서 호출
	UFUNCTION()
	void ChangeProgressBar(bool bIsSlow);

protected:
	virtual void NativeConstruct() override;

	virtual void Update() override;
};
