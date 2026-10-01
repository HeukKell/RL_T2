// Fill out your copyright notice in the Description page of Project Settings.


#include "Chaser.h"
#include "Runner.h"
#include "RL_T2_UE.h"

// Sets default values
AChaser::AChaser()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	RotateDelta = 3.0f;

	TrainingProcess = ETrainingProcess::IDLE;
}

// Called when the game starts or when spawned
void AChaser::BeginPlay()
{
	Super::BeginPlay();

	if (!MapSharedMemory()) {
		EndPlay(EEndPlayReason::Quit);
	}

	TrainingProcess = ETrainingProcess::IDLE;
}

void AChaser::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (hTrainTimer.IsValid()) {
		GetWorld()->GetTimerManager().PauseTimer(hTrainTimer);
		GetWorld()->GetTimerManager().ClearTimer(hTrainTimer);
	}

	UnmapSharedMemory();
}

// Called every frame
void AChaser::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	static float TickTimer = 0;

	if (nullptr == SharedMem) return;

	switch (TrainingProcess) {
		case ETrainingProcess::BEGIN_TRAIN: {

			if (nullptr == Runner) {
				
				ActorLog(TEXT("Warning"), TEXT("Runner is not valid"));
				TrainingProcess = ETrainingProcess::IDLE;
			}

			RunnerController = Cast<AAIController>(Runner->GetController());

			if (nullptr == RunnerController) {
				ActorLog(TEXT("Warning"), TEXT("RunnerController is not valid"));
				return;
			}

			SharedMem->Training = 1;

			TrainingProcess = ETrainingProcess::BEGIN_RUNNER_RUN;
			break;
		}
		case ETrainingProcess::BEGIN_RUNNER_RUN: {

			// 도망 시켜
			RunnerController->ReceiveMoveCompleted.AddDynamic(this, &AChaser::OnBind_MoveCompleted_Runner);
			Runner->Run();

			TrainingProcess = ETrainingProcess::WAITING_RUNNER_RUN;
			break;
		}
		case ETrainingProcess::WAITING_RUNNER_RUN: {

			ActorLog(TEXT("Log"), TEXT("Runner 기다리는중..."));
			break;
		}
		case ETrainingProcess::END_RUNNER_RUN: {

			FString DebugMsg_OnStart = FString::Printf(TEXT("Train Start (%d / %d)"), RunInvoke_Curr, RunInvoke_Target);
			ActorLog(TEXT("Log"), *DebugMsg_OnStart);

			// Runner 가 도망갔다!

			if (nullptr == Runner) {
				ActorLog(TEXT("Warning"), TEXT("Invalid Runner"));
			}

			if (nullptr == RunnerController) {
				ActorLog(TEXT("Warning"), TEXT("Invalid RunnerController"));
				return;
			}

			RunnerController->ReceiveMoveCompleted.RemoveDynamic(this, &AChaser::OnBind_MoveCompleted_Runner);

			// 거리 측정시작
			float TargetDir = 0.0f;
			if (!CheckTargetDir(Runner, TargetDir)) {

				// 거리측정 실패했다면,
				ActorLog(TEXT("Warning"), TEXT("Failed to checkTargetDir"));
				return;
			}

			// 거리는 잘 측정했다.
			int32 TargetDir_ToShare = AngleToSimpleDirection(TargetDir);

			SharedMem->Reward = 0;
			SharedMem->RewardSetted = 0;
			SharedMem->CurrentState = TargetDir_ToShare;	// 처음엔 똑같이
			SharedMem->NextState = TargetDir_ToShare;		// 처음엔 똑같이
			SharedMem->StateSetted = 1;

			TrainingProcess = ETrainingProcess::WAITING_ACTION;
			break;
		}
		case ETrainingProcess::WAITING_ACTION: {


			if (SharedMem->ActionSetted == 1) {
				TrainingProcess = ETrainingProcess::SETTED_ACTION;
			}

			break;
		}
		case ETrainingProcess::SETTED_ACTION: {

			int32 Action = SharedMem->Action - 1;			// 0,1,2 -> -1, 0, 1
			SharedMem->ActionSetted = 0;

			if (nullptr == Runner) {
				ActorLog(TEXT("Warning"), TEXT("Runner is not valid"));
				TrainingProcess = ETrainingProcess::IDLE;
				break;
			}

			CheckTargetDir(Runner, PrevAngle);
			Rotate(Action);

			TickTimer = 0.0f;
			TrainingProcess = ETrainingProcess::ROTATING;
			break;
		}
		case ETrainingProcess::ROTATING: {
			// 회전하는데에도 시간이 필요

			TickTimer += DeltaTime;

			if (TickTimer >= 0.5f) {
				TickTimer = 0.0f;
				TrainingProcess = ETrainingProcess::END_ROTATE;
			}

			break;
		}
		case ETrainingProcess::END_ROTATE: {

			CheckTargetDir(Runner, PostAngle);

			Reward_Temp = 0.0f;

			float PrevAngle_Error = FMath::Abs(PrevAngle);
			float PostAngle_Error = FMath::Abs(PostAngle);

			if (PostAngle_Error < PrevAngle_Error) {
				// 가까워 졌다.
				Reward_Temp = 1.0f;

			}
			else if (PostAngle_Error > PrevAngle_Error) {
				// 더 멀여졌다.
				Reward_Temp = -1.0f;
			}
			else {
				// 변화 없음
				// 중앙에서 멈춘경우라면, 목적지라 확신했겠지.
				if (PostAngle_Error <= 1.0f) {
					Reward_Temp = 10.0f;
				}
				else {
					Reward_Temp = -0.1f;
				}
			}

			// 보상과 다음 상태까지 다 넘겨줘야, 이에 맞게 q-learning 을 한다.
			SharedMem->CurrentState = AngleToSimpleDirection(PrevAngle);
			SharedMem->NextState = AngleToSimpleDirection(PostAngle);

			SharedMem->Reward = Reward_Temp;
			SharedMem->RewardSetted = 1;

			TrainingProcess = ETrainingProcess::WAITING_REWARD_OPEN;

			break;
		}
		case ETrainingProcess::WAITING_REWARD_OPEN: {

			if (SharedMem->RewardSetted == 0) {
				TrainingProcess = ETrainingProcess::CHECK_CONTINUE;
			}

			break;
		}
		case ETrainingProcess::CHECK_CONTINUE: {

			if (Reward_Temp == 10.0f) {
				// 정답을 맞췄으니 다시 도망갈꺼야.

				Reward_Temp = 0.0f;

				RunInvoke_Curr += 1;
				
				FString DebugMsg = FString::Printf(TEXT("Train %d times runned"), RunInvoke_Curr);
				ActorLog(TEXT("Log"), DebugMsg);

				if (RunInvoke_Curr >= RunInvoke_Target) {
					
					// 목표 학습량 수행 완료, 종료
					RunInvoke_Target = 0;
					RunInvoke_Curr = 0;

					SharedMem->Training = 0;
					TrainingProcess = ETrainingProcess::IDLE;
					break;
				}

				TrainingProcess = ETrainingProcess::BEGIN_RUNNER_RUN;
				break;
			}
			else {

				int32 NewState = AngleToSimpleDirection(PostAngle);

				SharedMem->CurrentState = NewState;	
				SharedMem->NextState = NewState;
				SharedMem->StateSetted = 1;

				TrainingProcess = ETrainingProcess::WAITING_ACTION;
				break;
			}

			break;
		}
	}	
}

// Called to bind functionality to input
void AChaser::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

bool AChaser::MapSharedMemory()
{
	
	SharedMemoryRegion = FPlatformMemory::MapNamedSharedMemoryRegion(
		TEXT("Chaser_Memory_1"), // 자동으로 Global\ 이 붙는다. 즉, python 에서는 관리자 없이 Global 을 붙일 수 없으므로 관리자 모드로 python 을 실행한다.
		false, // true 면 생성, false 면 기본 메모리 읽기
		FPlatformMemory::ESharedMemoryAccess::Read | FPlatformMemory::ESharedMemoryAccess::Write,
		sizeof(FSharedMemoryType)
	);

	if (SharedMemoryRegion) {
		// 성공시

		void* SharedMemoryAddr = SharedMemoryRegion->GetAddress();
		SharedMem = static_cast<FSharedMemoryType*>(SharedMemoryAddr);

		ActorLog(TEXT("Log"), TEXT("success connect shared memory"));
				
		//FString DebugMsg = FString::Printf(TEXT("found : %d"), SharedMem->PrevDirection);
		//UE_LOG(LogTemp, Warning, TEXT("global_shared_mem_1.data == %d"), SharedMem->PrevDirection);

		return true;
	}
	else {
		ActorLog(TEXT("Error"), TEXT("failed shared mem open"));

		return false;
	}

	
}

bool AChaser::UnmapSharedMemory()
{
	if (!SharedMemoryRegion) {
		return false;
	}

	FPlatformMemory::UnmapNamedSharedMemoryRegion(
		SharedMemoryRegion
	);

	SharedMemoryRegion = nullptr;

	return true;
}

FSharedMemoryType* AChaser::GetSharedMem()
{
	
	if (nullptr == SharedMem) {

		void* SharedMem_Addr = SharedMemoryRegion->GetAddress();
		SharedMem = static_cast<FSharedMemoryType*>(SharedMem_Addr);
	}

	return SharedMem;
}

bool AChaser::CheckTargetDir(AActor* Target, float& Direction_Deg) const
{

	if (nullptr == Target) {
		return false; // 방향 못잡음
	}

	// 이 액터의 전방벡터
	FVector ForwardVect = GetActorForwardVector();
	ForwardVect.Z = 0.0f;
	ForwardVect.Normalize();

	// 목표물을 바라보는 벡터
	FVector DirVect_ToTarget = Target->GetActorLocation() - GetActorLocation();
	DirVect_ToTarget.Z = 0.0f;
	DirVect_ToTarget.Normalize();

	float dot_product = FVector::DotProduct(ForwardVect, DirVect_ToTarget);
	FVector crossProduct = FVector::CrossProduct(ForwardVect, DirVect_ToTarget);
	float XY_Direction = crossProduct.Z; // XY 평면상에서 전방벡터를 기준으로 목표벡터가 어느방향에 있는지, 양수라면, CW 기준 시계방향에 있다. 즉 오른쪽

	float Angle_Rad = FMath::Atan2(XY_Direction, dot_product);
	Direction_Deg = FMath::RadiansToDegrees(Angle_Rad);

	return true;
}

void AChaser::Rotate(int32 RotateDirection)
{
	
	if (RotateDirection > 0) {
		AddActorLocalRotation(FRotator(0.0f, RotateDelta, 0.0f));
	}
	else if (RotateDirection < 0) {
		AddActorLocalRotation(FRotator(0.0f, -RotateDelta, 0.0f));
	}
	else {

	}
}

void AChaser::ActorLog(const FString& Verbose, const FString& DebugMessage)
{

	if (Verbose.Equals(TEXT("Warning"))) {

		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Orange, *DebugMessage);
		PLOG(Warning, TEXT("%s"), *DebugMessage);
	}
	else if (Verbose.Equals(TEXT("Error"))) {
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, *DebugMessage);
		PLOG(Error, TEXT("%s"), *DebugMessage);
	}
	else {
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Black, *DebugMessage);
		PLOG(Log, TEXT("%s"), *DebugMessage);
	}
}

int32 AChaser::AngleToSimpleDirection(float angle) const
{
	if (angle < -1.0f) {
		return 0;
	}
	else if (angle > 1.0f) {
		return 2;
	}
	else{ // angle == 0
		return 1;
	}
	
}

void AChaser::TrainModel(int32 NewRunInvoke)
{

	if (nullptr == Runner) {
		return;
	}

	RunInvoke_Curr = 0;
	RunInvoke_Target = NewRunInvoke;

	TrainingProcess = ETrainingProcess::BEGIN_TRAIN;
}

void AChaser::OnBind_MoveCompleted_Runner_Implementation(FAIRequestID RequestID, EPathFollowingResult::Type Result)
{
	TrainingProcess = ETrainingProcess::END_RUNNER_RUN;
}
