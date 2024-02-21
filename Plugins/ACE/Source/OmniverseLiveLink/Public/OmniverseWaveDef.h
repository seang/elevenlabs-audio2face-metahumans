// Copyright(c) 2022, NVIDIA CORPORATION. All rights reserved.
//
// NVIDIA CORPORATION and its licensors retain all intellectual property
// and proprietary rights in and to this software, related documentation
// and any modifications thereto.Any use, reproduction, disclosure or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA CORPORATION is strictly prohibited.

#pragma once
#include "CoreMinimal.h"

struct FOmniverseWaveFormatInfo
{
	static const int32 NumMembers = 4;
	int32 SamplesPerSecond;
	int32 NumChannels;
	int32 BitsPerSample;
	int32 SampleType; // 1-PCM, 3-float
};

inline bool operator==(const FOmniverseWaveFormatInfo& F1, const FOmniverseWaveFormatInfo& F2)
{
	return F1.SamplesPerSecond && F2.SamplesPerSecond 
		&& F1.NumChannels && F2.NumChannels
		&& F1.BitsPerSample && F2.BitsPerSample
		&& F1.SampleType && F2.SampleType;
}
