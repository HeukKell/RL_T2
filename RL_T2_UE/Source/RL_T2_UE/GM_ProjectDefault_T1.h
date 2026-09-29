// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "GM_ProjectDefault_T1.generated.h"

/**
 * 
 */
UCLASS()
class RL_T2_UE_API AGM_ProjectDefault_T1 : public AGameMode
{
	GENERATED_BODY()
	
public:

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void StartTrain();
	void StartTrain_Implementation();
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TObjectPtr<class AChaser> Chaser;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TObjectPtr<class ARunner> Runner;
	
};
