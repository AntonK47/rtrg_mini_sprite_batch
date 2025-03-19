#pragma once

#include "Common.hpp"
#include <memory>

struct RenderContext;

struct Game
{
private:
	
	struct GameImpl;
	std::unique_ptr<GameImpl> impl;
public:
	virtual ~Game();
	void Run(int argc, char* argv[]);
protected:
	Game();
	virtual void OnDraw(const f32 time) = 0;
	virtual void OnUpdate(const f32 time) = 0;
	virtual void OnLoad() = 0;
	virtual void OnUnload() = 0;

	std::unique_ptr<RenderContext> renderContext;
};