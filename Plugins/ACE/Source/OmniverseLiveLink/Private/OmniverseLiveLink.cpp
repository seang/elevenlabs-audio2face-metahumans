// Copyright(c) 2022-2023, NVIDIA CORPORATION. All rights reserved.
//
// NVIDIA CORPORATION and its licensors retain all intellectual property
// and proprietary rights in and to this software, related documentation
// and any modifications thereto.Any use, reproduction, disclosure or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA CORPORATION is strictly prohibited.

#include "OmniverseLiveLink.h"
#include "ACEPrivate.h"
#include "OmniverseLiveLinkCommands.h"
#include "OmniverseLiveLinkStyle.h"

#include "Interfaces/IPluginManager.h"

#define LOCTEXT_NAMESPACE "OmniverseLiveLink"

DEFINE_LOG_CATEGORY(LogACE);

void FOmniverseLiveLinkModule::StartupModule()
{
	TSharedPtr<IPlugin> ThisPlugin = IPluginManager::Get().FindPlugin(TEXT("OmniverseLiveLink"));

	UE_LOG(LogACE, Log, TEXT("Loaded ACE plugin version %s"), *ThisPlugin->GetDescriptor().VersionName);

    // This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	FOmniverseLiveLinkStyle::Initialize();
	FOmniverseLiveLinkStyle::ReloadTextures();

    FOmniverseLiveLinkCommands::Register();
}

void FOmniverseLiveLinkModule::ShutdownModule()
{
    // This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
    // we call this function before unloading the module.
	FOmniverseLiveLinkStyle::Shutdown();
	FOmniverseLiveLinkCommands::Unregister();
}


#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE( FOmniverseLiveLinkModule, OmniverseLiveLink )
