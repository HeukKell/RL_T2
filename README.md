# RL_T2

Unreal Engine 환경과 Python 학습 프로세스를 Windows 공유 메모리로 연결하여 간단한 Q-Learning을 실험하는 프로젝트입니다.

Unreal Engine의 `Chaser`는 `Runner`를 향한 상대 방향을 상태로 만들고, Python은 Q-Table을 바탕으로 회전 행동을 선택합니다. Unreal Engine은 행동 결과에 따른 보상을 계산하여 다시 Python에 전달합니다.

## 프로젝트 구성

```text
RL_T2/
├─ RL_T2_UE/                 # Unreal Engine 프로젝트
│  ├─ Config/                # 엔진, 게임 및 입력 설정
│  ├─ Content/               # 레벨, 블루프린트 및 UI 에셋
│  ├─ Source/RL_T2_UE/       # Chaser, Runner 및 게임 모드 C++ 코드
│  └─ RL_T2_UE.uproject
├─ RL_T2_PY/                 # Python Q-Learning 프로세스
│  ├─ main.py
│  ├─ pyproject.toml
│  └─ uv.lock
└─ README.md
```

## 동작 방식

학습은 다음 순서로 진행됩니다.

1. Python 프로세스가 `Global\\Chaser_Memory_1` 공유 메모리를 생성합니다.
2. Unreal Engine의 `Chaser`가 같은 공유 메모리에 연결합니다.
3. `Runner`가 새로운 위치로 이동합니다.
4. Unreal Engine이 `Chaser` 기준의 목표 방향을 상태로 기록합니다.
5. Python이 Q-Table에서 행동을 선택하여 공유 메모리에 기록합니다.
6. `Chaser`가 선택된 방향으로 회전합니다.
7. Unreal Engine이 회전 전후의 오차를 비교하여 보상을 기록합니다.
8. Python이 보상과 다음 상태를 이용해 Q-Table을 갱신합니다.

상태와 행동은 각각 세 단계로 단순화됩니다.

| 값 | 상태 | 행동 |
|---:|---|---|
| `0` | 목표가 왼쪽에 있음 | 왼쪽 회전 |
| `1` | 목표가 정면에 있음 | 정지 |
| `2` | 목표가 오른쪽에 있음 | 오른쪽 회전 |

보상은 목표와의 각도 오차 변화에 따라 결정됩니다.

| 조건 | 보상 |
|---|---:|
| 이전보다 목표에 가까워짐 | `+1.0` |
| 이전보다 목표에서 멀어짐 | `-1.0` |
| 정면 범위에서 정지 | `+10.0` |
| 방향 변화가 없고 정면이 아님 | `-0.1` |

## 공유 메모리 구조

Unreal Engine의 `FSharedMemoryType`과 Python의 바이트 오프셋은 아래 구조를 공유합니다. 모든 정수와 실수는 4바이트 little-endian 값입니다.

| 오프셋 | 형식 | 필드 | 설명 |
|---:|---|---|---|
| `0` | `uint32` | `CurrentState` | 현재 상태 |
| `4` | `uint32` | `NextState` | 행동 이후 상태 |
| `8` | `uint32` | `Action` | Python이 선택한 행동 |
| `12` | `float` | `Reward` | Unreal Engine이 계산한 보상 |
| `16` | `uint32` | `StateSetted` | 상태 기록 완료 플래그 |
| `20` | `uint32` | `ActionSetted` | 행동 기록 완료 플래그 |
| `24` | `uint32` | `RewardSetted` | 보상 기록 완료 플래그 |
| `28` | `uint32` | `Training` | 학습 진행 상태 |

현재 Python은 64바이트의 공유 메모리를 생성하며, 실제 프로토콜은 앞의 32바이트를 사용합니다.

## 요구 사항

- Windows
- Unreal Engine 5 계열과 C++ 빌드 도구
- Python `3.11.x`
- [uv](https://docs.astral.sh/uv/)
- CUDA 12.4 호환 환경
  - 현재 `uv.lock`과 `pyproject.toml`은 PyTorch `2.5.1` CUDA 12.4 패키지를 사용합니다.

`RL_T2_UE.uproject`의 `EngineAssociation`은 로컬 엔진 식별자이므로, 다른 PC에서는 사용할 Unreal Engine 설치를 다시 선택해야 할 수 있습니다.

## 환경 구성

저장소를 받은 후 Python 의존성을 설치합니다.

```powershell
cd RL_T2_PY
uv sync
```

Unreal Engine 프로젝트는 `RL_T2_UE/RL_T2_UE.uproject`를 열어 필요한 프로젝트 파일을 생성하고 C++ 모듈을 빌드합니다.

## 실행 순서

공유 메모리는 Python에서 생성하고 Unreal Engine에서는 기존 메모리를 열도록 구현되어 있으므로 Python 프로세스를 먼저 실행해야 합니다.

```powershell
cd RL_T2_PY
uv run python main.py
```

Windows의 `Global\\` 공유 메모리 네임스페이스 접근이 거부되면 터미널을 관리자 권한으로 실행해야 할 수 있습니다.

그다음 Unreal Editor에서 다음 작업을 수행합니다.

1. `RL_T2_UE.uproject`를 엽니다.
2. 메인 레벨을 실행합니다.
3. `Chaser`와 `Runner` 참조가 연결되어 있는지 확인합니다.
4. 블루프린트에서 `TrainModel`을 호출하고 반복 횟수를 전달합니다.

## 주요 구현 위치

- `RL_T2_UE/Source/RL_T2_UE/Chaser.cpp`
  - 공유 메모리 연결, 학습 상태 머신, 행동 적용 및 보상 계산
- `RL_T2_UE/Source/RL_T2_UE/Runner.cpp`
  - 지정 반경 안에서 임의 목적지 계산
- `RL_T2_PY/main.py`
  - 공유 메모리 생성, epsilon-greedy 행동 선택 및 Q-Table 갱신

현재 Python 학습 파라미터는 다음과 같습니다.

- 탐험 확률: `0.2`
- 학습률: `0.2`
- 미래 보상 반영 비율: `0.2`
- Q-Table 크기: `3 × 3`

