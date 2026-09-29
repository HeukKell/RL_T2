// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "EngineMinimal.h"

DECLARE_LOG_CATEGORY_EXTERN(ProjectLog, Log, All)

#define PLOG(Verbosity, Message, ...) UE_LOG(ProjectLog, Verbosity, TEXT("%s(%d) >> %s"),ANSI_TO_TCHAR(__FILE__), __LINE__, *FString::Printf(Message, ##__VA_ARGS__))
