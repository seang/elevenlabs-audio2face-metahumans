// Copyright(c) 2022-2023, NVIDIA CORPORATION. All rights reserved.
//
// NVIDIA CORPORATION and its licensors retain all intellectual property
// and proprietary rights in and to this software, related documentation
// and any modifications thereto.Any use, reproduction, disclosure or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA CORPORATION is strictly prohibited.

#include "OmniverseLiveLinkListener.h"

#include "ILiveLinkClient.h"
#include "Roles/LiveLinkAnimationTypes.h"
#include "Roles/LiveLinkAnimationRole.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

#include "ACEPrivate.h"
#include "OmniverseLiveLinkSourceSettings.h"

#define LOCTEXT_NAMESPACE "OmniverseLiveLinkListener"


FOmniverseLiveLinkListener::FOmniverseLiveLinkListener(uint32 InPort)
	: FOmniverseBaseListener(InPort)
{
}

FOmniverseLiveLinkListener::~FOmniverseLiveLinkListener()
{
}

void FOmniverseLiveLinkListener::OnPackageDataReceived(const uint8* InPackageData, int32 InPackageSize)
{
	ParseJSON(InPackageData, InPackageSize);
}

void FOmniverseLiveLinkListener::OnPackageDataPushed(const uint8* InPackageData, int32 InPackageSize, double DeltaTime, bool bBegin, bool bEnd)
{
	FOmniverseLiveLinkFramePlayer::Get().PushAnimeData_AnyThread(InPackageData, InPackageSize, DeltaTime, bBegin, bEnd);
}

uint32 FOmniverseLiveLinkListener::GetDelayTime() const
{
	if (LiveLinkClient)
	{
		UOmniverseLiveLinkSourceSettings* Settings = Cast<UOmniverseLiveLinkSourceSettings>(LiveLinkClient->GetSourceSettings(SourceGuid));
		if (Settings)
		{
			return Settings->AnimationDelayTime;
		}
	}

	return 0;
}

bool FOmniverseLiveLinkListener::IsHeaderPackage(const uint8* InPackageData, int32 InPackageSize) const
{
	const char MagicWord[] = { 'A', '2', 'F' };

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

bool FOmniverseLiveLinkListener::GetFPSInHeader(const uint8* InPackageData, int32 InPackageSize, double& OutFPS) const
{
	FString HeaderString = FString(InPackageSize, (ANSICHAR*)InPackageData);
	TArray<FString> A2FInfoStrings;
	HeaderString.ParseIntoArray(A2FInfoStrings, *HeaderSeparator);

	if (A2FInfoStrings.Num() == 2)
	{
		OutFPS = FCString::Atoi(*A2FInfoStrings[1]);
		return OutFPS > 0;
	}

	return false;
}

void FOmniverseLiveLinkListener::ResetUsingSubjects()
{
	for (auto& Subject : UsingSubjects)
	{
		Subject.Value = false;
	}
}

void FOmniverseLiveLinkListener::RemoveUnusedSubjects()
{
	TSet<FName> UnusedSubjects;
	for (auto& Subject : UsingSubjects)
	{
		if (!Subject.Value)
		{
			if (LiveLinkClient)
			{
				LiveLinkClient->RemoveSubject_AnyThread(FLiveLinkSubjectKey(SourceGuid, Subject.Key));
			}
			UnusedSubjects.Add(Subject.Key);
		}
	}

	for (auto& SubjectName : UnusedSubjects)
	{
		if (UsingSubjects.Contains(SubjectName))
		{
			UsingSubjects.Remove(SubjectName);
		}
	}
}

void FOmniverseLiveLinkListener::ClearAllSubjects()
{
	ResetUsingSubjects();
	RemoveUnusedSubjects();
}

bool FOmniverseLiveLinkListener::ParseJSON(const uint8* InPackageData, int32 InPackageSize)
{
	if (IsEOSPackage(InPackageData, InPackageSize))
	{
		return true;
	}

	if (IsHeaderPackage(InPackageData, InPackageSize))
	{
		//
		// Nothing to do for blendshape header right now
		// 
		return true;
	}
	else
	{
		FString JsonString = FString(InPackageSize, (ANSICHAR*)InPackageData);
		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
		if (FJsonSerializer::Deserialize(Reader, JsonObject))
		{
			ResetUsingSubjects();
			for (TPair<FString, TSharedPtr<FJsonValue>>& JsonField : JsonObject->Values)
			{
				FName SubjectName(*JsonField.Key);
				if (SubjectName == TEXT("Disconnect"))
				{
					break;
				}

				const TSharedPtr<FJsonObject> DataObject = JsonField.Value->AsObject();
				ProcessAnimationData(DataObject, SubjectName);
			}
			RemoveUnusedSubjects();

			return true;
		}
	}

	return false;
}

void FOmniverseLiveLinkListener::ProcessAnimationData(const TSharedPtr<FJsonObject>& DataObject, const FName& InSubjectName)
{
	if (LiveLinkClient == nullptr)
	{
		return;
	}

	const TArray<TSharedPtr<FJsonValue>>* BoneArray = nullptr;
	DataObject->TryGetArrayField(TEXT("Body"), BoneArray);

	const TSharedPtr<FJsonValue> Facial = DataObject->TryGetField(TEXT("Facial"));

	bool bCreateSubject = !UsingSubjects.Contains(InSubjectName);

	// NOTE: SkeletonData pointer to FrameData, so they must have the same scope
	FLiveLinkSkeletonStaticData* SkeletonData = nullptr;
	FLiveLinkSubjectFrameData FrameData;
	if (!bCreateSubject)
	{
		// check if static data (bones and curve names) is changed, if it was changed, recreate the subject
		auto AllSubjects = LiveLinkClient->GetSubjects(true, false);
		for (auto Subject : AllSubjects)
		{
			if (Subject.SubjectName == InSubjectName)
			{
				auto SubjectRole = LiveLinkClient->GetSubjectRole_AnyThread(Subject);				
				if (LiveLinkClient->EvaluateFrame_AnyThread(InSubjectName, SubjectRole, FrameData))
				{
					SkeletonData = FrameData.StaticData.Cast<FLiveLinkSkeletonStaticData>();
				}
				break;
			}
		}
	}

	// valid skeleton
	if (SkeletonData && BoneArray)
	{
		// bone is changed, need to be recreated
		if (SkeletonData->BoneNames.Num() != BoneArray->Num())
		{
			bCreateSubject = true;
		}
	}

	TArray< FName > ExpNames;
	if (Facial) // only facial need to check curve for now
	{
		auto ExpWeightObject = Facial->AsObject();
		if (ExpWeightObject.Get())
		{
			const TArray<TSharedPtr<FJsonValue>>* ExpData = nullptr;
			if (ExpWeightObject->TryGetArrayField(TEXT("Names"), ExpData))
			{
				for (int32 Index = 0; Index < ExpData->Num(); ++Index)
				{
					FString Name = (*ExpData)[Index]->AsString();
					ExpNames.Add(FName(*Name));
				}
			}
		}

		if (SkeletonData)
		{
			// different number of curves
			if (SkeletonData->PropertyNames.Num() != ExpNames.Num())
			{
				bCreateSubject = true;
			}
		}
	}

	// Create static data : skeleton and curves
	if (bCreateSubject)
	{
		UE_LOG(LogACE, Log, TEXT("Creating subject '%s'"), *InSubjectName.ToString());

		FLiveLinkStaticDataStruct StaticData(FLiveLinkSkeletonStaticData::StaticStruct());
		FLiveLinkSkeletonStaticData* NewSkeletonData = StaticData.Cast<FLiveLinkSkeletonStaticData>();
		TArray<FName> BoneNames;
		if (BoneArray)
		{
			BoneNames.SetNumUninitialized(BoneArray->Num());
			TArray<int32> BoneParents;
			BoneParents.SetNumUninitialized(BoneArray->Num());

			for (int32 BoneIndex = 0; BoneIndex < BoneArray->Num(); ++BoneIndex)
			{
				const TSharedPtr<FJsonValue>& BoneValue = (*BoneArray)[BoneIndex];
				const TSharedPtr<FJsonObject> BoneObject = BoneValue->AsObject();

				FString BoneName;
				if (BoneObject->TryGetStringField(TEXT("Name"), BoneName))
				{
					BoneNames[BoneIndex] = FName(*(BoneName.ToLower()));

					// Get bone index if getting parent
					FString BoneParentName;
					if (BoneObject->TryGetStringField(TEXT("ParentName"), BoneParentName))
					{
						BoneParents[BoneIndex] = BoneIndex;
					}
					else
					{
						return; // Invalid Json Format
					}
				}
				else
				{
					return; // Invalid Json Format
				}
			}

			NewSkeletonData->SetBoneNames(BoneNames);
			NewSkeletonData->SetBoneParents(BoneParents);
		}

		if (Facial) // Facial need the static curve name
		{
			NewSkeletonData->PropertyNames = ExpNames;
		}
		FLiveLinkSubjectKey Key = FLiveLinkSubjectKey(SourceGuid, InSubjectName);
		LiveLinkClient->RemoveSubject_AnyThread(Key);
		LiveLinkClient->PushSubjectStaticData_AnyThread(Key, ULiveLinkAnimationRole::StaticClass(), MoveTemp(StaticData));
	}
	UsingSubjects.Add(InSubjectName, true);

	FLiveLinkFrameDataStruct AnimationStruct(FLiveLinkAnimationFrameData::StaticStruct());
	FLiveLinkAnimationFrameData& NewData = *AnimationStruct.Cast<FLiveLinkAnimationFrameData>();
	TArray<FTransform>& DataTransforms = NewData.Transforms;

	if (BoneArray) // valid bone need transforms
	{
		DataTransforms.SetNumZeroed(BoneArray->Num());

		UE_LOG(LogACE, Log, TEXT("Bone Array '%d'"), BoneArray->Num());

		for (int32 BoneIndex = 0; BoneIndex < BoneArray->Num(); ++BoneIndex)
		{
			const TSharedPtr<FJsonValue>& BoneValue = (*BoneArray)[BoneIndex];
			const TSharedPtr<FJsonObject> BoneObject = BoneValue->AsObject();

			const TArray<TSharedPtr<FJsonValue>>* LocationArray = nullptr;
			FVector BoneLocation;

			if (BoneObject->TryGetArrayField(TEXT("Location"), LocationArray)
				&& LocationArray->Num() == 3) // X, Y, Z
			{
				double X = (*LocationArray)[0]->AsNumber();
				double Y = (*LocationArray)[1]->AsNumber();
				double Z = (*LocationArray)[2]->AsNumber();
				BoneLocation = FVector(X, -Y, Z);
			}
			else
			{
				// Invalid Json Format
				return;
			}

			const TArray<TSharedPtr<FJsonValue>>* RotationArray = nullptr;
			FQuat BoneQuat;
			if (BoneObject->TryGetArrayField(TEXT("Rotation"), RotationArray)
				&& RotationArray->Num() == 4) // X, Y, Z, W
			{
				double X = (*RotationArray)[0]->AsNumber();
				double Y = (*RotationArray)[1]->AsNumber();
				double Z = (*RotationArray)[2]->AsNumber();
				double W = (*RotationArray)[3]->AsNumber();
				FVector RightVec = FQuat(X, Y, Z, W).Euler();
				FVector LeftVec = FVector(-RightVec.X, RightVec.Y, -RightVec.Z);
				BoneQuat = FQuat::MakeFromEuler(LeftVec);
			}
			else
			{
				// Invalid Json Format
				return;
			}
			DataTransforms[BoneIndex] = FTransform(BoneQuat, BoneLocation);
		}
	}

	if (Facial && ExpNames.Num() > 0)
	{
		TArray<float>& PropertyValues = NewData.PropertyValues;
		PropertyValues.SetNumZeroed(ExpNames.Num());
		auto ExpWeightObject = Facial->AsObject();
		if (ExpWeightObject.Get())
		{
			const TArray<TSharedPtr<FJsonValue>>* ExpWeight = nullptr;
			if (ExpWeightObject->TryGetArrayField(TEXT("Weights"), ExpWeight))
			{
				for (int32 Index = 0; Index < ExpWeight->Num(); ++Index)
				{
					double fWeight = (*ExpWeight)[Index]->AsNumber();
					PropertyValues[Index] = fWeight;
				}
			}
		}
	}

	FLiveLinkSubjectKey SubjectKey(SourceGuid, InSubjectName);
	LiveLinkClient->PushSubjectFrameData_AnyThread(SubjectKey, MoveTemp(AnimationStruct));
}


#undef LOCTEXT_NAMESPACE
