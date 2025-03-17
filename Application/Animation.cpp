#include "Animation.hpp"

void AnimationGraph::AddNode(const std::string& name, const AnimationIndex animationIndex)
{
	assert(not nameToNodeIndexMap.contains(name));
	nameToNodeIndexMap[name] = animationIndex;
}

void AnimationGraph::AddTransition(const std::string& from, const std::string& to,
								   const AnimationSyncBehavior& syncBehavior)
{
	const auto fromIndex = nameToNodeIndexMap[from];
	const auto toIndex = nameToNodeIndexMap[to];
	auto syncKey = i32{ 0 };
	if (std::holds_alternative<SyncOnKey>(syncBehavior))
	{
		const auto sync = std::get<SyncOnKey>(syncBehavior);
		syncKey = sync.key;
	}
	if (std::holds_alternative<SyncOnLastFrame>(syncBehavior))
	{
		const auto lastFrame = animations[fromIndex].animationFrameCount - 1;
		syncKey = lastFrame;
	}
	if (std::holds_alternative<SyncImmediate>(syncBehavior))
	{
		syncKey = -1;
	}
	transitions[fromIndex].push_back(AnimationTransition{ syncKey, toIndex });
}

AnimationGraph::AnimationIndex AnimationGraph::GetNodeIndex(const std::string& nodeName) const
{
	assert(nameToNodeIndexMap.contains(nodeName));
	return nameToNodeIndexMap.at(nodeName);
}

AnimationSequence AnimationGraph::FindAnimationSequence(const AnimationInstance& instance, const std::string& to)
{
	// TODO: The graph might be not fully connected, so it might fail in the while loop. As an possible strategy we can
	// switch to a target animation immediately.
	const auto targetNodeIndex = nameToNodeIndexMap.at(to);
	const auto startNodeIndex = instance.currentNodeIndex;
	auto animationSequence = AnimationSequence{};

	auto distances = std::vector<u32>{};
	auto visited = std::vector<bool>{};
	auto previes = std::vector<u32>{};
	const auto nodes = nameToNodeIndexMap.size();
	distances.resize(nodes);
	visited.resize(nodes);
	previes.resize(nodes);

	constexpr auto maxDistanceValue = std::numeric_limits<decltype(distances)::value_type>::max();

	auto searchMinimumDistance = [&]()
	{
		auto min = maxDistanceValue;
		auto index = 0;

		for (auto i = 0; i < nodes; i++)
		{
			if (not visited[i] and distances[i] <= min)
			{
				min = distances[i];
				index = i;
			}
		}
		return index;
	};

	for (auto i = 0; i < nodes; i++)
	{
		distances[i] = maxDistanceValue;
		visited[i] = false;
	}
	distances[startNodeIndex] = 0;
	for (auto i = 0; i < nodes; i++)
	{
		auto minimumDistanceIndex = searchMinimumDistance();
		visited[minimumDistanceIndex] = true;

		for (auto& transition : transitions[minimumDistanceIndex])
		{
			const auto foundShortestPath = not visited[transition.nodeIndex] and
				distances[minimumDistanceIndex] != maxDistanceValue and
				distances[minimumDistanceIndex] + 1 < distances[transition.nodeIndex];

			if (foundShortestPath)
			{
				distances[transition.nodeIndex] = distances[minimumDistanceIndex] + 1;
				previes[transition.nodeIndex] = minimumDistanceIndex;
			}
		}
	}

	animationSequence.push_back(SequenceItem{ targetNodeIndex, -1 });
	auto tmp = targetNodeIndex;
	while (tmp != startNodeIndex)
	{
		const auto previesNode = previes[tmp];
		const auto& transition = transitions[previesNode];

		auto animationTransitionKey = 0;
		for (const auto& animation : transition)
		{
			if (animation.nodeIndex == tmp)
			{
				animationTransitionKey = animation.key;
			}
		}

		animationSequence.push_back(SequenceItem{ previesNode, animationTransitionKey });
		tmp = previesNode;
	}

	std::reverse(animationSequence.begin(), animationSequence.end());
	return animationSequence;
}

AnimationInstance AnimationPlayer::ForwardAnimation(AnimationInstance instance, AnimationSequence& sequence)
{
	auto newInstance = AnimationInstance{};
	newInstance.currentNodeIndex = instance.currentNodeIndex;
	newInstance.key = instance.key;

	if (nextKey)
	{
		const auto& animation = animations[instance.currentNodeIndex];


		if (animation.repeat == AnimationRepeat::loop)
		{
			newInstance.key = (newInstance.key + 1) % animation.animationFrameCount;
		}
		else
		{
			newInstance.key = std::min((newInstance.key + 1), animation.animationFrameCount - 1);
		}

		if (not sequence.empty())
		{
			const auto& item = sequence.front();
			if (item.node == newInstance.currentNodeIndex)
			{
				if (item.key == -1 or item.key == newInstance.key)
				{
					sequence.erase(std::remove(sequence.begin(), sequence.end(), item), sequence.end());
					if (not sequence.empty())
					{
						newInstance.currentNodeIndex = sequence.front().node;
						newInstance.key = 0;
					}
				}
			}
		}
	}
	return newInstance;
}

void AnimationPlayer::ForwardTime(const f32 deltaTime)
{
	nextKey = false;
	localTime += deltaTime;
	if (localTime > frameDuration)
	{
		nextKey = true;
		localTime = fmodf(localTime, frameDuration);
	}
}