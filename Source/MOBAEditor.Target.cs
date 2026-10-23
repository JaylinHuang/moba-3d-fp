using UnrealBuildTool;
using System.Collections.Generic;

public class MOBAEditorTarget : TargetRules
{
	public MOBAEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.Add("MOBA");
		ExtraModuleNames.Add("MOBAEditor");
	}
}
