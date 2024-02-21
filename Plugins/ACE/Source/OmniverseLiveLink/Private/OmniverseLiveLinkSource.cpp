// Copyright(c) 2022-2023, NVIDIA CORPORATION. All rights reserved.
//
// NVIDIA CORPORATION and its licensors retain all intellectual property
// and proprietary rights in and to this software, related documentation
// and any modifications thereto.Any use, reproduction, disclosure or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA CORPORATION is strictly prohibited.

#include "OmniverseLiveLinkSource.h"
#include "OmniverseWaveStreamer.h"
#include "OmniverseLiveLinkListener.h"
#include "OmniverseLiveLinkSourceSettings.h"

#include "ILiveLinkClient.h"
#include "Logging/LogMacros.h"
#include "Async/Async.h"

#define LOCTEXT_NAMESPACE "OmniverseLiveLinkSource"

FOmniverseLiveLinkSource::FOmniverseLiveLinkSource(uint32 InPort, uint32 InAudioPort, uint32 InSampleRate)
{
	SourceStatus = LOCTEXT("OmniverseLiveLinkSource", "Device Not Found");
	FOmniverseLiveLinkFramePlayer::Get().Start();
	WaveStreamer = MakeShareable(new FOmniverseWaveStreamer(InAudioPort, InSampleRate));
	LiveLinkListener = MakeShareable(new FOmniverseLiveLinkListener(InPort));

	FOmniverseLiveLinkFramePlayer::Get().RegisterAnime(LiveLinkListener);
	FOmniverseLiveLinkFramePlayer::Get().RegisterAudio(WaveStreamer);

	if (LiveLinkListener->IsSocketReady() && WaveStreamer->IsSocketReady())
	{
		Start();
		SourceStatus = LOCTEXT("OmniverseLiveLinkSource", "Active");
	}
}

FOmniverseLiveLinkSource::~FOmniverseLiveLinkSource()
{
	FOmniverseLiveLinkFramePlayer::Get().Reset();
    Stop();

	WaveStreamer.Reset();
	LiveLinkListener.Reset();
}

TSubclassOf<ULiveLinkSourceSettings> FOmniverseLiveLinkSource::GetSettingsClass() const
{ 
	return UOmniverseLiveLinkSourceSettings::StaticClass();
}

FText FOmniverseLiveLinkSource::GetSourceMachineName() const
{
	return LOCTEXT("OmniverseLiveLinkSourceMachineName", "localhost");
}

FText FOmniverseLiveLinkSource::GetSourceStatus() const
{
	return SourceStatus;
}

FText FOmniverseLiveLinkSource::GetSourceType() const
{
	return LOCTEXT("OmniverseLiveLinkSourceType", "NVIDIA Omniverse LiveLink");
}

void FOmniverseLiveLinkSource::ReceiveClient( ILiveLinkClient* InClient, FGuid InSourceGuid )
{
	LiveLinkListener->SetClient(InClient, InSourceGuid);
	WaveStreamer->SetClient(InClient, InSourceGuid);
}

bool FOmniverseLiveLinkSource::IsSourceStillValid() const
{
    // Source is valid if we have a valid thread and socket
    return LiveLinkListener->IsValid();
}

bool FOmniverseLiveLinkSource::RequestSourceShutdown()
{
    Stop();
	LiveLinkListener->ClearAllSubjects();
    return true;
}

void FOmniverseLiveLinkSource::Start()
{
	WaveStreamer->Start();
	LiveLinkListener->Start();
}

void FOmniverseLiveLinkSource::Stop()
{
	LiveLinkListener->Stop();
	WaveStreamer->Stop();
}

#undef LOCTEXT_NAMESPACE
