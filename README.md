# Orbital Snipe

**Orbital Snipe**는 Unreal Engine 5와 C++로 구현한 물리 기반 스코어 어택 게임 프로토타입입니다.

제한된 포탄으로 천체의 중력을 이용해 탄도를 조절하고,  
첫 충돌 이후 발생하는 천체 간 연쇄 충돌을 통해 모든 Target을 제거하는 것이 목표입니다.

단순히 목표물을 직접 맞히는 방식이 아니라,

**중력에 의한 탄도 편향 → 첫 충돌 → 연쇄 충돌 → 점수 획득**

을 하나의 게임 메커니즘으로 연결하는 것을 핵심으로 설계했습니다.

---

## Gameplay

1. 플레이어가 대포에 탑승합니다.
2. 포신의 방향과 발사 세기를 조절합니다.
3. 화면에 표시되는 예상 궤적을 확인합니다.
4. 제한된 포탄을 발사합니다.
5. Projectile이 Target과 충돌하면 해당 Target이 활성화됩니다.
6. 활성화된 Target이 다른 Target과 충돌하면서 연쇄 반응이 발생합니다.
7. 연쇄 깊이에 따라 더 높은 점수를 획득합니다.
8. 모든 Target을 제거하면 Stage Clear입니다.
9. 탄환을 모두 사용하고 더 이상 연쇄 충돌이 발생할 수 없으면 Stage Failed입니다.

---

## Core Features

### Gravity-based Projectile Simulation

Projectile은 아직 충돌하지 않은 Target들로부터 중력 가속도를 받습니다.

각 중력원의 영향을 합산하여 Projectile의 속도와 위치를 갱신합니다.

현재 물리 업데이트는 다음 형태의 **Semi-Implicit Euler 적분**을 사용합니다.

```text
velocity += acceleration * dt
position += velocity * dt
```

중력 계산과 Actor 이동은 Unreal Physics에 전적으로 맡기지 않고 C++ 게임 로직에서 직접 처리했습니다.

---

### Real-time Trajectory Prediction

현재 포신 방향과 발사 세기를 기준으로 Projectile의 예상 궤적을 실시간으로 계산합니다.

예측 궤적에는 실제 Projectile과 동일한 중력 모델을 적용하며,  
첫 번째 Target 충돌 지점까지만 궤적을 표시합니다.

이를 통해 플레이어가 중력에 의해 휘어지는 탄도를 사전에 확인하고 발사 방향을 결정할 수 있습니다.

---

### Chain Collision System

Target은 처음에는 고정된 중력원으로 존재합니다.

Projectile이 Target을 맞히면 해당 Target이 활성화되어 이동하기 시작하고,  
활성화된 Target은 다른 Target과 충돌하여 추가적인 연쇄 반응을 발생시킬 수 있습니다.

```text
Projectile
    ↓
Target A
    ↓
Target B
    ↓
Target C
```

동일 프레임에서 충돌이 무제한으로 전파되는 것을 방지하기 위해 이전 프레임의 Hit 상태를 별도로 관리합니다.

---

### Chain Score

각 Projectile에는 Shot ID를 부여하고, 충돌이 전달될 때마다 Chain Depth를 증가시킵니다.

현재 점수 계산은 다음과 같습니다.

```text
Score = 100 × Chain Depth
```

예:

```text
Projectile → A    +100
A → B             +200
B → C             +300
C → D             +400
```

따라서 단순 직접 명중보다 긴 연쇄 충돌을 만드는 플레이가 더 높은 점수로 연결됩니다.

---

### Limited Shot Stage System

기본적으로 한 Stage에서 사용할 수 있는 포탄 수는 제한되어 있습니다.

HUD에서 다음 정보를 실시간으로 확인할 수 있습니다.

```text
SHOTS
TARGETS
SCORE
```

모든 Target을 제거하면:

```text
STAGE CLEAR
```

탄환을 모두 사용한 뒤 남은 Projectile 또는 활성 Target으로 더 이상 연쇄 반응을 만들 수 없으면:

```text
STAGE FAILED
```

상태가 됩니다.

---

### Active Body Lifecycle

충돌 이후 활성화된 Projectile과 Target이 맵에 계속 남아 있는 문제를 방지하기 위해 별도의 생명주기를 관리합니다.

활성 물체는 다음 조건에 따라 제거됩니다.

- 일정 활성 시간이 지난 경우
- 지정된 활동 영역을 벗어난 경우

Actor를 제거하기 전에 Gravity Manager의 관리 배열에서도 해당 객체를 제거하여 수명 종료 이후 잘못된 포인터를 참조하지 않도록 구성했습니다.

---

### Stage Initialization & Retry

Stage Clear 또는 Stage Failed 이후 `Home` 키를 누르면 현재 Level을 다시 로드합니다.

재시작 시:

- Score
- Remaining Shots
- Remaining Targets
- Projectile
- Activated Targets
- Stage State

가 모두 초기 상태로 복구됩니다.

World Partition 환경에서 Level 재로드 직후 Target Actor의 준비 시점과 `BeginPlay()` 실행 시점이 달라질 수 있기 때문에,  
Target들이 안정적으로 검색된 이후 Stage를 초기화하도록 별도의 초기화 상태를 관리합니다.

게임 진행 중에는 `Home` 입력을 무시하여 실수로 Stage가 초기화되지 않도록 했습니다.

---

### Player / Cannon Possession

게임은 일반 Player Pawn으로 시작합니다.

플레이어는 대포 주변에서 `F` 키를 눌러 Cannon Pawn을 조종할 수 있으며,  
다시 `F` 키를 누르면 원래 Player Pawn으로 복귀합니다.

하차 시에는 대포의 Pitch와 무관하게 수평 방향을 기준으로 위치를 계산하고,  
Line Trace로 실제 지면을 찾아 Player를 배치합니다.

---

## Controls

### Player

| Key | Action |
| --- | --- |
| `WASD` | Move |
| `F` | Enter Cannon |

### Cannon

| Key | Action |
| --- | --- |
| `A / D` | Rotate |
| `W / S` | Elevate |
| `Q / E` | Adjust Power |
| `Left Mouse Button` | Fire |
| `F` | Exit Cannon |

### Stage Result

| Key | Action |
| --- | --- |
| `Home` | Retry |

`Home`은 Stage Clear 또는 Stage Failed 상태에서만 동작합니다.

---

## Architecture

주요 게임 로직은 C++ 클래스로 구성했습니다.

### `AGravityBody`

Projectile과 Target이 공통으로 사용하는 중력체 Actor입니다.

주요 데이터:

- Body Type
- Mass
- Radius
- Velocity
- Hit State
- Shot ID
- Chain Depth

---

### `AGravityManager`

게임의 물리 및 Stage 상태를 관리합니다.

주요 역할:

- Gravity Body 관리
- Projectile 중력 계산
- 위치 적분
- 충돌 판정
- 탄성 충돌 처리
- Target 활성화
- Chain Depth 계산
- Score 계산
- Target 개수 관리
- Clear / Failed 판정
- 활성 Body Lifetime 관리
- Stage 초기화

---

### `AOrbitalCannon`

대포의 입력과 발사를 담당합니다.

주요 역할:

- Yaw 조절
- Pitch 조절
- Power 조절
- Projectile 생성
- 실시간 예상 궤적 계산
- Remaining Shot 관리

---

### `AOrbitalPlayerController`

Player와 Cannon 사이의 조종권 전환 및 Stage Retry 입력을 담당합니다.

주요 역할:

- Cannon 탐색
- Player → Cannon Possess
- Cannon → Player Possess
- 안전한 하차 위치 탐색
- Stage 종료 후 Retry

---

### `AOrbitalHUD`

게임 상태와 조작 정보를 C++ HUD로 표시합니다.

표시 정보:

- Remaining Shots
- Remaining Targets
- Score
- Player Controls
- Cannon Controls
- Stage Clear / Failed
- Retry 안내

---

## Technical Focus

이 프로젝트에서는 Unreal Engine 기능을 사용하는 것뿐 아니라,  
게임 규칙에 필요한 핵심 계산과 상태 관리를 C++로 직접 구현하는 데 중점을 두었습니다.

특히 다음 문제를 직접 다뤘습니다.

- 복수 중력원 기반 Projectile 운동 계산
- 실제 발사와 예상 궤적 계산의 일관성
- 연쇄 충돌의 프레임 단위 전파 제어
- 충돌 깊이를 이용한 점수 시스템
- 동적 Actor의 안전한 수명 관리
- 탄환 소진 이후에도 진행 중인 연쇄 충돌을 고려한 Fail 판정
- World Partition Level 재로드 시 Actor 초기화 순서 처리
- Pawn Possession을 이용한 Player / Cannon 조작 전환

---

## Development Environment

- Unreal Engine 5.4
- C++
- Visual Studio 2022
- Git / GitHub

---

## Future Work

현재 핵심 게임 루프와 Stage Clear / Failed / Retry 흐름까지 구현되어 있습니다.

추가 개선 후보:

- Physics Solver 구조 분리
- RK4 적분 구현 및 현재 적분 방식과의 비교
- 실제 시뮬레이션과 Trajectory Prediction의 공통 Solver화
- Stage 구성 다양화
- 충돌 및 발사 효과 개선
- 사운드 및 게임 피드백 추가

RK4는 현재 구현된 기능이 아니라 향후 개선 항목입니다.

---

## Third-Party Assets

### Early 1900s Artillery Cannon

- Creator: Zack_Hawley
- Source: Sketchfab
- License: Creative Commons Attribution (CC BY)
- Original model:  
  https://sketchfab.com/3d-models/early-1900s-artillery-cannon-7fbd4063876446a482bc2b37520f03b2

The model was imported and adapted for use in Unreal Engine.