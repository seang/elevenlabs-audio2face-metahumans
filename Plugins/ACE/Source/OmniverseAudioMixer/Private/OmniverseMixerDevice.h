// Copyright(c) 2022, NVIDIA CORPORATION. All rights reserved.
//
// NVIDIA CORPORATION and its licensors retain all intellectual property
// and proprietary rights in and to this software, related documentation
// and any modifications thereto.Any use, reproduction, disclosure or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA CORPORATION is strictly prohibited.

#pragma once
#include "AudioMixerDevice.h"

namespace Audio
{
	class FOmniverseMixerDevice : public FMixerDevice
	{
	public:
		FOmniverseMixerDevice(IAudioMixerPlatformInterface* InAudioMixerPlatform, uint32 InSampleRate);

		virtual FAudioPlatformSettings GetPlatformSettings() const override;

	private:
		uint32 SampleRate;
	};
}

