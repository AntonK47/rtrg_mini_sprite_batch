#pragma once

#include <vector>
#include <array>
#include <string>
#include <unordered_map>
#include <variant>

#include "Common.hpp"

struct AnimationFrame
{
	Rectangle sourceSprite;
	vec2 root;
};

enum class AnimationRepeat
{
	once,
	loop
};

struct Animation
{
	std::string name;
	u32 animationIndex;
	u32 animationFrameCount;
	AnimationRepeat repeat{ AnimationRepeat::once };
};

enum class FrameFlip
{
	none,
	horizontal
};

struct AnimationKey
{
	u32 frameIndex{ 0 };
	u32 duration{ 1 };
	FrameFlip flip{ FrameFlip::none };
};

static const auto animationFrames = std::array{
	AnimationFrame{ { { 3, 22 }, { 316 - 3, 368 - 22 } }, { 131, 366 } },
	AnimationFrame{ { { 320, 22 }, { 633 - 320, 368 - 22 } }, { 447, 365 } },
	AnimationFrame{ { { 637, 22 }, { 950 - 637, 368 - 22 } }, { 764, 366 } },
	AnimationFrame{ { { 954, 22 }, { 1267 - 954, 368 - 22 } }, { 1080, 366 } },
	AnimationFrame{ { { 1271, 22 }, { 1584 - 1271, 368 - 22 } }, { 1958, 365 } },

	AnimationFrame{ { { 3, 1098 }, { 304 - 3, 1452 - 1098 } }, { 118, 1447 } },
	AnimationFrame{ { { 308, 1098 }, { 609 - 308, 1452 - 1098 } }, { 423, 1449 } },

	AnimationFrame{ { { 3, 391 }, { 317 - 3, 731 - 391 } }, { 191, 725 } },
	AnimationFrame{ { { 321, 391 }, { 635 - 321, 731 - 391 } }, { 491, 727 } },
	AnimationFrame{ { { 639, 391 }, { 953 - 639, 731 - 391 } }, { 771, 730 } },
	AnimationFrame{ { { 957, 391 }, { 1271 - 957, 731 - 391 } }, { 1062, 728 } },
	AnimationFrame{ { { 1275, 391 }, { 1589 - 1275, 731 - 391 } }, { 1358, 725 } },
	AnimationFrame{ { { 1593, 391 }, { 1907 - 1593, 731 - 391 } }, { 1673, 713 } },
	AnimationFrame{ { { 3, 735 }, { 317 - 3, 1075 - 735 } }, { 0, 0 } },
	AnimationFrame{ { { 321, 735 }, { 635 - 321, 1075 - 735 } }, { 0, 0 } },
};

static const auto animationSequences = std::array{
	AnimationKey{ 0, 1, FrameFlip::none },		  AnimationKey{ 1, 1, FrameFlip::none },
	AnimationKey{ 2, 1, FrameFlip::none },		  AnimationKey{ 3, 1, FrameFlip::none },
	AnimationKey{ 4, 1, FrameFlip::none },		  AnimationKey{ 3, 1, FrameFlip::none },
	AnimationKey{ 2, 1, FrameFlip::none },		  AnimationKey{ 1, 1, FrameFlip::none }, // Idle Right

	AnimationKey{ 0, 1, FrameFlip::horizontal },  AnimationKey{ 1, 1, FrameFlip::horizontal },
	AnimationKey{ 2, 1, FrameFlip::horizontal },  AnimationKey{ 3, 1, FrameFlip::horizontal },
	AnimationKey{ 4, 1, FrameFlip::horizontal },  AnimationKey{ 3, 1, FrameFlip::horizontal },
	AnimationKey{ 2, 1, FrameFlip::horizontal },  AnimationKey{ 1, 1, FrameFlip::horizontal }, // Idle Left

	AnimationKey{ 5, 1, FrameFlip::none },		  AnimationKey{ 6, 1, FrameFlip::none }, // Turn Left
	AnimationKey{ 5, 1, FrameFlip::horizontal },  AnimationKey{ 6, 1, FrameFlip::horizontal }, // Turn Right,

	AnimationKey{ 7, 1, FrameFlip::none },		  AnimationKey{ 8, 1, FrameFlip::none },
	AnimationKey{ 9, 1, FrameFlip::none },		  AnimationKey{ 10, 1, FrameFlip::none },
	AnimationKey{ 11, 1, FrameFlip::none },		  AnimationKey{ 12, 1, FrameFlip::none },
	AnimationKey{ 13, 1, FrameFlip::none },		  AnimationKey{ 14, 1, FrameFlip::none },

	AnimationKey{ 7, 1, FrameFlip::horizontal },  AnimationKey{ 8, 1, FrameFlip::horizontal },
	AnimationKey{ 9, 1, FrameFlip::horizontal },  AnimationKey{ 10, 1, FrameFlip::horizontal },
	AnimationKey{ 11, 1, FrameFlip::horizontal }, AnimationKey{ 12, 1, FrameFlip::horizontal },
	AnimationKey{ 13, 1, FrameFlip::horizontal }, AnimationKey{ 14, 1, FrameFlip::horizontal },
};

static const auto animations = std::array{
	Animation{ "idle-right", 0, 8, AnimationRepeat::loop },	 Animation{ "idle-left", 8, 8, AnimationRepeat::loop },
	Animation{ "turn-left", 16, 2, AnimationRepeat::once },	 Animation{ "turn-right", 18, 2, AnimationRepeat::once },
	Animation{ "walk-right", 20, 8, AnimationRepeat::loop }, Animation{ "walk-left", 28, 8, AnimationRepeat::loop }
};

struct AnimationInstance
{
	u32 currentNodeIndex;
	u32 key;
};

struct SequenceItem
{
	u32 node;
	i32 key;

	auto operator<=>(const SequenceItem&) const = default;
};
using AnimationSequence = std::vector<SequenceItem>;

struct AnimationPlayer
{
	AnimationInstance ForwardAnimation(AnimationInstance instance, AnimationSequence& sequence);
	void ForwardTime(const f32 deltaTime);

	f32 localTime;
	f32 frameDuration{ 0.16f };
	bool nextKey{ false };
};

struct AnimationState
{
	std::string name;
};

struct AnimationTransition
{
	i32 key;
	u32 nodeIndex;
};

struct SyncOnKey
{
	i32 key;
};

struct SyncOnLastFrame
{
};

struct SyncImmediate
{
};

namespace animation::sync
{
	inline constexpr SyncOnLastFrame lastFrame{};
	inline constexpr SyncImmediate immediate{};

} // namespace animation::sync

using AnimationSyncBehavior = std::variant<SyncOnKey, SyncOnLastFrame, SyncImmediate>;

struct AnimationGraph
{
	using AnimationIndex = u32;

	void AddNode(const std::string& name, const AnimationIndex animationIndex);
	void AddTransition(const std::string& from, const std::string& to,
					   const AnimationSyncBehavior& syncBehavior = animation::sync::immediate);
	AnimationIndex GetNodeIndex(const std::string& nodeName) const;
	AnimationSequence FindAnimationSequence(const AnimationInstance& instance, const std::string& to);

	std::unordered_map<std::string, AnimationIndex> nameToNodeIndexMap;
	std::unordered_map<AnimationIndex, std::vector<AnimationTransition>> transitions;
};
