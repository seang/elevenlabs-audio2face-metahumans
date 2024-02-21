// Copyright(c) 2022, NVIDIA CORPORATION. All rights reserved.
//
// NVIDIA CORPORATION and its licensors retain all intellectual property
// and proprietary rights in and to this software, related documentation
// and any modifications thereto.Any use, reproduction, disclosure or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA CORPORATION is strictly prohibited.

#include "SOmniverseLiveLinkWidget.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SNumericEntryBox.h"

#define LOCTEXT_NAMESPACE "SOmniverseLiveLinkWidget"

void SOmniverseLiveLinkWidget::Construct( const FArguments& Args )
{
	SampleRateOptions.Add(MakeShareable(new FName(TEXT("16k Hz"))));
	SampleRateOptions.Add(MakeShareable(new FName(TEXT("22.05k Hz"))));
	SampleRateOptions.Add(MakeShareable(new FName(TEXT("44.1k Hz"))));
	SampleRateOptions.Add(MakeShareable(new FName(TEXT("48k Hz"))));
	SelectedSampleRate = SampleRateOptions[0];

	OkClicked = Args._OnOkClicked;

	ChildSlot
	[
		SNew(SBox)
		.WidthOverride(300)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(10, 0, 10, 0)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Center)
				.FillWidth(0.5f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("OmniversePortNumber", "Port"))
				]
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Fill)
				.FillWidth(0.5f)
				[
					SAssignNew(PortEditabledText, SEditableTextBox)
					.Text(FText::FromString(GetPort()))
				.OnTextCommitted(this, &SOmniverseLiveLinkWidget::OnPortChanged)
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(10, 0, 10, 0)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Center)
				.FillWidth(0.5f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("OmniverseAudioPortNumber", "Audio Port"))
				]
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Fill)
				.FillWidth(0.5f)
				[
					SAssignNew(AudioPortEditabledText, SEditableTextBox)
					.Text(FText::FromString(GetAudioPort()))
				.OnTextCommitted(this, &SOmniverseLiveLinkWidget::OnAudioPortChanged)
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(10, 0, 10, 0)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Center)
				.FillWidth(0.5f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("OmniverseAudioSampleRate", "Audio Sample Rate"))
				]
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Fill)
				.FillWidth(0.5f)
				[
					SNew(SComboBox<TSharedPtr<FName>>)
					.OptionsSource(&SampleRateOptions)
					.OnSelectionChanged(this, &SOmniverseLiveLinkWidget::OnComboBoxChanged)
					.OnGenerateWidget(this, &SOmniverseLiveLinkWidget::OnGetComboBoxWidget)
					.Content()
					[
						SNew(STextBlock)
						.Text(this, &SOmniverseLiveLinkWidget::GetCurrentNameAsText)
					]
				]
			]
			+ SVerticalBox::Slot()
			.HAlign(HAlign_Right)
			.Padding(10, 10, 10, 0)
			.AutoHeight()
			[
				SNew( SHorizontalBox )
				+ SHorizontalBox::Slot()
				.HAlign( HAlign_Right )
				.AutoWidth()
				[
					SNew( SButton )
					.OnClicked( this, &SOmniverseLiveLinkWidget::OnOkClicked )
				    [
				    	SNew( STextBlock )
				    	.Text( LOCTEXT( "Ok", "Ok" ) )
				    ]
				]
			    + SHorizontalBox::Slot()
				.HAlign( HAlign_Right )
				.AutoWidth()
				[
					SNew( SButton )
					.OnClicked( this, &SOmniverseLiveLinkWidget::OnCancelClicked )
				    [
				    	SNew( STextBlock )
				    	.Text( LOCTEXT( "Cancel", "Cancel" ) )
				    ]
				]		
			]
		]
	];
}

void SOmniverseLiveLinkWidget::OnComboBoxChanged(TSharedPtr<FName> InItem, ESelectInfo::Type InSeletionInfo)
{
	if (InItem.IsValid() && InItem != SelectedSampleRate)
	{
		SelectedSampleRate = InItem;
	}
}

TSharedRef<SWidget> SOmniverseLiveLinkWidget::OnGetComboBoxWidget(TSharedPtr<FName> InItem)
{
	return SNew(STextBlock).Text(FText::FromName(InItem.IsValid() ? *InItem : NAME_None));
}

FText SOmniverseLiveLinkWidget::GetCurrentNameAsText() const
{
	return FText::FromName(SelectedSampleRate.IsValid() ? *SelectedSampleRate : NAME_None);
}

void SOmniverseLiveLinkWidget::OnPortChanged( const FText& NewValue, ETextCommit::Type )
{
	TSharedPtr<SEditableTextBox> EditabledTextPin = PortEditabledText.Pin();
	if (EditabledTextPin.IsValid() )
	{
		EditabledTextPin->SetText( FText::FromString( NewValue.ToString() ) );
	}
}

void SOmniverseLiveLinkWidget::OnAudioPortChanged(const FText& NewValue, ETextCommit::Type)
{
	TSharedPtr<SEditableTextBox> EditabledTextPin = AudioPortEditabledText.Pin();
	if (EditabledTextPin.IsValid())
	{
		EditabledTextPin->SetText(FText::FromString(NewValue.ToString()));
	}
}

FReply SOmniverseLiveLinkWidget::OnOkClicked()
{
	if (PortEditabledText.Pin().IsValid() && AudioPortEditabledText.Pin().IsValid())
	{
		FString SampleRateString = TEXT("16000");
		if (SelectedSampleRate->IsEqual(*SampleRateOptions[1].Get()))
		{
			SampleRateString = TEXT("22050");
		}
		else if (SelectedSampleRate->IsEqual(*SampleRateOptions[2].Get()))
		{
			SampleRateString = TEXT("44100");
		}
		else if (SelectedSampleRate->IsEqual(*SampleRateOptions[3].Get()))
		{
			SampleRateString = TEXT("48000");
		}
		// Blendshape port
		// Audio port
		OkClicked.ExecuteIfBound(PortEditabledText.Pin()->GetText().ToString()
			+ TEXT(";")
			+ AudioPortEditabledText.Pin()->GetText().ToString()
			+ TEXT(";")
			+ SampleRateString
		);
	}
	return FReply::Handled();
}

FReply SOmniverseLiveLinkWidget::OnCancelClicked()
{
	TSharedPtr<SWindow> CurrentWindow = FSlateApplication::Get().FindWidgetWindow( AsShared() );
	if(CurrentWindow.IsValid() )
	{
		CurrentWindow->RequestDestroyWindow();
	}

	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
