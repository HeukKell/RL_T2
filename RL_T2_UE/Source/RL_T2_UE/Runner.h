// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Runner.generated.h"

UCLASS()
class RL_T2_UE_API ARunner : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ARunner();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

public: 

	// 추적자
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TObjectPtr<AActor> Chaser;

public:

	/**
		특정 액터를 중심으로 반경내 적절한 위치를 찾는 함수
		@param PivotActor : 위치를 찾기위한 중심 액터
		@param Radius 반경.최소 150.0f 이상
		@param Positioin 찾아낸 위치
	*/
	UFUNCTION(BlueprintCallable)
	bool FindPosition(AActor* PivotActor, float Radius, FVector& Position);

	/**
		특정 위치로 도망가는 함수
		(Blueprint 에서 구현)
	*/
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void Run();
};
