// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class UnrealTeamProjectTarget : TargetRules
{
	public UnrealTeamProjectTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_5;
		UndefinedIdentifierWarningLevel = WarningLevel.Error;
		ExtraModuleNames.Add("UnrealTeamProject");
	}
}
