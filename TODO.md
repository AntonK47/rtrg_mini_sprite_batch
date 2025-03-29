# TODO
> The rocket ( :rocket: ) emoji is used to mark priority. More rockets ( :rocket: ) means higher priority. 

> last updated on 30.03.2025
---
- [x] create render_context
	- [x] backbuffer context [width, height] - viewport - scissor
	- [x] helper for render to backbuffer (simple blit) :rocket: :rocket: 
	> The heavy lifting part of this task is already done, but we have to move the rest of a code related to blit out of main into the render_context itself, or create a new helper class/struct.
	- [x] store pipelines
	- [ ] store samplers :rocket:
	> The pipeline and the samplers objects should be stored in the render_context. Follow the api of storing FBO's. The user should only work with handle object.
	---
	> vertex layout related task are only required if we want to change the underneath mesh in the sprite_batch, for example we want to change quads to a arbitrary polygonal mesh_
	- [ ] VertexLayoutDescriptor
	- [ ] VertexBuffer extends Buffer
	---
	> next task is related to a buffer with generated vertices. To maximize the performance and resolve the unnecessary synchronization, we have to implement a double-buffering mechanism
	- [ ] Buffer -> upload data
	---
	- [x] FramebufferDescriptor,
	- [x] Framebuffer
	- [x] Texture
	- [x] TextureDescriptor
	---
	
- [x] pass render_context to sprite_batch :rocket: :rocket:
- [ ] in sprite_batch: :rocket:
	- [x] render color attachment from FBO to backbuffer
	- [x] embed screen coordinate to NDC matrix
		- [x] test it with mouse position tracing
- [ ] extend begin interface:
	- [x] add transform matrix
- [ ] extend draw interface :rocket:
	- [ ] rotation -> origin
	- [ ] scale
	- [ ] layer :rocket: :rocket: (see offline tasks)
	- [x] texture sampling
	- [x] flip -> horizontal, vertical, both
	- [ ] add sprites blending state :rocket:
	
	#### offline tasks (not related to sprite_batch)
	- [x] checkout tiled 2d integration
	- [x] checkout the box2d for physics and collision detection
	- [x] add debug visualization for box2d (using sprite_batch or imgui's draw list api)
	- [x] implement basic character controller 
	- [x] add controller/keyboard controls
	- [ ] add jump, attack animation and insert them into animation graph
	- [ ] camera controller -> smooth transitions, smooth zoom in/out
	- [ ] twinning library (create a custom one or take some existing one)
	- [ ] design a simple "stupid" ai behavior
	- [x] create abstract Game class
	- [ ] split framework and game/application
	- [ ] move animation data to game/application code
	---
	- [ ] build visually interesting level
	 > For this task we need a ability to render multilayered maps, so we have to handle correctly the depth parameter in sprite_batch. To make it more interesting we can also add some layers with real-time fake lighting by applying the color grading from LUT based on proximity to the light source. This technique require a custom FBO handling and writing of custom shader effects.
	---