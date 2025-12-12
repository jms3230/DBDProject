#include "JMS/GAS/SurvivorAbilitySystemComponent.h"

#include "JMS/Character/SurvivorCharacter.h"
#include "JMS/DataAsset/DA_SurvivorASCData.h"
#include "JMS/GAS/AbilityPriorityInterface.h"
#include "JMS/GAS/SurvivorAttributeSet.h"
#include "Shared/DBDDebugHelper.h"
#include "Shared/DBDStruct.h"

void USurvivorAbilitySystemComponent::GrantInputAbilities()
{
	if (GetOwner()->HasAuthority())
	{
		for (const FInputAbilityInfo& GAClassPair : SurvivorASCData->
		     InputAbilities)
		{
			GiveAbility(FGameplayAbilitySpec(GAClassPair.AbilityClass, 0, (int32)GAClassPair.AbilityID, nullptr));
		}
	}
}

void USurvivorAbilitySystemComponent::InitializeBaseAttributes()
{
	if (!SurvivorASCData)
	{
		Debug::Print(TEXT("JMS: SurvivorASCData is null"), -1);
		return;
	}
	SetNumericAttributeBase(USurvivorAttributeSet::GetMaxHookHPAttribute(), SurvivorASCData->MaxHookHP);
	SetNumericAttributeBase(USurvivorAttributeSet::GetMaxDyingHPAttribute(), SurvivorASCData->MaxDyingHP);
	SetNumericAttributeBase(USurvivorAttributeSet::GetMaxHealProgressAttribute(), SurvivorASCData->MaxHealProgress);
	SetNumericAttributeBase(USurvivorAttributeSet::GetMovementSpeedAttribute(), SurvivorASCData->MovementSpeed);
	SetNumericAttributeBase(USurvivorAttributeSet::GetSprintSpeedAttribute(), SurvivorASCData->SprintSpeed);
	SetNumericAttributeBase(USurvivorAttributeSet::GetCrouchSpeedAttribute(), SurvivorASCData->CrouchSpeed);
	SetNumericAttributeBase(USurvivorAttributeSet::GetCrawlSpeedAttribute(), SurvivorASCData->CrawlSpeed);
	SetNumericAttributeBase(USurvivorAttributeSet::GetInteractSpeed_GeneratorAttribute(),
	                        SurvivorASCData->InteractSpeed_Generator);
	SetNumericAttributeBase(USurvivorAttributeSet::GetInteractSpeed_ExitDoorAttribute(),
	                        SurvivorASCData->InteractSpeed_ExitDoor);
	SetNumericAttributeBase(USurvivorAttributeSet::GetInteractSpeed_ChestAttribute(),
	                        SurvivorASCData->InteractSpeed_Chest);
	SetNumericAttributeBase(USurvivorAttributeSet::GetInteractSpeed_TotemCleansingAttribute(),
	                        SurvivorASCData->InteractSpeed_TotemCleansing);
	SetNumericAttributeBase(USurvivorAttributeSet::GetHealSpeedAttribute(), SurvivorASCData->HealSpeed);
	SetNumericAttributeBase(USurvivorAttributeSet::GetHookSabotageSpeedAttribute(), SurvivorASCData->HookSabotageSpeed);
	SetNumericAttributeBase(USurvivorAttributeSet::GetRescueSpeedAttribute(), SurvivorASCData->RescueSpeed);
	SetNumericAttributeBase(USurvivorAttributeSet::GetSkillCheckFrequencyAttribute(),
	                        SurvivorASCData->SkillCheckFrequency);
	SetNumericAttributeBase(USurvivorAttributeSet::GetQuickActionSoundVolumeAttribute(),
	                        SurvivorASCData->QuickActionSoundVolume);
	SetNumericAttributeBase(USurvivorAttributeSet::GetCurrentItemChargeAttribute(), SurvivorASCData->CurrentItemCharge);
	SetNumericAttributeBase(USurvivorAttributeSet::GetMaxItemChargeAttribute(), SurvivorASCData->MaxItemCharge);
}