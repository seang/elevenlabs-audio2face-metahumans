// Copyright(c) 2022-2023, NVIDIA CORPORATION. All rights reserved.
//
// NVIDIA CORPORATION and its licensors retain all intellectual property
// and proprietary rights in and to this software, related documentation
// and any modifications thereto.Any use, reproduction, disclosure or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA CORPORATION is strictly prohibited.

#include "OmniverseLiveLinkFramePlayer.h"
#include "Async/Async.h"
#include "HAL/Event.h"
#include "HAL/RunnableThread.h"
#include "GenericPlatform/GenericPlatformProcess.h"
#include "GenericPlatform/GenericPlatformTime.h"
#include "OmniverseBaseListener.h"


TUniquePtr< FOmniverseLiveLinkFramePlayer > FOmniverseLiveLinkFramePlayer::Instance;

FOmniverseLiveLinkFramePlayer::FOmniverseLiveLinkFramePlayer()
	: Thread(nullptr)
	, ThreadStopping(false)
	, ThreadReset(false)
	, AnimeListener(nullptr)
	, AudioListener(nullptr)
{
	
}

FOmniverseLiveLinkFramePlayer::~FOmniverseLiveLinkFramePlayer()
{
	Stop();

	if (Thread)
	{
		Thread->WaitForCompletion();
		delete Thread;
	}
}

void FOmniverseLiveLinkFramePlayer::Start()
{
	if (Thread == nullptr)
	{
		FString ThreadName = TEXT("Omniverse LiveLink Replayer ");
		ThreadName.AppendInt(FAsyncThreadIndex::GetNext());
		Thread = FRunnableThread::Create(this, *ThreadName, 128 * 1024, TPri_AboveNormal, FPlatformAffinity::GetPoolThreadMask());
	}
	ThreadStopping = false;
}

void FOmniverseLiveLinkFramePlayer::Reset()
{
	AudioPendBuffer.Empty();
	AnimePendBuffer.Empty();
	ThreadReset = true;
	// NOTE:
	// Don't reset AnimeListener and AudioListener here, because they're still be used in thread.
	// They'll be released when the new listeners come
}

void FOmniverseLiveLinkFramePlayer::Stop()
{
	ThreadStopping = true;
}

void FOmniverseLiveLinkFramePlayer::RegisterAnime(TSharedPtr<class FOmniverseBaseListener, ESPMode::ThreadSafe> Listener)
{
	AnimeListener = Listener;
}

void FOmniverseLiveLinkFramePlayer::RegisterAudio(TSharedPtr<class FOmniverseBaseListener, ESPMode::ThreadSafe> Listener)
{
	AudioListener = Listener;
}

void FOmniverseLiveLinkFramePlayer::PushAudioData_AnyThread(const uint8* InData, int32 InSize, double DeltaTime, bool bBegin, bool bEnd)
{
	AudioPendBuffer.Enqueue({ std::string((char*)InData, InSize), DeltaTime, bBegin, bEnd });
}

void FOmniverseLiveLinkFramePlayer::PushAnimeData_AnyThread(const uint8* InData, int32 InSize, double DeltaTime, bool bBegin, bool bEnd)
{
	AnimePendBuffer.Enqueue({ std::string((char*)InData, InSize), DeltaTime, bBegin, bEnd });
}

void FOmniverseLiveLinkFramePlayer::PlayAudio(double CurrentTime)
{
	if (AudioListener)
	{
		AudioListener->OnPackageDataReceived((uint8*)CurrentAudio.GetValue().Buffer.data(), CurrentAudio.GetValue().Buffer.size());
	}
	CurrentAudio.Reset();
	LastAudioPlayTime = CurrentTime;
}

void FOmniverseLiveLinkFramePlayer::PlayAnime(double CurrentTime)
{
	if (AnimeListener)
	{
		AnimeListener->OnPackageDataReceived((uint8*)CurrentAnime.GetValue().Buffer.data(), CurrentAnime.GetValue().Buffer.size());
	}
	CurrentAnime.Reset();
	LastAnimePlayTime = CurrentTime;
}

uint32 FOmniverseLiveLinkFramePlayer::Run()
{
	while (!ThreadStopping)
	{
		if (ThreadStopping)
		{
			break;
		}

		if (ThreadReset)
		{
			CurrentAudio.Reset();
			CurrentAnime.Reset();
			ThreadReset = false;
		}

		if (!CurrentAudio.IsSet())
		{
			FPendBuffer DequeueData;
			if (AudioPendBuffer.Dequeue(DequeueData))
			{
				CurrentAudio = DequeueData;
			}
		}

		if (!CurrentAnime.IsSet())
		{
			FPendBuffer DequeueData;
			if (AnimePendBuffer.Dequeue(DequeueData))
			{
				CurrentAnime = DequeueData;
			}
		}

		double CurrentTime = FPlatformTime::Seconds();
		if (CurrentAudio.IsSet() && CurrentTime - LastAudioPlayTime >= CurrentAudio.GetValue().DeltaPendingTime)
		{
			if (CurrentAudio.GetValue().BeginFence)
			{
				Fence &= 0xFE;
			}

			bool IsAvailable = false;
			if (CurrentAudio.GetValue().EndFence)
			{
				Fence |= 0x1;
				if (Fence == UINT8_MAX)
				{
					IsAvailable = true;
				}
			}
			else
			{
				IsAvailable = true;
			}

			if (IsAvailable)
			{
				PlayAudio(CurrentTime);
			}
		}

		if (CurrentAnime.IsSet() && CurrentTime - LastAnimePlayTime >= CurrentAnime.GetValue().DeltaPendingTime)
		{
			if (CurrentAnime.GetValue().BeginFence)
			{
				Fence &= 0xFD;
			}

			bool IsAvailable = false;
			if (CurrentAnime.GetValue().EndFence)
			{
				Fence |= 0x2;
				if (Fence == UINT8_MAX)
				{
					IsAvailable = true;
				}
			}
			else
			{
				IsAvailable = true;
			}

			if (IsAvailable)
			{
				PlayAnime(CurrentTime);
			}
		}
	}
	return 0;
}

FOmniverseLiveLinkFramePlayer& FOmniverseLiveLinkFramePlayer::Get()
{
	if (!Instance.IsValid())
	{
		Instance = MakeUnique<FOmniverseLiveLinkFramePlayer>();
	}
	return *Instance;
}
