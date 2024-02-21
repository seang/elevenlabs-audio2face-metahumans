// Copyright(c) 2022, NVIDIA CORPORATION. All rights reserved.
//
// NVIDIA CORPORATION and its licensors retain all intellectual property
// and proprietary rights in and to this software, related documentation
// and any modifications thereto.Any use, reproduction, disclosure or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA CORPORATION is strictly prohibited.

#pragma once

#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"
#include "Misc/Guid.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Input/Reply.h"
#include "Types/SlateEnums.h"

class SCheckBox;
class SEditableTextBox;
class SOmniverseLiveLinkWidget : public SCompoundWidget
{
public:
	DECLARE_DELEGATE_OneParam( FOnOkClicked, const FString& );
	FOnOkClicked OkClicked;

	SLATE_BEGIN_ARGS(SOmniverseLiveLinkWidget) {}
	    SLATE_EVENT( FOnOkClicked, OnOkClicked )
	SLATE_END_ARGS()

	void Construct( const FArguments& Args );

private:
	void OnPortChanged(const FText& strNewValue, ETextCommit::Type);
	void OnAudioPortChanged(const FText& strNewValue, ETextCommit::Type);
	void OnComboBoxChanged(TSharedPtr<FName> InItem, ESelectInfo::Type InSeletionInfo);
	TSharedRef<SWidget> OnGetComboBoxWidget(TSharedPtr<FName> InItem);
	FText GetCurrentNameAsText() const;

	FReply OnOkClicked();
	FReply OnCancelClicked();

	FString GetPort() const
	{
		return PortNumber;
	};

	FString GetAudioPort() const
	{
		return AudioPortNumber;
	};

private:
	TWeakPtr<SEditableTextBox> PortEditabledText;
	TWeakPtr<SEditableTextBox> AudioPortEditabledText;
	FString PortNumber = "12030";
	FString AudioPortNumber = "12031";
	TArray<TSharedPtr<FName>> SampleRateOptions;
	TSharedPtr<FName> SelectedSampleRate;
};
