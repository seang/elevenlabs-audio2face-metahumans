// Copyright(c) 2022, NVIDIA CORPORATION. All rights reserved.
//
// NVIDIA CORPORATION and its licensors retain all intellectual property
// and proprietary rights in and to this software, related documentation
// and any modifications thereto.Any use, reproduction, disclosure or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA CORPORATION is strictly prohibited.

using UnrealBuildTool;

public class OmniverseAudioMixer : ModuleRules
{
	public OmniverseAudioMixer(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(
        new string[] {
                "Core",
                "CoreUObject",
                "Engine",
                "BinkAudioDecoder",
                "AudioMixerCore",
                "SignalProcessing",
                "AudioMixer",
			}
		);
    }
}

