// Publisher: Fullike (https://github.com/fullike)
// Copyright 2024. All Rights Reserved.

#include "RoadBuilder.h"
#include "UObject/CoreRedirects.h"
#include "HAL/PlatformTime.h"

IMPLEMENT_MODULE(FRoadBuilder, RoadBuilder)
void FRoadBuilder::StartupModule()
{
}

void FRoadBuilder::ShutdownModule()
{
}

DEFINE_LOG_CATEGORY(LogRoadBuilder);
FCustomCycleCounter::FCustomCycleCounter(const FString& InTag):Tag(InTag)
{
	StartTime = FPlatformTime::Seconds();
}

FCustomCycleCounter::~FCustomCycleCounter()
{
	double ElapsedTime = FPlatformTime::Seconds() - StartTime;
	UE_LOG(LogRoadBuilder, Log, TEXT("Cost %lf seconds to execute %s"), ElapsedTime, *Tag);
}