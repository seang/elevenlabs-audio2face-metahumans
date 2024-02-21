// Copyright(c) 2022, NVIDIA CORPORATION. All rights reserved.
//
// NVIDIA CORPORATION and its licensors retain all intellectual property
// and proprietary rights in and to this software, related documentation
// and any modifications thereto.Any use, reproduction, disclosure or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA CORPORATION is strictly prohibited.

#pragma once
#include "CoreMinimal.h"
#include "OmniverseBaseListener.h"


class FOmniverseLiveLinkListener : public FOmniverseBaseListener
{
public:
	FOmniverseLiveLinkListener(uint32 InPort);
	virtual ~FOmniverseLiveLinkListener();

	virtual void OnPackageDataReceived(const uint8* InPackageData, int32 InPackageSize) override;
	virtual void OnPackageDataPushed(const uint8* InPackageData, int32 InPackageSize, double DeltaTime, bool bBegin = false, bool bEnd = false) override;
	virtual uint32 GetDelayTime() const override;
	virtual bool IsHeaderPackage(const uint8* InPackageData, int32 InPackageSize) const override;
	virtual bool GetFPSInHeader(const uint8* InPackageData, int32 InPackageSize, double& OutFPS) const override;

	void ClearAllSubjects();

private:
	void ResetUsingSubjects();
	void RemoveUnusedSubjects();
	void ProcessAnimationData(const TSharedPtr<class FJsonObject>& DataObject, const FName& InSubjectName);
	bool ParseJSON(const uint8* InPackageData, int32 InPackageSize);

private:

	// List of subjects in using
	TMap<FName, bool> UsingSubjects;
};
