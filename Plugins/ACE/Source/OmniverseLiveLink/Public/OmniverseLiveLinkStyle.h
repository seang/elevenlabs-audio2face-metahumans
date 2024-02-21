// Copyright(c) 2022, NVIDIA CORPORATION. All rights reserved.
//
// NVIDIA CORPORATION and its licensors retain all intellectual property
// and proprietary rights in and to this software, related documentation
// and any modifications thereto.Any use, reproduction, disclosure or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA CORPORATION is strictly prohibited.

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateStyle.h"

class FOmniverseLiveLinkStyle
{
public:

    static void Initialize();

    static void Shutdown();

    /** reloads textures used by slate renderer */
    static void ReloadTextures();

    /** @return The Slate style set for the Shooter game */
    static const ISlateStyle& Get();

    static FName GetStyleSetName();

private:

    static TSharedRef< class FSlateStyleSet > Create();

private:

    static TSharedPtr< class FSlateStyleSet > StyleInstance;
};