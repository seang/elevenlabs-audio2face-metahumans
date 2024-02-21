// Copyright(c) 2022, NVIDIA CORPORATION. All rights reserved.
//
// NVIDIA CORPORATION and its licensors retain all intellectual property
// and proprietary rights in and to this software, related documentation
// and any modifications thereto.Any use, reproduction, disclosure or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA CORPORATION is strictly prohibited.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "OmniverseLiveLinkStyle.h"

class FOmniverseLiveLinkCommands : public TCommands<FOmniverseLiveLinkCommands>
{
public:

    FOmniverseLiveLinkCommands()
        : TCommands<FOmniverseLiveLinkCommands>( TEXT( "OmniverseLiveLink" ),
                                          NSLOCTEXT( "Contexts", "OmniverseLiveLink", "OmniverseLiveLink Plugin" ),
                                          NAME_None,
                                          FOmniverseLiveLinkStyle::GetStyleSetName() )
    {
    }

    // TCommands<> interface
    virtual void RegisterCommands() override;

public:
    TSharedPtr< FUICommandInfo > PluginAction;
};
