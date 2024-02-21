// Copyright(c) 2022-2023, NVIDIA CORPORATION. All rights reserved.
//
// NVIDIA CORPORATION and its licensors retain all intellectual property
// and proprietary rights in and to this software, related documentation
// and any modifications thereto.Any use, reproduction, disclosure or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA CORPORATION is strictly prohibited.

#include "OmniverseWaveStreamer.h"
#include "ACEPrivate.h"
#include "OmniverseLiveLinkSourceSettings.h"
#include "OmniverseSubmixListener.h"

#include "ILiveLinkClient.h"

#define LOCTEXT_NAMESPACE "OmniverseWaveStreamer"


FOmniverseWaveStreamer::FOmniverseWaveStreamer(uint32 InPort, uint32 InSampleRate)
    : FOmniverseBaseListener(InPort)
{
	SubmixListener = MakeShareable(new FOmniverseSubmixListener());
	SubmixListener->SetSampleRate(InSampleRate);
}

FOmniverseWaveStreamer::~FOmniverseWaveStreamer()
{
	SubmixListener.Reset();
}

// FRunnable interface
void FOmniverseWaveStreamer::Start()
{
	FOmniverseBaseListener::Start();
	SubmixListener->Activate();
}

void FOmniverseWaveStreamer::Stop()
{
	FOmniverseBaseListener::Stop();
	SubmixListener->Deactivate();
}

void FOmniverseWaveStreamer::OnPackageDataReceived(const uint8* InPackageData, int32 InPackageSize)
{
	ParseWave(InPackageData, InPackageSize);
}

void FOmniverseWaveStreamer::OnPackageDataPushed(const uint8* InPackageData, int32 InPackageSize, double DeltaTime, bool bBegin, bool bEnd)
{
	FOmniverseLiveLinkFramePlayer::Get().PushAudioData_AnyThread(InPackageData, InPackageSize, DeltaTime, bBegin, bEnd);
}

uint32 FOmniverseWaveStreamer::GetDelayTime() const
{
	if (LiveLinkClient)
	{
		UOmniverseLiveLinkSourceSettings* Settings = Cast<UOmniverseLiveLinkSourceSettings>(LiveLinkClient->GetSourceSettings(SourceGuid));
		if (Settings)
		{
			return Settings->AudioDelayTime;
		}
	}

	return 0;
}

bool FOmniverseWaveStreamer::IsHeaderPackage(const uint8* InPackageData, int32 InPackageSize) const
{
	const char MagicWord[] = { 'W', 'A', 'V', 'E' };

	const int32 MagicSize = sizeof(MagicWord) / sizeof(MagicWord[0]);

	bool bIsHeader = InPackageSize > MagicSize;
	if (bIsHeader)
	{
		for (int32 Index = 0; Index < MagicSize; ++Index)
		{
			if (InPackageData[Index] != MagicWord[Index])
			{
				bIsHeader = false;
				break;
			}
		}
	}

	return bIsHeader;
}

void FOmniverseWaveStreamer::ParseWave(const uint8* InReceivedData, int32 InReceivedSize)
{
	if (IsEOSPackage(InReceivedData, InReceivedSize))
	{
		return;
	}

	// The first batch will be a string containing wave format info
	// "SamplesPerSecond Channels BitsPerSample Samples SampleType"
	// This could also be JSON formatted instead
	
	if (IsHeaderPackage(InReceivedData, InReceivedSize))
	{
		FString ReceievedString = FString(InReceivedSize, (ANSICHAR*)InReceivedData);
		UE_LOG(LogACE, Warning, TEXT("Received wave format info: '%s'"), *ReceievedString);
		
		FOmniverseWaveFormatInfo WaveInfo;

		TArray<FString> WaveFormatInfoStrings;
		ReceievedString.ParseIntoArray(WaveFormatInfoStrings, *HeaderSeparator);
		if (WaveFormatInfoStrings.Num() == FOmniverseWaveFormatInfo::NumMembers + 1)
		{
			int32 InfoIndex = 1; // The first is MagicWord
			WaveInfo.SamplesPerSecond = FCString::Atoi(*WaveFormatInfoStrings[InfoIndex++]);
			WaveInfo.NumChannels = FCString::Atoi(*WaveFormatInfoStrings[InfoIndex++]);
			WaveInfo.BitsPerSample = FCString::Atoi(*WaveFormatInfoStrings[InfoIndex++]);
			WaveInfo.SampleType = FCString::Atoi(*WaveFormatInfoStrings[InfoIndex++]);
			SubmixListener->AddNewWave(WaveInfo);
		}
	}
	else
	{
		//UE_LOG(LogACE, Warning, TEXT("Wav bytes received: %i"), ReceivedData.Num());
		SubmixListener->AppendStream(InReceivedData, InReceivedSize);
	}
}

#undef LOCTEXT_NAMESPACE
