// Copyright(c) 2022, NVIDIA CORPORATION. All rights reserved.
//
// NVIDIA CORPORATION and its licensors retain all intellectual property
// and proprietary rights in and to this software, related documentation
// and any modifications thereto.Any use, reproduction, disclosure or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA CORPORATION is strictly prohibited.

#include "OmniverseLiveLinkSourceFactory.h"
#include "OmniverseLiveLinkSource.h"
#include "ILiveLinkClient.h"
#include "Features/IModularFeatures.h"
#include "SOmniverseLiveLinkWidget.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "Logging/LogMacros.h"


#define LOCTEXT_NAMESPACE "OmniverseLiveLinkSourceFactory"

FText UOmniverseLiveLinkSourceFactory::GetSourceDisplayName() const
{
	return LOCTEXT("SourceDisplayName", "NVIDIA Omniverse LiveLink");
}

FText UOmniverseLiveLinkSourceFactory::GetSourceTooltip() const
{
	return LOCTEXT("SourceTooltip", "Creates a connection to an Omniverse TCP Stream");
}

TSharedPtr<SWidget> UOmniverseLiveLinkSourceFactory::BuildCreationPanel( FOnLiveLinkSourceCreated InOnLiveLinkSourceCreated ) const
{
	return SNew( SOmniverseLiveLinkWidget )
		.OnOkClicked(SOmniverseLiveLinkWidget::FOnOkClicked::CreateUObject( this, &UOmniverseLiveLinkSourceFactory::OnOkClicked, InOnLiveLinkSourceCreated ) );
}

TSharedPtr<ILiveLinkSource> UOmniverseLiveLinkSourceFactory::CreateSource(const FString& ConnectionString) const
{
	TArray<FString> ConnectionInfos;
	ConnectionString.ParseIntoArray(ConnectionInfos, TEXT(";"), false);
	check(ConnectionInfos.Num() >= 2);
	int Port = FCString::Atoi(*ConnectionInfos[0]);
	int AudioPort = FCString::Atoi(*ConnectionInfos[1]);
	int SampleRate = FCString::Atoi(*ConnectionInfos[2]);
	return MakeShared<FOmniverseLiveLinkSource>(Port, AudioPort, SampleRate);
}

void UOmniverseLiveLinkSourceFactory::OnOkClicked(const FString& ConnectionString, FOnLiveLinkSourceCreated OnLiveLinkSourceCreated) const
{
	if(ConnectionString.IsEmpty())
	{
		return;
	}

	TArray<FString> ConnectionInfos;
	ConnectionString.ParseIntoArray(ConnectionInfos, TEXT(";"), false);
	check(ConnectionInfos.Num() >= 2);
	int Port = FCString::Atoi(*ConnectionInfos[0]);
	int AudioPort = FCString::Atoi(*ConnectionInfos[1]);
	int SampleRate = FCString::Atoi(*ConnectionInfos[2]);
	OnLiveLinkSourceCreated.ExecuteIfBound(MakeShared<FOmniverseLiveLinkSource>(Port, AudioPort, SampleRate), ConnectionString);
}

#undef LOCTEXT_NAMESPACE
