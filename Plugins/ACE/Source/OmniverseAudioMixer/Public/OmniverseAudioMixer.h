// Copyright(c) 2022, NVIDIA CORPORATION. All rights reserved.
//
// NVIDIA CORPORATION and its licensors retain all intellectual property
// and proprietary rights in and to this software, related documentation
// and any modifications thereto.Any use, reproduction, disclosure or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA CORPORATION is strictly prohibited.

#pragma once
#include "AudioMixer.h"
#include "AudioMixerDevice.h"
#include "Modules/ModuleManager.h"

class OMNIVERSEAUDIOMIXER_API FOmniverseAudioMixerModule : public IAudioDeviceModule
{
public:
	virtual bool IsAudioMixerModule() const override { return true; }

	void SetPlatformInterface(Audio::IAudioMixerPlatformInterface* Interface);

	void SetSampleRate(uint32 InSampleRate) { SampleRate = InSampleRate; }

	virtual FAudioDevice* CreateAudioDevice() override;
	

private:
	Audio::IAudioMixerPlatformInterface* AudioMixerPlatformInterface = nullptr;
	uint32 SampleRate = 16000;
};
