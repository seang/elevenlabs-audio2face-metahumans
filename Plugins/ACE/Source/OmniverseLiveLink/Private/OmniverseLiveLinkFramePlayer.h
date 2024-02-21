// Copyright(c) 2022, NVIDIA CORPORATION. All rights reserved.
//
// NVIDIA CORPORATION and its licensors retain all intellectual property
// and proprietary rights in and to this software, related documentation
// and any modifications thereto.Any use, reproduction, disclosure or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA CORPORATION is strictly prohibited.

#pragma once
#include "CoreMinimal.h"
#include "Containers/Queue.h"
#include "HAL/Runnable.h"
#include "HAL/ThreadSafeBool.h"
#include <string>

struct FPendBuffer
{
	// use std::string to avoid TChar to uint8 conversion
	std::string Buffer;
	double DeltaPendingTime = 0.0;
	bool BeginFence = false;
	bool EndFence = false;
};

class FOmniverseLiveLinkFramePlayer : public FRunnable
{
public:
	DECLARE_DELEGATE_TwoParams(FOnFramePlayed, const uint8*, int32);
public:
	FOmniverseLiveLinkFramePlayer();
	virtual ~FOmniverseLiveLinkFramePlayer();

	// Begin FRunnable Interface
	virtual void Stop() override;
	// End FRunnable Interface

	void Start();
	void Reset();

	void PushAnimeData_AnyThread(const uint8* InData, int32 InSize, double DeltaTime, bool bBegin, bool bEnd);
	void PushAudioData_AnyThread(const uint8* InData, int32 InSize, double DeltaTime, bool bBegin, bool bEnd);

	void RegisterAnime(TSharedPtr<class FOmniverseBaseListener, ESPMode::ThreadSafe> Listener);
	void RegisterAudio(TSharedPtr<class FOmniverseBaseListener, ESPMode::ThreadSafe> Listener);


	static FOmniverseLiveLinkFramePlayer& Get();

protected:
	// Begin FRunnable Interface
	virtual bool Init() override { return true; }
	virtual uint32 Run() override;
	virtual void Exit() override {}
	// End FRunnable Interface

private:
	void PlayAudio(double CurrentTime);
	void PlayAnime(double CurrentTime);

	// Thread to run work operations on
	class FRunnableThread* Thread;

	// Threadsafe Bool for terminating the main thread loop
	FThreadSafeBool ThreadStopping;

	TQueue<FPendBuffer> AudioPendBuffer;
	TQueue<FPendBuffer> AnimePendBuffer;

	double LastAnimePlayTime = 0.0;
	double LastAudioPlayTime = 0.0;


	static TUniquePtr< class FOmniverseLiveLinkFramePlayer > Instance;

	TOptional<FPendBuffer> CurrentAudio;
	TOptional<FPendBuffer> CurrentAnime;

	uint8 Fence = UINT8_MAX;
	FThreadSafeBool ThreadReset;

	TSharedPtr<class FOmniverseBaseListener, ESPMode::ThreadSafe> AnimeListener;
	TSharedPtr<class FOmniverseBaseListener, ESPMode::ThreadSafe> AudioListener;
};
