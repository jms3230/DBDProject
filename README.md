# DBDProject (Dead by Daylight Clone)

[![Video Label](http://img.youtube.com/vi/ro7gwYu5df8/0.jpg)](https://youtu.be/ro7gwYu5df8)

## 📝 프로젝트 소개
**Dead by Daylight**를 모작한 비대칭 멀티플레이어 서바이벌 호러 게임 프로젝트입니다.  
언리얼 엔진 5와 C++를 기반으로 개발되었으며, **Gameplay Ability System (GAS)**을 적극적으로 활용하여 캐릭터의 능력, 상태, 상호작용을 체계적으로 구현했습니다.

## 👥 팀원 및 역할
| 이름 | 역할 | 담당 파트 |
|:---:|:---:|:---|
| **정민수 (팀장)** | **Survivor** | **생존자 캐릭터 FSM, GAS 시스템 설계, 상호작용, 퍽/아이템 시스템** |
| 모명준 | Object | 게임 플로우, 발전기/판자 등 오브젝트 상호작용 |
| 김미진 | Killer | 살인마 캐릭터 FSM, AI, 공격 시스템 |
| 유호근 | Map/UI | 맵 레벨 디자인, 정보 시스템, UI |

## 📅 제작 기간
*   **2025.09.01 - 2025.11.06**

## 🛠 기술 스택 (Tech Stack)
### Development
<div>
<img src="https://img.shields.io/badge/Unreal Engine 5-0E1128?style=flat-square&logo=Unreal Engine&logoColor=white"/>
<img src="https://img.shields.io/badge/C++-00599C?style=flat-square&logo=c%2B%2B&logoColor=white"/>
<img src="https://img.shields.io/badge/Gameplay Ability System-FE5F55?style=flat-square&logo=unrealengine&logoColor=white"/>
</div>

### Tools & Communication
<div>
<img src="https://img.shields.io/badge/Notion-000000?style=flat-square&logo=notion&logoColor=white"/>
<img src="https://img.shields.io/badge/Discord-5865F2?style=flat-square&logo=discord&logoColor=white"/>
<img src="https://img.shields.io/badge/Perforce-4C00FF?style=flat-square&logo=perforce&logoColor=white"/>
<img src="https://img.shields.io/badge/GitHub-181717?style=flat-square&logo=github&logoColor=white"/>
</div>

## 💡 주요 기능 (Key Features)
### 🏃‍♂️ 생존자 (Survivor) 시스템 (담당: 정민수)
*   **상태 관리 (State Management)**
    *   건강함(Healthy) -> 부상(Injured) -> 빈사(Dying) -> 갈고리(Hooked) -> 사망(Dead)으로 이어지는 생명 주기 구현.
    *   각 상태에 따른 이동 속도 변화 및 애니메이션 처리.
*   **Gameplay Ability System (GAS)**
    *   **AttributeSet**: 체력, 이동 속도, 수리/치료 속도 등 캐릭터 스탯 관리.
    *   **GameplayAbility**: 달리기(Sprint), 치료(Heal), 수리(Repair) 등 액션의 모듈화.
    *   **GameplayEffect**: 아이템/퍽 효과 적용 및 상태 이상(Buff/Debuff) 처리.
*   **상호작용 (Interaction)**
    *   `InteractableComponent`와 `InteractorComponent`를 통한 유연한 상호작용 구조.
    *   발전기 수리, 동료 치료, 아이템 루팅, 창틀 넘기 등 다양한 상호작용 구현.
*   **스킬 체크 (Skill Check)**
    *   수리나 치료 중 무작위로 발생하는 QTE 시스템.
    *   성공/대성공/실패에 따른 진행도 보너스 및 페널티 적용.
*   **퍽 (Perk) 시스템**
    *   Dead by Daylight의 주요 퍽 구현 (Adrenaline, Sprint Burst, Self Care, Bond 등).
    *   Tag 기반의 조건 체크를 통해 다양한 퍽 효과를 유연하게 적용.
*   **아이템 (Item)**
    *   구급상자, 공구통 등 아이템 장착 및 사용 구현.
    *   아이템 애드온(Addon) 시스템을 위한 확장성 고려.
*   **최적화 (Optimization)**
    *   **Object Pooling**: `ScratchMark`(발자국)와 같이 빈번하게 생성/삭제되는 객체에 풀링 시스템 적용하여 성능 최적화.

## 📂 자료
*   [📄 발표 PPT](https://drive.google.com/drive/folders/1GIwR3PEj1KsdwtcEwJnWt60qXxBfUbw3?usp=sharing)
*   [📺 시연 영상](https://youtu.be/ro7gwYu5df8)

## ⚠️ 참고 사항
*   본 프로젝트는 학습 및 포트폴리오 목적으로 제작되었습니다.
*   저장소에는 Source Code만 포함되어 있으며, 에셋(Asset) 파일은 포함되어 있지 않아 정상 구동이 어려울 수 있습니다.
*   코드 구조 및 구현 방식을 확인하는 용도로 참고해주시기 바랍니다.
