// Fill out your copyright notice in the Description page of Project Settings.


#include "Runner.h"
#include "Math/UnrealMathUtility.h"

// Sets default values
ARunner::ARunner()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ARunner::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ARunner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void ARunner::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

bool ARunner::FindPosition(AActor* PivotActor, float Radius, FVector& Position)
{
	if (nullptr != PivotActor) {
		
		if (Radius < 150.0f) {
			Radius = 150.0f;
		}

		// 랜덤한 방향 벡터를 얻는다.

		float RandomAngle = FMath::FRand() * PI * 2; 
		FVector DirectionalVector_Rand = FVector(FMath::Cos(RandomAngle), FMath::Sin(RandomAngle), 0.0f);
		float VectorLength_Clamped = FMath::Clamp<float>(FMath::FRand() * Radius, 100.0f, Radius);
		DirectionalVector_Rand = VectorLength_Clamped * DirectionalVector_Rand;

		Position = PivotActor->GetActorLocation() + DirectionalVector_Rand;
		return true;
	}

	return false;
}

