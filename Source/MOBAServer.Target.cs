using UnrealBuildTool;
using System.Collections.Generic;

public class MOBAServerTarget : TargetRules
{
	public MOBAServerTarget(TargetInfo Target) : base(Target)
	{
		// 启动器安装的 5.8 没有 TargetType.Server 预编译库，按服务器类型链接会被拒绝。
		// 本目标仍编出 MOBAServer.exe。启动时必须带 -server，进程没有本地玩家，权威在这台服务器上。
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.Add("MOBA");
	}
}
