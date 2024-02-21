// Copyright(c) 2022, NVIDIA CORPORATION. All rights reserved.
//
// NVIDIA CORPORATION and its licensors retain all intellectual property
// and proprietary rights in and to this software, related documentation
// and any modifications thereto.Any use, reproduction, disclosure or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA CORPORATION is strictly prohibited.

#pragma once
#include "CoreMinimal.h"
#include "OmniverseWaveDef.h"
#include "OmniverseBaseListener.h"

class FOmniverseWaveStreamer : public FOmniverseBaseListener
{
public:
	FOmniverseWaveStreamer(uint32 InPort, uint32 InSampleRate);
    virtual ~FOmniverseWaveStreamer();

	virtual void Stop() override;
	virtual void Start() override;
	virtual void OnPackageDataReceived(const uint8* InPackageData, int32 InPackageSize) override;
	virtual void OnPackageDataPushed(const uint8* InPackageData, int32 InPackageSize, double DeltaTime, bool bBegin = false, bool bEnd = false) override;
	virtual uint32 GetDelayTime() const override;
	virtual bool IsHeaderPackage(const uint8* InPackageData, int32 InPackageSize) const override;

private:
    void ParseWave(const uint8* InReceivedData, int32 InReceivedSize);

private:

	TSharedPtr<class FOmniverseSubmixListener> SubmixListener;
};
