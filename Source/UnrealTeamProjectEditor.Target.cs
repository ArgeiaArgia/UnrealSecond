// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class UnrealTeamProjectEditorTarget : TargetRules
{
	public UnrealTeamProjectEditorTarget( TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_5;
		UndefinedIdentifierWarningLevel = WarningLevel.Error;
		ExtraModuleNames.Add("UnrealTeamProject");
	}
}
