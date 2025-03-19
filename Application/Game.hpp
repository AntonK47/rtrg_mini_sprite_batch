#pragma once

#include <memory>
#include "Common.hpp"

struct RenderContext;
struct ContentManager;

struct Game
{
private:
	struct GameImpl;
	std::unique_ptr<GameImpl> impl;

public:
	virtual ~Game();
	void Run(int argc, char* argv[]);

	std::unique_ptr<ContentManager> content;

protected:
	Game();
	virtual void OnDraw(const f32 deltaTime) = 0;
	virtual void OnUpdate(const f32 deltaTime) = 0;
	virtual void OnLoad() = 0;
	virtual void OnUnload() = 0;

	std::unique_ptr<RenderContext> renderContext;
};