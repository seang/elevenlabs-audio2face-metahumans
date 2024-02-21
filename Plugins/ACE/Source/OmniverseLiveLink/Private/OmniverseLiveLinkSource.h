// Copyright(c) 2022-2023, NVIDIA CORPORATION. All rights reserved.
//
// NVIDIA CORPORATION and its licensors retain all intellectual property
// and proprietary rights in and to this software, related documentation
// and any modifications thereto.Any use, reproduction, disclosure or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA CORPORATION is strictly prohibited.

#pragma once
#include "CoreMinimal.h"
#include "ILiveLinkSource.h"
#include "Tickable.h"
#include "Interfaces/IPv4/IPv4Address.h"

class FOmniverseLiveLinkSource : public ILiveLinkSource
{
public:
    FOmniverseLiveLinkSource(uint32 InPort, uint32 InAudioPort, uint32 InSampleRate);
    virtual ~FOmniverseLiveLinkSource();

    // Begin ILiveLinkSource Interface
    virtual void ReceiveClient( ILiveLinkClient* InClient, FGuid InSourceGuid ) override;
    virtual bool IsSourceStillValid() const override;
    virtual bool RequestSourceShutdown() override;
    virtual FText GetSourceType() const override;
	virtual FText GetSourceMachineName() const override;
	virtual FText GetSourceStatus() const override;
	virtual TSubclassOf<ULiveLinkSourceSettings> GetSettingsClass() const override;
    // End ILiveLinkSource Interface

private:
	void Start();
	void Stop();

private:
    
	TSharedPtr<class FOmniverseWaveStreamer, ESPMode::ThreadSafe> WaveStreamer;
	TSharedPtr<class FOmniverseLiveLinkListener, ESPMode::ThreadSafe> LiveLinkListener;

	FText SourceStatus;
};
