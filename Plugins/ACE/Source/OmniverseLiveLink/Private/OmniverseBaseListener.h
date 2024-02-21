// Copyright(c) 2022, NVIDIA CORPORATION. All rights reserved.
//
// NVIDIA CORPORATION and its licensors retain all intellectual property
// and proprietary rights in and to this software, related documentation
// and any modifications thereto.Any use, reproduction, disclosure or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA CORPORATION is strictly prohibited.

#pragma once
#include "CoreMinimal.h"
#include "HAL/Runnable.h"
#include "HAL/ThreadSafeBool.h"
#include "ILiveLinkClient.h"
#include "OmniverseLiveLinkFramePlayer.h"


class FOmniverseBaseListener : public FRunnable
{
public:
	FOmniverseBaseListener(uint32 InPort);
	virtual ~FOmniverseBaseListener();

	// Begin FRunnable Interface
	virtual void Stop() override;
	// End FRunnable Interface

	// Begin FOmniverseBaseListener Interface
	virtual void Start();
	virtual bool IsValid() const;
	virtual bool IsSocketReady() const;
	// Get the raw data from network
	virtual void OnRawDataReceived(const uint8* InReceivedData, int32 InReceivedSize);
	// Get the size-checked package
	virtual void OnPackageDataReceived(const uint8* InPackageData, int32 InPackageSize) {};
	virtual void OnPackageDataPushed(const uint8* InPackageData, int32 InPackageSize, double DeltaTime, bool bBegin = false, bool bEnd = false) {};
	virtual uint32 GetDelayTime() const { return 0; }

	virtual bool IsEOSPackage(const uint8* InPackageData, int32 InPackageSize) const;
	virtual bool IsHeaderPackage(const uint8* InPackageData, int32 InPackageSize) const { return false; }
	virtual bool GetFPSInHeader(const uint8* InPackageData, int32 InPackageSize, double& OutFPS) const { return false; }

	// End FOmniverseBaseListener Interface

	void SetClient(class ILiveLinkClient* InClient, FGuid InSourceGuid);
protected:
	// Begin FRunnable Interface
	virtual bool Init() override { return true; }
	virtual uint32 Run() override;
	virtual void Exit() override {}
	// End FRunnable Interface

	// Live link client
	class ILiveLinkClient* LiveLinkClient = nullptr;
	// Source GUID in LiveLink
	FGuid SourceGuid;

	const static FString HeaderSeparator;

private:
	void PushPackageData(const uint8* InPackageData, int32 InPackageSize);

	// Tcp Server
	class FSocket* ListenerSocket;
	class FSocket* ConnectionSocket;
	class ISocketSubsystem* SocketSubsystem;
	// Thread to run socket operations on
	class FRunnableThread* SocketThread;

	// Threadsafe Bool for terminating the main thread loop
	FThreadSafeBool ThreadStopping;

	// Buffer to receive socket data into
	// Only in socket thread
	TArray<uint8> RecvBuffer;
	TArray<uint8> IncompleteData;
	TOptional<int32> DataSizeInHeader;

	TOptional<double> CustomDeltaTime;
	TOptional<double> LastPushTime;
	bool bInBurst = false;
};
