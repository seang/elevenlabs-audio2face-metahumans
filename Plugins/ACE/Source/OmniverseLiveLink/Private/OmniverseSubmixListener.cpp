// Copyright(c) 2022-2023, NVIDIA CORPORATION. All rights reserved.
//
// NVIDIA CORPORATION and its licensors retain all intellectual property
// and proprietary rights in and to this software, related documentation
// and any modifications thereto.Any use, reproduction, disclosure or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA CORPORATION is strictly prohibited.

#include "OmniverseSubmixListener.h"
#include "AudioMixerDevice.h"
#include "AudioMixerSubmix.h"
#include "Engine/World.h"
#include "Runtime/Launch/Resources/Version.h"

#include "ACEPrivate.h"
#include "OmniverseAudioMixer.h"

#define LOCTEXT_NAMESPACE "OmniverseSubmixListener"


static TAutoConsoleVariable<int32> CVarOmniverseWaveStreamBufferSize(
	TEXT("omni.WaveStreamBufferSize"),
	1,
	TEXT("Adjusts the size of the circular audio sample buffer in MB (default is 1).\n"),
	ECVF_Default);


FOmniverseSubmixListener::FOmniverseSubmixListener()
{
	DeviceDestroyedHandle = FAudioDeviceManagerDelegates::OnAudioDeviceDestroyed.AddRaw(this, &FOmniverseSubmixListener::OnDeviceDestroyed);
}

FOmniverseSubmixListener::~FOmniverseSubmixListener()
{
	FAudioDeviceManagerDelegates::OnAudioDeviceDestroyed.Remove(DeviceDestroyedHandle);
}

// Register with audio device my submix listener
void FOmniverseSubmixListener::Activate()
{
	if (FAudioDeviceManager* AudioDeviceManager = FAudioDeviceManager::Get())
	{
		FOmniverseAudioMixerModule* AudioModule = &FModuleManager::GetModuleChecked<FOmniverseAudioMixerModule>("OmniverseAudioMixer");
		if (AudioModule)
		{
			// Audio device should be created in game thread, same as submix, so make sure audio thread is stopped.
			FAudioThread::StopAudioThread();

			AudioModule->SetPlatformInterface(AudioDeviceManager->GetAudioDeviceModule()->CreateAudioMixerPlatformInterface());
			AudioModule->SetSampleRate(SubmixSampleRate);
			FAudioDeviceParams MainDeviceParams;
			MainDeviceParams.Scope = EAudioDeviceScope::Shared;
			MainDeviceParams.bIsNonRealtime = false;
			MainDeviceParams.AssociatedWorld = GWorld;
			MainDeviceParams.AudioModule = AudioModule;
			AudioDeviceHandle = AudioDeviceManager->RequestAudioDevice(MainDeviceParams);

			// Device is created, start audio thread
			FAudioThread::StartAudioThread();

			if (AudioDeviceHandle.IsValid())
			{
				FAudioDevice* AudioDevice = AudioDeviceHandle.GetAudioDevice();
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 3
				if (AudioDevice)
#else
				if (AudioDevice && AudioDevice->IsAudioMixerEnabled())
#endif
				{
					Audio::FMixerDevice* MixerDevice = static_cast<Audio::FMixerDevice*>(AudioDevice);
					if (MixerDevice)
					{
						auto MixerSubmixPtr = MixerDevice->GetMasterSubmix().Pin();
						if (MixerSubmixPtr.IsValid())
						{
							MixerDevice->AudioRenderThreadCommand([MixerSubmixPtr]()
								{
									MixerSubmixPtr->SetAutoDisable(false);
								});
						}
					}
				}
				AudioDeviceHandle->RegisterSubmixBufferListener(this);
			}
		}
	}

	bSubmixActivated = true;
}

void FOmniverseSubmixListener::Deactivate()
{
	bSubmixActivated = false;

	if (FAudioDeviceManager* AudioDeviceManager = FAudioDeviceManager::Get())
	{
		if (AudioDeviceHandle.IsValid())
		{
			AudioDeviceHandle->UnregisterSubmixBufferListener(this);

			// AudioDeviceHandle needs to be released without Audio thread
			FAudioThread::StopAudioThread();
			AudioDeviceHandle.Reset();
			FAudioThread::StartAudioThread();
		}
	}
}

void FOmniverseSubmixListener::OnDeviceDestroyed(Audio::FDeviceId InDeviceId)
{
	if (AudioDeviceHandle.IsValid() && InDeviceId == AudioDeviceHandle.GetDeviceID())
	{
		// AudioDeviceHandle needs to be released without Audio thread
		FAudioThread::StopAudioThread();
		AudioDeviceHandle.Reset();
		FAudioThread::StartAudioThread();

		FAudioDeviceManagerDelegates::OnAudioDeviceDestroyed.Remove(DeviceDestroyedHandle);
	}
}

void FOmniverseSubmixListener::AddNewWave(const FOmniverseWaveFormatInfo& Format)
{
	int32 BufferMB = CVarOmniverseWaveStreamBufferSize.GetValueOnAnyThread();
	auto NewStream = MakeShareable(new FWaveStream(Format, BufferMB * 1024 * 1024));
	
	if (!PlayingStream.IsValid())
	{
		PlayingStream = NewStream;
	}

	if (!LastPlayingStream.IsValid())
	{
		LastPlayingStream = PlayingStream;
	}
	else
	{
		LastPlayingStream.Pin()->NextStream = NewStream;
		LastPlayingStream = LastPlayingStream.Pin()->NextStream;
	}
}

void FOmniverseSubmixListener::AppendStream(const uint8* Data, int32 Size)
{
	auto LastStream = LastPlayingStream;

	if (LastStream.IsValid())
	{
		FOmniverseWaveFormatInfo& WaveFormat = LastStream.Pin()->WaveFormat;
		if ((WaveFormat.SampleType == 1 || WaveFormat.SampleType == 3) 
		&& (WaveFormat.NumChannels == 1 || WaveFormat.NumChannels == 2))
		{
			// Always append to the last stream
			int32 PushSize = LastStream.Pin()->LocklessStreamBuffer.Push(Data, Size);
			if (PushSize != Size)
			{
				// Alloc new buffer
				AddNewWave(WaveFormat);
				AppendStream(Data + PushSize, Size - PushSize);
			}
		}
	}

	TrySwitchToNextStream();
}

void FOmniverseSubmixListener::TrySwitchToNextStream()
{
	// Clean the stream buffer if it's empty
	auto CurrentStream = PlayingStream;
	if (CurrentStream.IsValid() && CurrentStream->NextStream != nullptr && !CurrentStream->HasStream())
	{
		UE_LOG(LogACE, Log, TEXT("Clean the stream buffer."));
		auto Temp = CurrentStream->NextStream;
		CurrentStream->NextStream = nullptr;
		PlayingStream = Temp;
	}
}

// ISubmixBufferListener
// when called, submit samples to audio device in OnNewSubmixBuffer
void FOmniverseSubmixListener::OnNewSubmixBuffer(const USoundSubmix* OwningSubmix, float* AudioData, int32 NumSamples, int32 NumChannels, const int32 SampleRate, double AudioClock)
{
	auto GetFloatSample = [](uint8* Buffer, int32 StartIndex, int32 BufferSize, int32 BitsPerSample, int32 SampleType, float& Out)
	{
		if (BitsPerSample == 32 && SampleType == 3)
		{
			if (StartIndex + 3 >= BufferSize)
			{
				return false;
			}
			// IEEE Float
			float* Sample = reinterpret_cast<float*>(Buffer);
			Out = (*Sample);
			return true;
		}
		else if (BitsPerSample == 64 && SampleType == 3)
		{
			if (StartIndex + 7 >= BufferSize)
			{
				return false;
			}

			// IEEE Float
			double* Sample = reinterpret_cast<double*>(Buffer);
			Out = (float)(*Sample);
			return true;
		}
		else if (BitsPerSample == 16 && SampleType == 1)
		{
			if (StartIndex + 1 >= BufferSize)
			{
				return false;
			}

			// convert 16bit pcm to float
			int16* Sample = reinterpret_cast<int16*>(Buffer);
			Out = (float)(*Sample) / ((*Sample) >= 0 ? (float)MAX_int16 : ((float)MAX_int16 + 1));
			return true;
		}
		else if (BitsPerSample == 8 && SampleType == 1)
		{
			if (StartIndex >= BufferSize)
			{
				return false;
			}

			// convert 8bit pcm to float
			int8* Sample = reinterpret_cast<int8*>(Buffer);
			Out = (float)(*Sample) / ((*Sample) >= 0 ? (float)MAX_int8 : ((float)MAX_int8 + 1));
			return true;
		}

		return false;
	};

	auto CurrentStream = PlayingStream;
	if (bSubmixActivated && CurrentStream.IsValid())
	{
		if (CurrentStream->HasStream())
		{
			// Use WaveFormat struct to do all of the int-float + resampling
			FOmniverseWaveFormatInfo& WaveFormat = CurrentStream->WaveFormat;
			int32 Stride = WaveFormat.BitsPerSample / 8;
			// Sample numbers of steam

			int32 StreamSamples = CurrentStream->LocklessStreamBuffer.Num() / Stride;
			if (WaveFormat.NumChannels == 1)
			{
				StreamSamples *= NumChannels;
			}
			// if stereo wave didn't contain the even number of wave samples, fix it 
			else if (StreamSamples % 2 == 1)
			{
				StreamSamples++;
			}

			// Get the minimal sample number
			int32 PlaySamples = FMath::Min(NumSamples, StreamSamples);
			auto AllocPopBuffer = [Stride, NumChannels](TArray<uint8>& Buffer, int32 AllocSamples, int WaveChannels)
			{
				int32 BufferSize = AllocSamples * Stride;
				// Mono wave just need half samples
				if (WaveChannels == 1)
				{
					BufferSize /= NumChannels;
				}
				Buffer.AddUninitialized(BufferSize);
			};
			
			// Check if next stream can fill in
			int32 NextFetchSamples = 0;
			if (NumSamples - PlaySamples > 0 && CurrentStream->NextStream.IsValid())
			{
				auto NextStream = CurrentStream->NextStream.Get();
				if (NextStream->WaveFormat == WaveFormat)
				{
					int32 NextStreamSamples = NextStream->LocklessStreamBuffer.Num() / Stride;
					if (WaveFormat.NumChannels == 1)
					{
						NextStreamSamples *= NumChannels;
					}

					// Get the size which needs to fetch from next stream
					NextFetchSamples = FMath::Min(NumSamples - PlaySamples, NextStreamSamples);
					// New play samples
					PlaySamples += NextFetchSamples;
				}
			}

			if (PlaySamples > 0)
			{
#if 0
				if (WaveFormat.SamplesPerSecond != SampleRate)
				{
					// To get the real size to resample
					Audio::FAlignedFloatBuffer PreResamplerInputData;
					PreResamplerInputData.AddUninitialized(NumSamples);
					Audio::FResamplingParameters PreResamplerParameters = {
						Audio::EResamplingMethod::BestSinc,
						NumChannels,
						SampleRate,
						WaveFormat.SamplesPerSecond,
						PreResamplerInputData
					};
					int32 ResamplerSamples = FMath::Min(PlaySamples, Audio::GetOutputBufferSize(PreResamplerParameters));

					// Use this real size to get the buffer to sampler
					Audio::FAlignedFloatBuffer ResamplerInputData;
					ResamplerInputData.AddUninitialized(ResamplerSamples);

					TArray<uint8> Buffer;
					AllocPopBuffer(Buffer, ResamplerSamples, WaveFormat.NumChannels);

					int32 PopSize = CurrentStream->LocklessStreamBuffer.Pop(Buffer.GetData(), Buffer.Num());

					if (PopSize < Buffer.Num() && CurrentStream->NextStream.IsValid())
					{
						auto NextStream = CurrentStream->NextStream.Get();
						if (NextStream->WaveFormat == WaveFormat)
						{
							NextStream->LocklessStreamBuffer.Pop(Buffer.GetData() + PopSize, Buffer.Num() - PopSize);
						}
					}

					for (int32 SampleIndex = 0, SourceSampleIndex = 0; SampleIndex < ResamplerSamples; SampleIndex += NumChannels)
					{
						for (int32 Channel = 0; Channel < NumChannels; ++Channel)
						{
							float Out;
							int32 BufferIdx = SourceSampleIndex * Stride;
							if (BufferIdx < Buffer.Num() && GetFloatSample(&Buffer[BufferIdx], BufferIdx, Buffer.Num(), WaveFormat.BitsPerSample, WaveFormat.SampleType, Out))
							{
								ResamplerInputData[SampleIndex + Channel] = Out;
							}
							else
							{
								ResamplerInputData[SampleIndex + Channel] = 0.0f;
							}

							// Mono wave need duplicate the sample
							if (WaveFormat.NumChannels < NumChannels)
							{
								if (Channel == WaveFormat.NumChannels)
								{
									SourceSampleIndex++;
								}
							}
							else
							{
								SourceSampleIndex++;
							}
						}
					}

					Audio::FResamplingParameters ResamplerParameters = {
						Audio::EResamplingMethod::BestSinc,
						NumChannels,
						WaveFormat.SamplesPerSecond,
						SampleRate,
						ResamplerInputData
					};

					Audio::FAlignedFloatBuffer ResamplerOutputData;
					// This buffer size should be close or equal to NumSamples
					ResamplerOutputData.AddUninitialized(Audio::GetOutputBufferSize(ResamplerParameters));
					Audio::FResamplerResults ResamplerResults;
					ResamplerResults.OutBuffer = &ResamplerOutputData;

					bool bResampleResult = Audio::Resample(ResamplerParameters, ResamplerResults);
					if (bResampleResult)
					{
						int32 NumResamples = FMath::Min(ResamplerOutputData.Num(), NumSamples);
						for (int32 SampleIndex = 0; SampleIndex < NumResamples; SampleIndex += NumChannels)
						{
							for (int32 Channel = 0; Channel < NumChannels; ++Channel)
							{
								AudioData[SampleIndex + Channel] += ResamplerOutputData[SampleIndex + Channel];
							}
						}
					}
				}
				else
#endif
				{
					TArray<uint8> Buffer;
					AllocPopBuffer(Buffer, PlaySamples, WaveFormat.NumChannels);

					int32 PopSize = CurrentStream->LocklessStreamBuffer.Pop(Buffer.GetData(), Buffer.Num());
					// Fill in buffer from next stream if it's available
					if (PopSize < Buffer.Num() && CurrentStream->NextStream.IsValid() && NextFetchSamples > 0)
					{
						CurrentStream->NextStream.Get()->LocklessStreamBuffer.Pop(Buffer.GetData() + PopSize, Buffer.Num() - PopSize);
					}

					for (int32 SampleIndex = 0, SourceSampleIndex = 0; SampleIndex < PlaySamples; SampleIndex += NumChannels)
					{
						for (int32 Channel = 0; Channel < NumChannels; ++Channel)
						{
							float Out;
							int32 BufferIdx = SourceSampleIndex * Stride;
							if (BufferIdx < Buffer.Num() && GetFloatSample(&Buffer[BufferIdx], BufferIdx, Buffer.Num(), WaveFormat.BitsPerSample, WaveFormat.SampleType, Out))
							{
								AudioData[SampleIndex + Channel] += Out;
							}

							// Mono wave need duplicate the sample
							if (WaveFormat.NumChannels < NumChannels)
							{
								if (Channel == WaveFormat.NumChannels)
								{
									SourceSampleIndex++;
								}
							}
							else
							{
								SourceSampleIndex++;
							}
						}
					}
				}
			}
		}

		TrySwitchToNextStream();
	}
}

#undef LOCTEXT_NAMESPACE
