#pragma once

/**
 * 按当前 LaneTest 地板量出的小地图范围。
 * 只给表现投影用，不是玩法规则。地板以后挪了，只改这里。
 */
struct FMatchMapPlaceholder
{
	static constexpr float MinX = -5000.f;
	static constexpr float MaxX = 5000.f;
	static constexpr float MinY = 500.f;
	static constexpr float MaxY = 5500.f;
};
