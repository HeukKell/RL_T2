// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"

#include "HAL/PlatformMemory.h"
#include "HAL/UnrealMemory.h"
#include "AIController.h"

#include "Chaser.generated.h"

UENUM(BlueprintType)
enum class ETrainingProcess : uint8 {

	IDLE,
	BEGIN_TRAIN,
	BEGIN_RUNNER_RUN,
	WAITING_RUNNER_RUN,
	END_RUNNER_RUN,
	SETTED_STATE,
	WAITING_ACTION,
	SETTED_ACTION,
	ROTATING,
	END_ROTATE,
	WAITING_REWARD_OPEN,	// 보상을 읽을때까지 기다립니다.
	CHECK_CONTINUE			// 계속 진행할지 확인
};


struct FSharedMemoryType {
	//uint32 PrevDirection;	// 이전 상태. py 에서 보상을 받을때, 이전 상태를 알아야 그곳에 기록할 수 있기 때문에
	uint32 CurrentState; // 0 : left, 1 : mid , 2 : right
	uint32 NextState; // 0 : left, 1: mid, 2: right
	uint32 Action; // 0 : 왼쪽 회전, 1 : 가만히 있기, 2 : 우측 회전
	float Reward; // -1 : 더 멀어진경우 , +1 : 가까워진경우, +10 정확하게 맞춤(오차 5도 이내)

	uint32 StateSetted;		// 목표물의 방향이 설정된경우 
	uint32 ActionSetted;		// 행동이 설정된 경우
	uint32 RewardSetted;		// 보상이 설정된 경우

	uint32 Training;
};

UCLASS(Blueprintable)
class RL_T2_UE_API AChaser : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AChaser();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

public:

	UFUNCTION(BlueprintCallable)
	bool MapSharedMemory();

	UFUNCTION(BlueprintCallable)
	bool UnmapSharedMemory();

	FPlatformMemory::FSharedMemoryRegion* SharedMemoryRegion = nullptr;

	FSharedMemoryType* SharedMem = nullptr;

public:

	FSharedMemoryType* GetSharedMem();
public:

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float RotateDelta;
public:
	

	/** Runner 의 방향을 계산 
		@param Target 목표물 참조
		@param Direction_Deg 전방벡터와 이루는 각도
		@return 계산 성공 여부
	*/
	UFUNCTION(BlueprintCallable)
	bool CheckTargetDir(AActor* Target, float& Direction_Deg) const;

	/**
		회전합니다. 
		@param RotateDirection 양수면 시계방향, 음수면 반시계, 0 이면 정지
	*/
	UFUNCTION(BlueprintCallable)
	void Rotate(int32 RotateDirection);

	UFUNCTION(BlueprintCallable)
	void ActorLog(const FString& Verbose, const FString& DebugMessage);

	int32 AngleToSimpleDirection(float angle) const;
public:

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void OnBind_MoveCompleted_Runner(FAIRequestID RequestID, EPathFollowingResult::Type Result);
	void OnBind_MoveCompleted_Runner_Implementation(FAIRequestID RequestID, EPathFollowingResult::Type Result);

public:

	/**
		AI Model 훈련시작 합니다
	*/
	UFUNCTION(BlueprintCallable)
	void TrainModel(int32 NewRunInvoke);

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	ETrainingProcess TrainingProcess; // 학습 진행상태

	// 현재 학습량
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 RunInvoke_Curr;

	// 목표 학습량
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 RunInvoke_Target;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FTimerHandle hTrainTimer;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FTimerHandle h_RewardSetChecker;

	
public:

	/** 목표물 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TObjectPtr<class ARunner> Runner;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TObjectPtr<AAIController> RunnerController;
	
	// 회전하기 전 상태 저장
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float PrevState; 

	// 회전 후 상태 저장 
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float PostState;

	// 보상 임시저장
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float Reward_Temp;
	
};
