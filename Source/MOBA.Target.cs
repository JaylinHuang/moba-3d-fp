using UnrealBuildTool;
using System.Collections.Generic;

public class MOBATarget : TargetRules
{
	public MOBATarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.Add("MOBA");
	}
}
