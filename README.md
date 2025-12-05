# DBDProject (Dead by Daylight 모작)

[![Video Label](http://img.youtube.com/vi/ro7gwYu5df8/0.jpg)](https://youtu.be/ro7gwYu5df8)

## 📝 프로젝트 소개
비대칭 멀티플레이어 서바이벌 호러 게임 **Dead by Daylight**를 모작한 프로젝트입니다.  
언리얼 엔진 5와 C++로 개발되었으며, **Dedicated Server**로 구현되었습니다. **Gameplay Ability System (GAS)**을 활용하여 캐릭터의 능력, 상태, 상호작용을 객체지향적으로 구현했습니다.

## 👥 팀원 및 역할
| 이름 | 역할 | 담당 파트 |
|:---:|:---:|:---|
| **정민수 (팀장)** | **Survivor** | **생존자 캐릭터, 상호작용 시스템, 퍽 시스템, 캐릭터 오라 및 발자국 시스템** |
| 모명준 | Object | 로비, 게임 플로우, 오브젝트, 상호작용 시스템, 오브젝트 오라 시스템 |
| 김미진 | Killer | 살인마 캐릭터, UI 베이스 설계 |
| 유호근 | Map | 맵 랜덤 생성 시스템, 라이팅 |

## 📅 제작 기간
*   **2025.09.01 - 2025.11.06**

## 🛠 기술 스택
### Development
<div>
<img src="https://img.shields.io/badge/Unreal Engine 5-0E1128?style=flat-square&logo=unrealengine&logoColor=white"/>
<img src="https://img.shields.io/badge/C++-00599C?style=flat-square&logo=c%2B%2B&logoColor=white"/>
<img src="https://img.shields.io/badge/Gameplay Ability System-FE5F55?style=flat-square&logo=unrealengine&logoColor=white"/>
<img src="https://img.shields.io/badge/Dedicated Server-00599C?style=flat-square&logo=unrealengine&logoColor=white"/>
</div>

### Tools & Communication
<div>
<img src="https://img.shields.io/badge/Notion-000000?style=flat-square&logo=notion&logoColor=white"/>
<img src="https://img.shields.io/badge/Discord-5865F2?style=flat-square&logo=discord&logoColor=white"/>
<img src="https://img.shields.io/badge/Perforce-4C00FF?style=flat-square&logo=perforce&logoColor=white"/>
</div>

## 💡 주요 기능 (Key Features)
### 🏃‍♂️ 캐릭터 (생존자)
*   **캐릭터 초기화 로직 (Character Creation)**
    *   **캐릭터 스폰 및 초기화 과정 시퀀스 다이어그램**
    <details>
    <summary>캐릭터 스폰 및 초기화 과정 시퀀스 다이어그램</summary>
    
    ```mermaid
    sequenceDiagram
    participant ADBDGameMode
    participant ASurvivorCharacter
    ADBDGameMode->>ASurvivorCharacter: HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) 호출
    ADBDGameMode->>ADBDGameMode: SpawnSurvivorCharacter(PS, SurvivorSpawnTransform) 호출
    ADBDGameMode->>ADBDGameMode: NewPlayer->Possess 호출
    activate ASurvivorCharacter
    note over ASurvivorCharacter: PossessedBy(NewController)
    ASurvivorCharacter->>+ASurvivorCharacter: ServerSideInit() 호출
    note over ASurvivorCharacter: ServerSideInit()
    ASurvivorCharacter->>ASurvivorCharacter: GAS 초기화, AnimInstance와 GAS 연결
    alt 아이템 설정됨 (InitialItemInfo 존재)
        ASurvivorCharacter->>ASurvivorCharacter: InitializeEquippedItem(InitialItemInfo) 호출
    else 아이템 설정 안됨
        ASurvivorCharacter->>ASurvivorCharacter: 초기화 스킵
    end
    ASurvivorCharacter->>ASurvivorCharacter: 서브시스템에 생존자 등록
    deactivate ASurvivorCharacter
    ```
    * **퍽 초기화 과정 시퀀스 다이어그램(생존자, 살인마 공통)**
    ```mermaid
    sequenceDiagram
    participant ADBDCharacterServer as ADBDCharacter (Server)
    participant UPerkComponent
    participant ADBDCharacterClient as ADBDCharacter (Client)
    
    note over ADBDCharacterServer: AuthInitPerks()
    
    ADBDCharacterServer->>ADBDCharacterServer: Killer/Survivor 판별 후 알맞는 데이터 테이블 조회
    
    loop 퍽 4종
        ADBDCharacterServer->>ADBDCharacterServer: InitializePerks(DataTable, 퍽 4개 Datatable 키) 호출
        ADBDCharacterServer->>UPerkComponent: 스폰 & RegisterComponent() 호출
        ADBDCharacterServer->>UPerkComponent: OnServerSideInitilaized() 호출
    end
    
    note over ADBDCharacterServer: BeginPlay()
    
    ADBDCharacterServer->>ADBDCharacterServer: 5초 후에 InitPerkOnClient() 호출
   
    ADBDCharacterServer->>ADBDCharacterClient: Client_UpdatePerk(UPerkComponent* 퍽 4개) 호출
    
    loop 퍽 4종
        ADBDCharacterClient->>UPerkComponent: OnOwnerClientSideInitilaized() 호출
        ADBDCharacterClient->>UPerkComponent: RegisterComponent() 호출
    end

    ```
    * **아이템 초기화 과정 시퀀스 다이어그램**
    ```mermaid
    sequenceDiagram
    participant ASurvivorCharacter
    participant ASurvivorItem
    participant UAddonComponent
    participant USurvivorAttributeSet
    
    activate ASurvivorCharacter
    note over ASurvivorCharacter: InitializeEquippedItem()
    
    ASurvivorCharacter->>ASurvivorItem: 스폰(DB에서 키 값 조회)
    activate ASurvivorItem
    loop 애드온 1,2
        note over ASurvivorCharacter: 애드온 초기화
        Activate UAddonComponent
        ASurvivorCharacter->>UAddonComponent: 스폰 & Attach (UAddonComponent)
        UAddonComponent->>UAddonComponent: OnInitialized() 호출
    end
    
    ASurvivorCharacter->>ASurvivorCharacter: EquipItem(스폰한 아이템)
    
    ASurvivorItem->>ASurvivorItem: AttachToCharacter
    ASurvivorItem->>+ASurvivorItem: OnEquipItem() 호출
    
    note over ASurvivorItem: 아이템 장착
    ASurvivorItem->>USurvivorAttributeSet: 아이템 내구도 어트리뷰트 초기화
    ASurvivorItem->>UAddonComponent: OnEquip() 호출
    ASurvivorItem->>ASurvivorItem: 아이템 어빌리티 부여
    
    ASurvivorItem->>-ASurvivorItem: OnInitialized() 호출
    
    deactivate ASurvivorItem
    deactivate ASurvivorCharacter
    ```
*   **상태 관리 (State Management)**
    *   건강함(Normal) -> 부상(Injured) -> 빈사(Dying) -> 갈고리(Hooked) -> 사망(Dead)으로 이어지는 생명 주기 구현.
    *   GameplayTag를 사용하여 상태 체크 및 이벤트 처리.
    *   각 상태에 따른 이동 속도 변화 및 애니메이션 처리.
*   **Gameplay Ability System (GAS)**
    *   **AttribteSet**: 체력, 이동 속도, 수리/치료 속도 등 캐릭터 스탯 관리.
    *   **GameplayAbility**: 달리기(Sprint), 치료(Heal), 수리(Repair) 등 액션의 모듈화.
    *   **GameplayEffect**: 아이템/퍽 효과 적용 및 상태 이상(Buff/Debuff) 처리.
    *   **GameplayAbility 클래스 다이어그램**
    ```mermaid
    classDiagram

    class USurvivorInteractionAbility {
        +GetInteractorComponentFromActorInfo(): UInteractorComponent#42;
        +InteractStart(): void
        +SetRandomSkillCheckEnabledOnClient(): void
        +Server_SendSkillCheckResult(): void*
        #GetCachedCurrentInteractable~T~(): T#42;
    }
    USurvivorGameplayAbility <|-- USurvivorInteractionAbility

    USurvivorGameplayAbility <|-- USurvivorPassiveAbility

    USurvivorGameplayAbility <|-- USurvivorUseItemAbility

    class UItemGameplayAbility_Charge {
        +ChargeConsumptionPerSecond: float
        #ChargeConsumptionEffect: UGameplayEffect
        #OnCurrentItemChargeChanged(): void
    }
    USurvivorInteractionAbility <|-- UItemGameplayAbility_Charge

    class USurvivorInteractionAbility자식클래스{
        UGA_SelfCare
        UGA_Survivor_RepairGenerator
        UGA_Survivor_Rescue
        UGA_Survivor_HealOther
        UGA_Survivor_OpenCabinet
        UGA_Toolbox_Sabotage
    }
    USurvivorInteractionAbility <|-- USurvivorInteractionAbility자식클래스

    class UItemGameplayAbility_Charge자식클래스{
        UGA_MedKit_HealSelf
        UGA_MedKit_HealOther
        UGA_Toolbox_Repair
        UGA_Toolbox_Sabotage
    }
    UItemGameplayAbility_Charge <|-- UItemGameplayAbility_Charge자식클래스

    USurvivorUseItemAbility <|-- UGA_UseFireCracker

    class USurvivorGameplayAbility자식클래스{
        UGA_Survivor_Crouch
        UGA_Survivor_Drop
        UGA_Survivor_Sprint
    }
    USurvivorGameplayAbility <|-- USurvivorGameplayAbility자식클래스

    class USurvivorPassiveAbility자식클래스{
        UGA_Survivor_Dying
        UGA_Survivor_HookedIn
        UGA_Survivor_CapturedByKiller
    }
    USurvivorPassiveAbility <|-- USurvivorPassiveAbility자식클래스
    ```
*   **상호작용 컴포넌트 (InteractorComponent)**
    *   `InteractableComponent`와 `InteractorComponent`로 이루어진 독립적인 상호작용 모듈 구현.
    *   커스텀 충돌 채널을 활용.
    *   발전기 수리, 동료 치료, 갈고리 파괴 등 다양한 상호작용 구현
    *   **InteractorComponent 동작 시퀀스 다이어그램**
    ```mermaid
    sequenceDiagram
    participant SC as UInteractorComponent(Client)
    participant SS as UInteractorComponent(Server)
    participant IC as UInteractableComponent(Server)
    participant GAS as USurvivorInteractionAbility(Server)
    activate SS
    note over SS: BeginPlay
    SS->>SS: 타이머 시작
    loop 지정된 Interval마다 반복
        SS->>SS: CheckNearbyInteractable() 호출
        alt CurrentInteractable이 변경됨
            SS->>SC: RPC호출(변경사항 즉시 반영)
        end
    end
    activate GAS
    note over GAS: 자식 클래스에서
    GAS->>SS: InteractWithCurrentInteractable 호출

    note over SS: InteractWithCurrentInteractable
    SS->>IC: Interface를 통해 StartInteraction 호출
    SS->>SS: 탐색 중지
    GAS->>SS: EndInteraction 호출
    SS->>IC: Interface를 통해 FinishInteraction 호출
    SS->>SS: 탐색 재개
   
    deactivate GAS
    deactivate SS
    ```
*   **스킬체크 컴포넌트 (Skill Check)**
    *   수리나 치료 중 무작위로 발생하는 QTE 시스템.
    *   성공/대성공/실패에 따른 진행도 보너스 및 페널티 적용.
    *   **스킬체크 동작 시퀀스 다이어그램**
    ```mermaid
    sequenceDiagram
    participant S as USkillCheckComponent
    participant GAC as USurvivorInteractionAbility(Client)
    participant GAS as USurvivorInteractionAbility(Server)

    activate GAC
    note right of GAC: 자식 클래스의 ActivateAbility
    GAC->>GAC: SetRandomSkillCheckEnabledOnClient(float Frequency)  호출
    alt 어빌리티 활성화
        GAC->>S: SkillCheckEndDelegate에 가입
        note right of GAC: SetRandomSkillCheckEnabledOnClient
        GAC->>GAC: 스킬체크 타이머 시작(시간 랜덤)
        activate S
        GAC->>S: 타이머 발동 시 TriggerOneShotSkillCheck 호출
        S->>S: 스킬체크 미니게임 진행(Tick으로 타이밍 측정)
        note over GAC,S: 스킬체크 종료 루틴
        S->>GAC: 스킬체크 종료 후 Delegate로 결과값(ESkillCheckResult) 전달
        deactivate S
        GAC->GAS: RPC로 결과를 서버로 전달
    else 어빌리티 종료
        alt 스킬체크 진행중
            note over GAC,S: 스킬체크 종료 루틴
        end
    end
    deactivate GAC
    ```
*   **퍽 (Perk) 시스템**
    *   Dead by Daylight의 주요 퍽 구현 (Adrenaline, Sprint Burst, Self Care, Bond 등).
    *   다형성을 활용하여 확장성 확보
    *   **퍽 클래스 다이어그램**
    ```mermaid
    classDiagram
    direction TB
    class ADBDCharacter {
	    #AuthInitPerks() void
	    #Client_UpdatePerk(UPerkComponent* ...) void
    }

    class UPerkComponent {
	    +OnServerSideInitialized() void*
	    +OnOwnerClientSideInitialized() void*
    }

    class UPerkComponent자식클래스{
        UPerk_Adrenaline
        UPerk_Bond
        UPerk_BotanyKnowledge
        UPerk_Empathy
        UPerk_Leader
        UPerk_ProveThyself
        UPerk_QuickAndQuiet
        UPerk_SelfCare
        UPerk_SprintBurst
    }

    ADBDCharacter --> UPerkComponent
    UPerkComponent <|-- UPerkComponent자식클래스
    ```
*   **아이템 & 애드온**
    *   구급상자, 공구상자 등 아이템 장착, 버리기, 사용 구현.
    *   아이템의 능력을 강화하는 애드온 구현
*   **캐릭터 오라 시스템**
    *   서브시스템을 활용한 구현
*   **생존자 발자국 시스템**
    *   **Object Pooling**: `ScratchMark`(발자국)와 같이 빈번하게 생성/삭제되는 객체에 풀링 시스템 적용하여 성능 최적화

## 📂 자료
*   [📄 발표 PPT](https://drive.google.com/drive/folders/1GIwR3PEj1KsdwtcEwJnWt60qXxBfUbw3?usp=sharing)
*   [📺 시연 영상](https://youtu.be/ro7gwYu5df8)

## ⚠️ 참고 사항
*   본 프로젝트는 학습 및 포트폴리오 목적으로 제작되었습니다.
*   저장소에는 Source Code만 포함되어 있으며, 에셋(Asset) 파일은 포함되어 있지 않습니다.
