// Fill out your copyright notice in the Description page of Project Settings.


#include "GM_ProjectDefault_T1.h"
#include "Runner.h"
#include "Chaser.h"

void AGM_ProjectDefault_T1::StartTrain_Implementation()
{
	UE_LOG(LogTemp, Log, TEXT("StartTrain"));
	GEngine->AddOnScreenDebugMessage(1, 5.0f, FColor::Blue, TEXT("Start Train"));


	// 먼저 Runner 를 이동

	if (nullptr == Runner || nullptr == Chaser) {
		
		GEngine->AddOnScreenDebugMessage(1, 5.0f, FColor::Blue, TEXT("No Actor"));
		return;
	}

	Runner->Run();

}
