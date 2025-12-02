// Fill out your copyright notice in the Description page of Project Settings.


#include "Shared/DBDBlueprintFunctionLibrary.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "RootMotionModifier.h"
#include "Camera/CameraShakeSourceActor.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Kismet/GameplayStatics.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Shared/DBDEnum.h"
#include "Shared/DBDGameplayTags.h"
#include "Shared/Character/DBDCharacter.h"
#include "Shared/GameFramework/DBDPlayerState.h"
#include "Shared/GAS/DBDAbilitySystemComponent.h"
#include "Shared/DBDStruct.h"
#include "MotionWarping/Public/MotionWarpingComponent.h"

UDBDAbilitySystemComponent* UDBDBlueprintFunctionLibrary::NativeGetAbilitySystemComponentFromActor(AActor* Actor)
{
	if (IsValid(Actor))
	{
		return CastChecked<
			UDBDAbilitySystemComponent>(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor));
	}

	UE_LOG(LogTemp, Warning,
	       TEXT("BlueprintFunctionLibrary : NativeGetAbilitySystemComponentFromActor Is Not Valid Actor"));

	return nullptr;
}

void UDBDBlueprintFunctionLibrary::AddUniqueTagToActor(AActor* Actor, FGameplayTag Tag)
{
	if (UDBDAbilitySystemComponent* ASC = NativeGetAbilitySystemComponentFromActor(Actor))
	{
		if (!ASC->HasMatchingGameplayTag(Tag))
		{
			ASC->AddLooseGameplayTag(Tag);
			ASC->AddReplicatedLooseGameplayTag(Tag);
		}
	}
}


void UDBDBlueprintFunctionLibrary::RemoveTagFromActor(AActor* Actor, FGameplayTag Tag)
{
	if (UDBDAbilitySystemComponent* ASC = NativeGetAbilitySystemComponentFromActor(Actor))
	{
		if (ASC->HasMatchingGameplayTag(Tag))
		{
			ASC->RemoveLooseGameplayTag(Tag);
			ASC->RemoveReplicatedLooseGameplayTag(Tag);
		}
	}
}

bool UDBDBlueprintFunctionLibrary::NativeActorHasTag(AActor* Actor, FGameplayTag Tag)
{
	if (UDBDAbilitySystemComponent* ASC = NativeGetAbilitySystemComponentFromActor(Actor))
	{
		return ASC->HasMatchingGameplayTag(Tag);
	}

	return false;
}

void UDBDBlueprintFunctionLibrary::BP_HasTag(AActor* Actor, FGameplayTag Tag, EBaseConfirmType& OutType)
{
	OutType = NativeActorHasTag(Actor, Tag) ? EBaseConfirmType::Yes : EBaseConfirmType::No;
}

void UDBDBlueprintFunctionLibrary::ApplyGameplayEffectToTargetActor(AActor* TargetActor, AActor* SourceActor,
                                                                    TSubclassOf<UGameplayEffect> GameplayEffectClass,
                                                                    int level)
{
	if (UDBDAbilitySystemComponent* SourceASC = NativeGetAbilitySystemComponentFromActor(SourceActor))
	{
		if (UDBDAbilitySystemComponent* TargetASC = NativeGetAbilitySystemComponentFromActor(TargetActor))
		{
			UGameplayEffect* GEInstance = nullptr;

			if (GameplayEffectClass)
			{
				GEInstance = NewObject<UGameplayEffect>(TargetASC, GameplayEffectClass);
			}
			SourceASC->ApplyGameplayEffectToTarget(GEInstance, TargetASC);
			FGameplayEffectSpecHandle GESpec = UAbilitySystemBlueprintLibrary::MakeSpecHandle(
				GEInstance, SourceActor, SourceActor, level);
			SourceASC->ApplyGameplayEffectSpecToTarget(*GESpec.Data.Get(), TargetASC);
		}
	}
}

FName UDBDBlueprintFunctionLibrary::GetTagLastName(const FGameplayTag& Tag)
{
	if (!Tag.IsValid()) return FName();

	FString TagName = Tag.ToString();
	int32 LastIndex;
	if (TagName.FindLastChar('.', LastIndex))
	{
		FString LastTagName = TagName.RightChop(LastIndex + 1);

		return FName(*LastTagName);
	}

	return Tag.GetTagName();
}

FGameplayTag UDBDBlueprintFunctionLibrary::ComputeInteractDirection(const AActor* Object, const AActor* Interactor)
{
	const FVector ObjectForward = Object->GetActorForwardVector();
	const FVector ObjectToInteractorNormalized = (Interactor->GetActorLocation() - Object->GetActorLocation()).
		GetSafeNormal();

	const float DotResult = FVector::DotProduct(ObjectForward, ObjectToInteractorNormalized);
	float OutAngleDifference = UKismetMathLibrary::DegAcos(DotResult);

	const FVector CrossResult = FVector::CrossProduct(ObjectForward, ObjectToInteractorNormalized);

	if (CrossResult.Z < 0.f)
	{
		OutAngleDifference *= -1.f;
	}
	if (OutAngleDifference >= -65.f && OutAngleDifference <= 65.f)
	{
		return DBDGameplayTags::Shared_Direction_Front;
	}
	else if (OutAngleDifference < -65.f && OutAngleDifference >= -115.f)
	{
		return DBDGameplayTags::Shared_Direction_Left;
	}
	else if (OutAngleDifference > 65.f && OutAngleDifference <= 115.f)
	{
		return DBDGameplayTags::Shared_Direction_Right;
	}
	else if (OutAngleDifference < -115.f || OutAngleDifference > 115.f)
	{
		return DBDGameplayTags::Shared_Direction_Back;
	}
	return DBDGameplayTags::Shared_Direction_Default;
}

EPlayerRole UDBDBlueprintFunctionLibrary::GetPlayerRole(const AActor* Actor)
{
	if (const ADBDCharacter* Character = Cast<ADBDCharacter>(Actor))
	{
		if (APlayerController* PC = Cast<APlayerController>(Character->GetController()))
		{
			if (ADBDPlayerState* PS = PC->GetPlayerState<ADBDPlayerState>())
			{
				return PS->GetPlayerRole();
			}
		}
	}
	return EPlayerRole();
}

bool UDBDBlueprintFunctionLibrary::IsSurvivor(const AActor* Actor)
{
	if (GetPlayerRole(Actor) == EPlayerRole::Survivor)
	{
		return true;
	}
	return false;
}

bool UDBDBlueprintFunctionLibrary::IsKiller(const AActor* Actor)
{
	if (GetPlayerRole(Actor) == EPlayerRole::Killer)
	{
		return true;
	}
	return false;
}

void UDBDBlueprintFunctionLibrary::RemoveAllSubTags(FName TagMask, UAbilitySystemComponent* TargetASC)
{
	if (!TargetASC->GetOwner()->HasAuthority())
	{
		return;
	}
	FGameplayTag RootTag = FGameplayTag::RequestGameplayTag(TagMask);
	FGameplayTagContainer TagsToRemove;

	FGameplayTagContainer CurrentTags;
	TargetASC->GetOwnedGameplayTags(CurrentTags);

	for (auto& Tag : CurrentTags)
	{
		if (Tag.MatchesTag(RootTag))
		{
			TagsToRemove.AddTag(Tag);
		}
	}

	TargetASC->RemoveLooseGameplayTags(TagsToRemove);
	TargetASC->RemoveReplicatedLooseGameplayTags(TagsToRemove);
}

FTransform UDBDBlueprintFunctionLibrary::AttachDBDCharacterToMeshSocket(USkeletalMeshComponent* Mesh, FName MeshSocket,
                                                                        ADBDCharacter* Character,
                                                                        FName CharacterSocket)
{
	if (!Character || !Mesh || !Mesh->DoesSocketExist(MeshSocket))
	{
		Debug::Print(TEXT("JMS11 : AttachDBDCharacterToSocket() : Invalid Parameter"), 11);
		return FTransform::Identity;
	}

	Character->K2_DetachFromActor(EDetachmentRule::KeepWorld, EDetachmentRule::KeepWorld, EDetachmentRule::KeepWorld);
	FTransform ResultTransform = MoveDBDCharacterToMeshSocket(Mesh, MeshSocket, Character, CharacterSocket);

	Character->AttachToComponent(Mesh, FAttachmentTransformRules::KeepWorldTransform, MeshSocket);
	return ResultTransform;
}

FTransform UDBDBlueprintFunctionLibrary::MoveDBDCharacterToMeshSocket(USkeletalMeshComponent* Mesh, FName MeshSocket,
                                                                      ADBDCharacter* Character, FName CharacterSocket)
{
	if (!Character || !Mesh || !Mesh->DoesSocketExist(MeshSocket))
	{
		Debug::Print(TEXT("JMS11 : MoveDBDCharacterToMeshSocket() : Invalid Parameter"), 11);
		return FTransform::Identity;
	}
	USkeletalMeshComponent* CharacterMesh = Character->GetMesh();
	FVector MeshSocketLocation = Mesh->GetSocketLocation(MeshSocket);
	FVector CharacterBaseLocation = CharacterMesh->GetSocketLocation(CharacterSocket);

	Character->AddActorWorldOffset(MeshSocketLocation - CharacterBaseLocation);
	return Character->GetTransform();
}

FTransform UDBDBlueprintFunctionLibrary::MoveCharacterWithRotationAdjust(USkeletalMeshComponent* Mesh, FName MeshSocket,
                                                                         ADBDCharacter* Character, FRotator Offset,
                                                                         FName CharacterSocket)
{
	if (!Character || !Mesh || !Mesh->DoesSocketExist(MeshSocket))
	{
		Debug::Print(TEXT("JMS11 : AttachDBDCharacterToSocket() : Invalid Parameter"), 11);
		return FTransform::Identity;
	}
	Character->K2_DetachFromActor(EDetachmentRule::KeepWorld, EDetachmentRule::KeepWorld, EDetachmentRule::KeepWorld);

	FRotator MeshSocketRotation = Mesh->GetSocketRotation(MeshSocket);
	Character->SetActorRotation(FRotator(0, MeshSocketRotation.Yaw, 0) + Offset);

	MoveDBDCharacterToMeshSocket(Mesh, MeshSocket, Character, CharacterSocket);
	return Character->GetTransform();
}

void UDBDBlueprintFunctionLibrary::AddOrUpdateMotionWarpingTarget(
	const TArray<FMotionWarpingInfo>& MotionWarpingTargets, UMotionWarpingComponent* MotionWarpingComponent)
{
	for (FMotionWarpingInfo Target : MotionWarpingTargets)
	{
		MotionWarpingComponent->AddOrUpdateWarpTargetFromComponent(Target.WarpTargetName, Target.Component,
		                                                           Target.BoneName, Target.bFollowComponent,
		                                                           Target.LocationOffset, Target.RotationOffset);
	}
}

void UDBDBlueprintFunctionLibrary::SimpleDetachAndAttachToMesh(AActor* AttachingActor, USkeletalMeshComponent* Mesh,
                                                               FName MeshSocket)
{
	if (!AttachingActor)
	{
		Debug::Print(TEXT("JMS11 : SimpleDetachAndAttachToMesh() : Invalid AttachingActor"), 11);
		return;
	}
	if (!Mesh)
	{
		Debug::Print(TEXT("JMS11 : SimpleDetachAndAttachToMesh() : Invalid Mesh"), 11);
		return;
	}
	if (MeshSocket != NAME_None && !Mesh->DoesSocketExist(MeshSocket))
	{
		Debug::Print(TEXT("JMS11 : SimpleDetachAndAttachToMesh() : Invalid MeshSocket"), 11);
		return;
	}


	AttachingActor->K2_DetachFromActor(EDetachmentRule::KeepWorld, EDetachmentRule::KeepWorld,
	                                   EDetachmentRule::KeepWorld);
	AttachingActor->K2_AttachToComponent(Mesh, MeshSocket, EAttachmentRule::KeepWorld, EAttachmentRule::KeepWorld,
	                                     EAttachmentRule::KeepWorld, false);
}

void UDBDBlueprintFunctionLibrary::AuthPlaySyncAnimMontage(ADBDCharacter* Character1, ADBDCharacter* Character2,
                                                           UAnimMontage* Character1Montage,
                                                           UAnimMontage* Character2Montage, float PlayRate,
                                                           FName Montage1StartSectionName, FName
                                                           Montage2StartSectionName)
{
	if (!(Character1 && Character2 && Character1Montage && Character2Montage))
	{
		return;
	}
	if (Character1->HasAuthority())
	{
		Character1->PlaySyncMontageFromServer(Character1Montage, PlayRate, Montage1StartSectionName);
		Character2->PlaySyncMontageFromServer(Character2Montage, PlayRate, Montage2StartSectionName);
	}
}

void UDBDBlueprintFunctionLibrary::AttachDBDCharacterToComponent(ADBDCharacter* Character, USceneComponent* Parent,
                                                                 FName ParentSocketName, FName OptionalChildSocketname)
{
	if (!Character || !Parent)
	{
		return;
	}
	Character->GetMesh()->K2_AttachToComponent(Parent, ParentSocketName, EAttachmentRule::SnapToTarget,
	                                           EAttachmentRule::SnapToTarget,
	                                           EAttachmentRule::SnapToTarget, false);
	if (OptionalChildSocketname != NAME_None)
	{
		FVector ChildSocketLocation = Character->GetMesh()->GetSocketLocation(OptionalChildSocketname);
		FVector ChildRootLocation = Character->GetMesh()->GetComponentLocation();
		FHitResult HitResult;
		Character->GetMesh()->K2_AddRelativeLocation(ChildSocketLocation - ChildRootLocation, false, HitResult, false);
	}
}

void UDBDBlueprintFunctionLibrary::DetachDBDCharacterFromComponent(ADBDCharacter* Character)
{
	if (Character)
	{
		Character->GetMesh()->K2_DetachFromComponent();
		Character->GetMesh()->K2_AttachToComponent(Cast<USceneComponent>(Character->GetCapsuleComponent()),
		                                           FName(TEXT("NAME_None")), EAttachmentRule::SnapToTarget,
		                                           EAttachmentRule::SnapToTarget, EAttachmentRule::SnapToTarget,
		                                           false);
	}
}

void UDBDBlueprintFunctionLibrary::SetDBDCharacterLocationOnSocket(ADBDCharacter* Character, USceneComponent* Target,
                                                                   FName SocketName)
{
	if (Character && Target)
	{
		Character->SetActorLocation(Target->GetSocketLocation(SocketName) + FVector(0, 0, 90.f));
	}
}
