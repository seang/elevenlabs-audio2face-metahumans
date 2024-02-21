// Copyright(c) 2022, NVIDIA CORPORATION. All rights reserved.
//
// NVIDIA CORPORATION and its licensors retain all intellectual property
// and proprietary rights in and to this software, related documentation
// and any modifications thereto.Any use, reproduction, disclosure or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA CORPORATION is strictly prohibited.

#pragma once

#include "CoreMinimal.h"
#include "LiveLinkSourceSettings.h"
#include "OmniverseLiveLinkSourceSettings.generated.h"

/** Class for Omniverse live link source settings */
UCLASS()
class UOmniverseLiveLinkSourceSettings : public ULiveLinkSourceSettings
{
public:
	GENERATED_BODY()
	/**  Milliseconds delay to wait before playing blendshapes animation. */
	UPROPERTY(EditAnywhere, Category = "Settings", meta = (ClampMin = 0, ClampMax = 1000))
	uint32 AnimationDelayTime = 150;

	/**  Milliseconds delay to wait before playing audio. */
	UPROPERTY(EditAnywhere, Category = "Settings", meta = (ClampMin = 0, ClampMax = 1000))
	uint32 AudioDelayTime = 0;

};
