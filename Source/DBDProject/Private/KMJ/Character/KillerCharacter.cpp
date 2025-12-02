// Fill out your copyright notice in the Description page of Project Settings.

#include "KMJ/Character/KillerCharacter.h"

#include "EngineUtils.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Camera/CameraComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameSession.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/SpringArmComponent.h"
#include "JMS/Character/SurvivorCharacter.h"
#include "Kismet/KismetMathLibrary.h"
#include "KMJ/AbilitySystem/KillerAbilitySystemComponent.h"
#include "KMJ/AbilitySystem/KillerAttributeSet.h"
#include "KMJ/AnimInstances/KillerAnimInstance.h"
#include "KMJ/Component/KillerInteractableComponent.h"
#include "KMJ/DataAsset/DA_KillerInput.h"
#include "MMJ/Object/Interactable/Obj_Generator.h"
#include "MMJ/Object/Interactable/Obj_Hook.h"
#include "Net/UnrealNetwork.h"
#include "Shared/DBDBlueprintFunctionLibrary.h"
#include "Shared/DBDDebugHelper.h"
#include "Shared/Component/InteractorComponent.h"
#include "Shared/Controller/DBDPlayerController.h"
#include "Shared/GameFramework/DBDPlayerState.h"
#include "Shared/Subsystem/DBDObjectAuraSubsystem.h"
#include "Shared/Subsystem/DBDCharacterSubsystem.h"
#include "Shared/Subsystem/DBDObjectObserver.h"
#include "Slate/SGameLayerManager.h"

AKillerCharacter::AKillerCharacter()
{
	CameraArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraArm"));
	//CameraArm->SetupAttachment(GetRootComponent());
	CameraArm->SetupAttachment(GetMesh(), TEXT("joint_Head_01"));
	FollowCam = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCam"));
	//FollowCam->SetupAttachment(GetRootComponent());
	FollowCam->SetupAttachment(CameraArm);
	//FollowCam->SetupAttachment(GetMesh(), TEXT("joint_Head_01"));
	FollowCam->bUsePawnControlRotation = true;
	
	//메시 -90�려�아 �면�로 조정
	//BodyMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("BodyMesh"));
	GetMesh()->SetupAttachment(GetRootComponent());

	KillerAbilitySystemComponent = CreateDefaultSubobject<UKillerAbilitySystemComponent>(TEXT("KillerAbilitySystemComponent"));
	KillerAttributeSet = CreateDefaultSubobject<UKillerAttributeSet>(TEXT("KillerAttributeSet"));

	// InteractableComponent
	KillerInteractableComponent = CreateDefaultSubobject<UKillerInteractableComponent>(
		TEXT("KillerInteractableComponent"));
	KillerInteractableComponent->SetupAttachment(GetRootComponent());
	bReplicates = true;
}

UAbilitySystemComponent* AKillerCharacter::GetAbilitySystemComponent() const
{
	return KillerAbilitySystemComponent;
}

UInteractableComponent* AKillerCharacter::GetInteractableComponent() const
{
	return KillerInteractableComponent;
}


EPlayerRole AKillerCharacter::GetInteractorRole() const
{
	return EPlayerRole::Killer;
}

void AKillerCharacter::ServerSideInit()
{
	KillerAbilitySystemComponent->InitAbilityActorInfo(this, this);
	KillerAbilitySystemComponent->ApplyInitializeEffects();
	KillerAbilitySystemComponent->OperatingInitializedAbilities();
	ADBDPlayerState* PS = Cast<ADBDPlayerState>(GetPlayerState());
	if (PS)
	{
		AuthInitPerks();
	}
	if (!IsRunningDedicatedServer())
	{
		UKillerAnimInstance* KillerAnimInstance = Cast<UKillerAnimInstance>(GetMesh()->GetAnimInstance());
		if (KillerAnimInstance)
		{
			//UE_LOG(LogTemp, Error, TEXT("KillerCharacter::ServerSideInit: KillerAnimInstance Is %s"), *KillerAnimInstance->GetName());
			KillerAnimInstance->InitializeWithAbilitySystem(KillerAbilitySystemComponent);
		}
		else
		{
		    //UE_LOG(LogTemp, Error, TEXT("KillerCharacter::ServerSideInit: KillerAnimInstance Is Null"));
		}
	}
	// JMS: ��버� �버/�라�언각각 관리해줘야 �서 �기추�습�다.
	UDBDCharacterSubsystem* CharacterSubsystem = GetWorld()->GetSubsystem<UDBDCharacterSubsystem>();
	if (!CharacterSubsystem)
	{
		//Debug::Print(TEXT("JMS11: CharacterSubsystem is NULL!"), 11);
	}
	else
	{
		CharacterSubsystem->RegisterKiller(this);
	}
}

void AKillerCharacter::ClientSideInit()
{
	KillerAbilitySystemComponent->InitAbilityActorInfo(this, this);
	UKillerAnimInstance* KillerAnimInstance = Cast<UKillerAnimInstance>(GetMesh()->GetAnimInstance());
	if (KillerAnimInstance)
	{
		//UE_LOG(LogTemp, Error, TEXT("KillerCharacter::ClientSideInit: KillerAnimInstance Is %s"), *KillerAnimInstance->GetName());
		KillerAnimInstance->InitializeWithAbilitySystem(KillerAbilitySystemComponent);
	}
	else
	{
		//UE_LOG(LogTemp, Error, TEXT("KillerCharacter::ClientSideInit: KillerAnimInstance Is Null"));
	}
	// JMS: ��버� �버/�라�언각각 관리해줘야 �서 �기추�습�다.
	UDBDCharacterSubsystem* CharacterSubsystem = GetWorld()->GetSubsystem<UDBDCharacterSubsystem>();
	if (!CharacterSubsystem)
	{
		//Debug::Print(TEXT("JMS11: CharacterSubsystem is NULL!"), 11);
	}
	else
	{
		CharacterSubsystem->RegisterKiller(this);
		CharacterSubsystem->PrintAllCharacter();
	}
}

void AKillerCharacter::BeginPlay()
{
	Super::BeginPlay();
	if (GetController())
	{
		if (FPVAnimClass)
		{
			GetMesh()->SetAnimInstanceClass(FPVAnimClass);
			//Debug::Print(FString::Printf(TEXT("KMJ:: %s: FPV: %s"), *GetController()->GetCharacter()->GetName() ,*FPVAnimClass->GetName()), 3);
			//UE_LOG(LogTemp, Warning, TEXT("FPV: %s"), *FPVAnimClass->GetName());
		}
		//else UE_LOG(LogTemp, Warning, TEXT("FPVAnimClass is null!"));
	}
	else
	{
		if (TPVAnimClass)
		{
			GetMesh()->SetAnimInstanceClass(TPVAnimClass);
			//Debug::Print(FString::Printf(TEXT("KMJ:: %s: TPV: %s"), *GetController()->GetCharacter()->GetName(),*TPVAnimClass->GetName()), 3);
			//UE_LOG(LogTemp, Warning, TEXT("TPV: %s"), *TPVAnimClass->GetName());
		}
		//else UE_LOG(LogTemp, Warning, TEXT("TPVAnimClass is null!"));
	}
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);


	ADBDPlayerController* PC = Cast<ADBDPlayerController>(GetController());
	if (PC)
	{
		DisableInput(PC);
		if (ADBDPlayerState* PS = PC->GetPlayerState<ADBDPlayerState>())
		{
			if (PS->GetPlayerEndState() != EPlayerEndState::None)
			{
				DisableInput(PC);
				UE_LOG(LogTemp, Warning, TEXT("MMJLog : EndLevel"));
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("MMJLog : InGameLevel"));
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("MMJLog : Not Valid PS"));
		}
	}
	//YHG �크 �우코드
	//GetAbilitySystemComponent()->RegisterGameplayTagEvent(DBDGameplayTags::Killer_Common_Status_Carrying, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &ThisClass::OnAura_Hook);
}

void AKillerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (GetCharacterMovement()->IsFalling())
	{
		GetCharacterMovement()->RotationRate = FRotator(0.0f, 180.0f, 0.0f);
	}
	else
	{
		GetCharacterMovement()->RotationRate = FRotator(0.0f, 360.0f, 0.0f);
	}

	//�태 �의 코드
	bIsGrounded = GetCharacterMovement()->IsMovingOnGround();
	bIsFalling = GetCharacterMovement()->IsFalling();
	//�무�직임�고, 지�일 	bIsIdle = GetCharacterMovement()->Velocity.IsZero() && bIsGrounded;

	//카메기� �른쪽으례직이��, �쪽�로 �직이��
	FVector VelocityDirection = GetCharacterMovement()->Velocity.GetSafeNormal();
	bIsLeftMoving = UKismetMathLibrary::Cross_VectorVector(VelocityDirection, FollowCam->GetForwardVector()).Z > 0;
	bIsRightMoving = UKismetMathLibrary::Cross_VectorVector(VelocityDirection, FollowCam->GetForwardVector()).Z < 0;

	if ((bIsLeftMoving && bIsRightMoving) || (!bIsLeftMoving && !bIsRightMoving))
	{
		if (FMath::RandBool())
		{
			bIsLeftMoving = true;
			bIsRightMoving = false;
		}
		else
		{
			bIsLeftMoving = false;
			bIsRightMoving = true;
		}
	}
}

void AKillerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	if (NewController)
	{
		ServerSideInit();
	}

}

void AKillerCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	//if (IsLocallyControlledByPlayer())
	//{
		ClientSideInit();
	//}
	// JMS: 발전긤라륄해 추�습�다.
	if (IsLocallyControlledByPlayer())
	{
		UDBDObjectObserver* ObjectObserver = GetWorld()->GetSubsystem<UDBDObjectObserver>();
		TArray<AObj_Generator*> Generators = ObjectObserver->GetGenerators();
		for (AObj_Generator* Generator : Generators)
		{
			//Generator->SetCustomDepth(1);
			//Generator->EnableAura();
		}
	}
}

bool AKillerCharacter::IsLocallyControlledByPlayer() const
{
	// 로컬�어 & �레�어 컨트롤러�� �단
	return GetController() && GetController()->IsLocalPlayerController();
}

void AKillerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (EnhancedInputComponent)
	{
		EnhancedInputComponent->BindAction(InputData->IA_Look, ETriggerEvent::Triggered, this,
										   &AKillerCharacter::LookAction);
		EnhancedInputComponent->BindAction(InputData->IA_Move, ETriggerEvent::Triggered, this,
										   &AKillerCharacter::MoveAction);

		for (const TPair<EKillerAbilityInputID, UInputAction*>& InputActionPair : InputData->
			 GameplayAbilityInputActions)
		{
			EnhancedInputComponent->BindAction(InputActionPair.Value, ETriggerEvent::Triggered, this,
											   &AKillerCharacter::AbilityInput, InputActionPair.Key);
		}
	}
}

void AKillerCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();
	APlayerController* PC = GetController<APlayerController>();
	if (PC)
	{
		UEnhancedInputLocalPlayerSubsystem* InputSubsystem = PC->GetLocalPlayer()->GetSubsystem<
			UEnhancedInputLocalPlayerSubsystem>();
		if (InputSubsystem)
		{
			InputSubsystem->AddMappingContext(InputData->InputMappingContext,
											  InputData->InputMappingContextPriority);
		}
	}
}

void AKillerCharacter::InitItemHUD()
{
	// JMS: UI수정: WidgetComponent에서 스폰 시 초기화
	// OnItemUIInitialize.Broadcast(this, GetInteractorRole());
}

/*void AKillerCharacter::AddControllerPitchInput(float Val)
{
	if (Controller && Val != 0.0f)
	{
		FRotator NewRotation = Controller->GetControlRotation();
		//float NewPitch; //= FMath::Clamp(NewRotation.Pitch + Val, -120.0f, 120.0f); // Pitch �한
		if (NewRotation.Pitch + Val > 120.0f) NewRotation.Pitch = 120.0f;
		else if (NewRotation.Pitch + Val < -120.0f) NewRotation.Pitch = -120.0f;
		else NewRotation.Pitch =  NewRotation.Pitch + Val;
		//NewRotation.Pitch = NewPitch;
		Controller->SetControlRotation(NewRotation);
	}
}*/

void AKillerCharacter::OnRep_CarriedSurvivorCharacter()
{
}

void AKillerCharacter::Server_SetCarriedSurvivorCharacter_Implementation(ASurvivorCharacter* NewSurvivorCharacter)
{
	// �효�존캐릭�인지 �인
	if (IsValid(NewSurvivorCharacter))
	{
		CarriedSurvivorCharacter = NewSurvivorCharacter;

		// �플리�션�리거하�라�언�에 �기		OnRep_CarriedSurvivorCharacter();  // �라�언측에처리�용추� 가
		// 추�인 �버�로직 처리 가( �력 �스
		//UE_LOG(LogTemp, Log, TEXT("Carried Survivor Character set to: %s"), *NewSurvivorCharacter->GetName());
	}
	else
	{
		//UE_LOG(LogTemp, Warning, TEXT("Invalid Survivor Character passed to server!"));
	}
}

bool AKillerCharacter::Server_SetCarriedSurvivorCharacter_Validate(ASurvivorCharacter* NewSurvivorCharacter)
{
	// �기�절검�로직추�니 �� �어:
	if (IsValid(NewSurvivorCharacter))
	{
		// �존�� �효�고 �아�는지 �인
		return true;
	}
	else
	{
		//UE_LOG(LogTemp, Warning, TEXT("Invalid Survivor Character passed to server during validation!"));
		return false; // �라�언�� 보낸 �이�� �효�� �으�false 반환
	}
}

void AKillerCharacter::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AKillerCharacter, CarriedSurvivorCharacter);
	//DOREPLIFETIME(AKillerCharacter, KillerAim_Horizontal);
	DOREPLIFETIME_CONDITION(AKillerCharacter, KillerAim_Horizontal, COND_SkipOwner)
	DOREPLIFETIME(AKillerCharacter, IsCarrying);
}


void AKillerCharacter::OnRep_KillerAimHorizontal()
{
}


void AKillerCharacter::Server_SetKillerAimHorizontal_Implementation(float NewPitch)
{
	KillerAim_Horizontal = NewPitch;
}

bool AKillerCharacter::Server_SetKillerAimHorizontal_Validate(float NewPitch)
{
	return true;
}

void AKillerCharacter::Server_SetKiller_IsCarrying_Implementation(bool bIsCarrying)
{
	IsCarrying = bIsCarrying;
}

bool AKillerCharacter::Server_SetKiller_IsCarrying_Validate(bool bIsCarrying)
{
	return true;
}

void AKillerCharacter::AbilityInput(const FInputActionValue& InputActionValue, EKillerAbilityInputID InputID)
{
	const bool bPressed = InputActionValue.Get<bool>();
	if (bPressed)
	{
		GetAbilitySystemComponent()->AbilityLocalInputPressed(static_cast<int32>(InputID));
	}
	else
	{
		GetAbilitySystemComponent()->AbilityLocalInputReleased(static_cast<int32>(InputID));
	}
}

void AKillerCharacter::LookAction(const FInputActionValue& InputActionValue)
{
	FVector2D InputValue = InputActionValue.Get<FVector2D>();

	AddControllerPitchInput(-InputValue.Y);
	AddControllerYawInput(InputValue.X);
	float NewPitch = GetControlRotation().Pitch;
	Server_SetKillerAimHorizontal(NewPitch);
}

void AKillerCharacter::MoveAction(const FInputActionValue& InputActionValue)
{
	FVector2D InputValue = InputActionValue.Get<FVector2D>();

	AddMovementInput(GetMoveForwardDirection() * InputValue.Y + GetLookRightDirection() * InputValue.X);
}

FVector AKillerCharacter::GetLookRightDirection() const
{
	return FollowCam->GetRightVector();
}

FVector AKillerCharacter::GetLookForwardDirection() const
{
	return FollowCam->GetForwardVector();
}

FVector AKillerCharacter::GetMoveForwardDirection() const
{
	return FVector::CrossProduct(GetLookRightDirection(), FVector::UpVector);
}

bool AKillerCharacter::GetIsGrounded() const
{
	return bIsGrounded;
}

bool AKillerCharacter::GetIsFalling() const
{
	return bIsFalling;
}

bool AKillerCharacter::GetIsIdle() const
{
	return bIsIdle;
}

bool AKillerCharacter::GetIsRightMoving() const
{
	return bIsLeftMoving;
}

bool AKillerCharacter::GetIsLeftMoving() const
{
	return bIsRightMoving;
}

//�크 �우�성비활�화
void AKillerCharacter::OnAura_Hook(const FGameplayTag Tag, const int32 NewCount) const
{
	UDBDObjectObserver* ObjectObserver = GetWorld()->GetSubsystem<UDBDObjectObserver>();
	if (!ObjectObserver)
	{
		return;
	}

	ADBDPlayerState* DBDPlayerState = Cast<ADBDPlayerState>(GetPlayerState());
	if (!DBDPlayerState)
	{
		return;
	}

	UDBDObjectAuraSubsystem* AuraSubsystem = GetWorld()->GetSubsystem<UDBDObjectAuraSubsystem>();
	if (!AuraSubsystem)
	{
		return;
	}
	
	if (NewCount > 0)
	{
		for (AObj_Hook* Hook : ObjectObserver->GetHooks())
		{
			//AuraSubsystem->SetAuraState(Hook, GetPlayerState(), 1);
		}
		// if (IsLocallyControlled())
		// {
		// 	for (AObj_Hook* Hook : ObjectObserver->GetHooks())
		// 	{
		// 		Hook->EnableAura();	
		// 	}
		// }
	}
	else
	{
		for (AObj_Hook* Hook : ObjectObserver->GetHooks())
		{
			//AuraSubsystem->UnSetAuraState(Hook, GetPlayerState());
		}
		// if (IsLocallyControlled())
		// {
		// 	for (AObj_Hook* Hook : ObjectObserver->GetHooks())
		// 	{
		// 		Hook->DisableAura();	
		// 	}
		// }
	}
}