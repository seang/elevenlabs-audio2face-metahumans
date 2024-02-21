// Copyright(c) 2022, NVIDIA CORPORATION. All rights reserved.
//
// NVIDIA CORPORATION and its licensors retain all intellectual property
// and proprietary rights in and to this software, related documentation
// and any modifications thereto.Any use, reproduction, disclosure or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA CORPORATION is strictly prohibited.

#pragma once
#include "LiveLinkSourceFactory.h"
#include "OmniverseLiveLinkSourceFactory.generated.h"

UCLASS()
class UOmniverseLiveLinkSourceFactory : public ULiveLinkSourceFactory
{
public:

    GENERATED_BODY()

    virtual FText GetSourceDisplayName() const override;
    virtual FText GetSourceTooltip() const override;

    virtual EMenuType GetMenuType() const override { return EMenuType::SubPanel; }
    virtual TSharedPtr<SWidget> BuildCreationPanel( FOnLiveLinkSourceCreated OnLiveLinkSourceCreated ) const override;
    TSharedPtr<ILiveLinkSource> CreateSource( const FString& ConnectionString) const override;

private:
    void OnOkClicked(const FString& ConnectionString, FOnLiveLinkSourceCreated OnLiveLinkSourceCreated) const;

};