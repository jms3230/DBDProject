// Fill out your copyright notice in the Description page of Project Settings.


#include "JMS/Component/SkillCheckComponent.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "IMediaControls.h"
#include "Blueprint/UserWidget.h"
#include "Components/TimelineComponent.h"
#include "JMS/UI/SkillCheckModalWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Shared/DBDDebugHelper.h"
#include "Sound/SoundCue.h"


USkillCheckComponent::USkillCheckComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	bIsSkillCheckDone = true;
}

void USkillCheckComponent::BeginPlay()
{
	Super::BeginPlay();
	APlayerController* OwningPlayerController = Cast<APlayerController>(GetOwningController());
	if (OwningPlayerController && OwningPlayerController->IsLocalPlayerController())
	{
		SkillCheckWidget = CreateWidget<USkillCheckModalWidget>(OwningPlayerController, SkillCheckWidgetClass);
		if (SkillCheckWidget)
		{
			SkillCheckWidget->AddToViewport();
			SkillCheckWidget->SetVisibility(ESlateVisibility::Hidden);
		}
	}
	// SkillCheckTimeline = NewObject<UTimelineComponent>(this, FName("SkillCheckTimeline"));
	// SkillCheckTimeline->RegisterComponent();
	//
	// TimelineUpdateDelegate.BindUFunction(this, FName("TimelineUpdate"));
	// SkillCheckTimeline->AddInterpFloat(SkillCheckTimelineCurve, TimelineUpdateDelegate);
	//
	// TimelineFinishedDelegate.BindUFunction(this, FName("TimelineFinished"));
	// SkillCheckTimeline->SetTimelineFinishedFunc(TimelineFinishedDelegate);
	SetComponentTickEnabled(false);
}

void USkillCheckComponent::TickComponent(float DeltaTime, enum ELevelTick TickType,
                                         FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	CurrentTime += DeltaTime;
	if (SkillCheckWidget && CachedDuration > 0)
	{
		SkillCheckWidget->SkillCheckNeedleUpdate(CurrentTime / CachedDuration);
	}
	if (CurrentTime > CachedDuration)
	{
		OnEndSkillCheck();
	}
}


void USkillCheckComponent::TriggerOneShotSkillCheck(float Duration, float GoodWindowStart, float GoodWindowLength,
                                                    float GreatWindowLength)
{
	SetComponentTickEnabled(true);
	bIsSkillCheckDone = false;
	UGameplayStatics::PlaySound2D(GetWorld(), SkillCheckStartSound);
	CurrentTime = 0;
	CachedDuration = Duration;
	CachedGoodWindowStart = GoodWindowStart;
	CachedGoodWindowLength = GoodWindowLength;
	CachedGreatWindowLength = GreatWindowLength;
	APlayerController* PC = Cast<APlayerController>(GetOwningController());
	if (PC && PC->IsLocalPlayerController())
	{
		UEnhancedInputLocalPlayerSubsystem* InputSubsystem = PC->GetLocalPlayer()->GetSubsystem<
			UEnhancedInputLocalPlayerSubsystem>();
		if (InputSubsystem)
		{
			InputSubsystem->AddMappingContext(SkillCheckIMC, SkillCheckIMCPriority);
		}
	}
	if (Duration < 0)
	{
		return;
	}
	if (SkillCheckWidget && SkillCheckWidget->GetVisibility() == ESlateVisibility::Hidden)
	{
		SkillCheckWidget->Activate(Duration, GoodWindowLength, GoodWindowStart, GreatWindowLength);
	}
}

void USkillCheckComponent::CancelSkillCheck()
{
	APlayerController* PC = Cast<APlayerController>(GetOwningController());
	if (PC && PC->IsLocalPlayerController())
	{
		UEnhancedInputLocalPlayerSubsystem* InputSubsystem = PC->GetLocalPlayer()->GetSubsystem<
			UEnhancedInputLocalPlayerSubsystem>();
		if (InputSubsystem)
		{
			InputSubsystem->RemoveMappingContext(SkillCheckIMC);
		}
	}
	SetComponentTickEnabled(false);
	if (SkillCheckWidget)
	{
		SkillCheckWidget->Deactivate();
	}
}

void USkillCheckComponent::OnEndSkillCheck()
{
	bIsSkillCheckDone = true;

	if (CurrentTime > CachedGoodWindowStart && CurrentTime < CachedGoodWindowStart +
		CachedGoodWindowLength)
	{
		if (CurrentTime < CachedGoodWindowStart + CachedGreatWindowLength)
		{
			CachedResult = ESkillCheckResult::Great;
			UGameplayStatics::PlaySound2D(GetWorld(), SkillCheckGreatSound);
		}
		else
		{
			CachedResult = ESkillCheckResult::Good;
			UGameplayStatics::PlaySound2D(GetWorld(), SkillCheckGoodSound);
		}
	}
	else
	{
		CachedResult = ESkillCheckResult::Bad;
	}
	SkillCheckEndDelegate.Broadcast(CachedResult);
	APlayerController* PC = Cast<APlayerController>(GetOwningController());
	if (PC && PC->IsLocalPlayerController())
	{
		UEnhancedInputLocalPlayerSubsystem* InputSubsystem = PC->GetLocalPlayer()->GetSubsystem<
			UEnhancedInputLocalPlayerSubsystem>();
		if (InputSubsystem)
		{
			InputSubsystem->RemoveMappingContext(SkillCheckIMC);
		}
	}
	SetComponentTickEnabled(false);
	if (SkillCheckWidget)
	{
		SkillCheckWidget->Deactivate();
	}
}

void USkillCheckComponent::SetupInputActionBinding(UEnhancedInputComponent* EnhancedInputComponent)
{
	if (EnhancedInputComponent && SkillCheckInputAction)
	{
		EnhancedInputComponent->BindAction(SkillCheckInputAction, ETriggerEvent::Triggered, this,
		                                   &USkillCheckComponent::SkillCheckConfirm);
	}
}

bool USkillCheckComponent::IsSkillCheckStillPlaying()
{
	return !bIsSkillCheckDone;
}

void USkillCheckComponent::SkillCheckConfirm(const FInputActionValue& InputActionValue)
{
	bool bConfirm = InputActionValue.Get<bool>();
	if (bConfirm)
	{
		OnEndSkillCheck();
	}
}
