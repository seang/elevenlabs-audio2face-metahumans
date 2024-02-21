// Copyright(c) 2022-2023, NVIDIA CORPORATION. All rights reserved.
//
// NVIDIA CORPORATION and its licensors retain all intellectual property
// and proprietary rights in and to this software, related documentation
// and any modifications thereto.Any use, reproduction, disclosure or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA CORPORATION is strictly prohibited.

#pragma once
#include "CoreMinimal.h"
#include "AudioDevice.h"
#include "ISubmixBufferListener.h"
#include "OmniverseWaveDef.h"

class FOmniverseSubmixListener : public ISubmixBufferListener
{
public:
	FOmniverseSubmixListener();
	virtual ~FOmniverseSubmixListener();

	// Register with audio device my submix listener
	void Activate();
	void Deactivate();

	void SetSampleRate(uint32 InSampleRate)
	{
		SubmixSampleRate = InSampleRate;
	}

	void AddNewWave(const FOmniverseWaveFormatInfo& Format);
	void AppendStream(const uint8* Data, int32 Size);

protected:
	// ISubmixBufferListener
	// when called, submit samples to audio device in OnNewSubmixBuffer
	virtual void OnNewSubmixBuffer(const USoundSubmix* OwningSubmix, float* AudioData, int32 NumSamples, int32 NumChannels, const int32 SampleRate, double AudioClock) override;
	
private:
	struct FWaveStream
	{
		FWaveStream(const FOmniverseWaveFormatInfo& NewWaveFormat, uint32 Capacity)
			: WaveFormat(NewWaveFormat)
			, NextStream(nullptr)
		{
			LocklessStreamBuffer.SetCapacity(Capacity);
		}

		bool HasStream() { return LocklessStreamBuffer.Num() > 0; }

		FOmniverseWaveFormatInfo WaveFormat;
		mutable Audio::TCircularAudioBuffer<uint8> LocklessStreamBuffer;
		TSharedPtr<FWaveStream> NextStream;
	};

	void OnDeviceDestroyed(Audio::FDeviceId InDeviceId);
	void TrySwitchToNextStream();

	// Valid in Audio thread
	TSharedPtr<FWaveStream, ESPMode::ThreadSafe> PlayingStream = nullptr;
	TWeakPtr<FWaveStream, ESPMode::ThreadSafe> LastPlayingStream = nullptr;

	FThreadSafeBool bSubmixActivated = false;
	FAudioDeviceHandle AudioDeviceHandle;
	FDelegateHandle DeviceDestroyedHandle;
	uint32 SubmixSampleRate = 16000;
};
