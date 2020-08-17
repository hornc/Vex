#include "GPU.h"
#include <glad/glad.h>
#include <GLFW\glfw3.h>
#include <iostream>
#include "Framebuffer.h"
#include "util.cpp"

GPU::GPU() { }

GPU::GPU(int w, int h) {
	this->w = w;
	this->h = h;
	points = new Point[settings.MAX_POINTS];
	lines = new Line[settings.MAX_LINES];
	sprites = new GPUSprite[settings.MAX_SPRITES];
	scaledscreen = new float[24];
	std::cout << "GPU created" << std::endl;
}

void GPU::Resize(int w, int h) {
	std::cout << "window scaled to " << w << "," << h << std::endl;
	this->w = w;
	this->h = h;
	// Calculate the new screen w/h based on aspect ratio
	screenw = h * settings.ASPECT_RATIO;
	screenh = h;
	float scale = std::min((float)w / (float)screenw, (float)h / (float)screenh);
	screenw = scale * screenw;
	screenh = scale * screenh;
	// Resize the screen vertices to match the ratio on the new window size
	float xscale = (float)screenw / (float)w;
	float yscale = (float)screenh / (float)h;
	for (int i = 0; i < 12; i++) {
		float x = screendata[i];
		float y = screendata[i + 1];
		scaledscreen[i * 2] = screendata[i * 2] * xscale;
		scaledscreen[i * 2 + 1] = screendata[i * 2 + 1] * yscale;
	}
	std::cout << "screen scaled to " << screenw << "," << screenh << std::endl;
	makeBuffers();
}

void GPU::loadImage(Image* image) {
	images.push_back(image);
}
void GPU::loadFont(Font* font) {
	fonts.push_back(font);
}

int GPU::reserveSprite() {
	int id = 0;
	while ((id < settings.MAX_SPRITES) && sprites[id].active) { id++; }
	if (id == settings.MAX_SPRITES) throw "too many sprites";
	if (id >= spritec) spritec = id + 1;
	sprites[id].active = true;
	return id;
}

GPUSpriteTicket GPU::createSprite(int image) {
	int id = reserveSprite();
	GPUSprite* sprite = &sprites[id];
	sprite->loadImage(images.at(image));

	GPUSpriteTicket ticket;
	ticket.gpuSprite = sprite;
	ticket.gpuSpriteID = id;
	return ticket;
}

GPUSpriteTicket GPU::createText(int font, std::string* text, float r, float g, float b) {
	int id = reserveSprite();
	GPUSprite* sprite = &sprites[id];
	sprite->allocateLines(fonts[font]->countLinesInText(text));

	// draw glyphs into sprite
	int linec = 0;
	float hoff = 0.0f;
	for (char& c : *text) {
		std::cout << "render char " << c << std::endl;
		if (c == ' ') {
			hoff += 0.4f;
		} else if (fonts.at(font)->glyphs.count(c) > 0) {
			std::vector<Line>* glyph = fonts.at(font)->glyphs[c];
			float minx = 0.0f;
			float maxx = 0.0f;
			for (int j = 0; j < glyph->size(); j++) {
				Line* l = &glyph->at(j);
				sprite->writeLine(linec, l->x1 + hoff, l->y1, l->x2 + hoff, l->y2, r, g, b);
				linec++;
				if (l->x1 < minx) { minx = l->x1; }
				if (l->x1 > maxx) { maxx = l->x1; }
				if (l->x2 < minx) { minx = l->x2; }
				if (l->x2 > maxx) { maxx = l->x2; }
			}
			hoff += (maxx - minx) + 0.1f;
		}
	}

	GPUSpriteTicket ticket;
	ticket.gpuSprite = sprite;
	ticket.gpuSpriteID = id;
	return ticket;
}

void GPU::destroySprite(int id) {
	sprites[id].active = false;
}

Point* GPU::addPoint(float x, float y, float r, float g, float b, float size) {
	if (pointc >= settings.MAX_POINTS) throw "GPU point memory full!";
	points[pointc] = Point(x, y, r, g, b, size);
	pointc++;
	return &points[pointc - 1];
}

Line* GPU::addLine(float x1, float y1, float x2, float y2, float r, float g, float b) {
	if (linec >= settings.MAX_LINES) throw "GPU line memory full!";
	lines[linec] = Line(x1, y1, x2, y2, r, g, b);
	linec++;
	return &lines[linec - 1];
}


// Remove all prims from draw lists
void GPU::Reset() {
	pointc = 0;
	linec = 0;
	spritec = 0;
}

void GPU::makeShaders() {
	std::cout << "Compiling shaders..." << std::endl;
	pointShader = Shader("./data/shaders/drawpoint.vert", "./data/shaders/drawpoint.frag");
	pointShader.Load();
	lineShader = Shader("./data/shaders/drawline.vert", "./data/shaders/drawline.frag");
	lineShader.Load();
	blitShader = Shader("./data/shaders/blit.vert", "./data/shaders/blit.frag");
	blitShader.Load();
	blitShader.SetUniform("screenTexture", 0);
	fadeShader = Shader("./data/shaders/fade.vert", "./data/shaders/fade.frag");
	fadeShader.Load();
	fadeShader.SetUniform("inTexture", 0);
	composeShader = Shader("./data/shaders/compose.vert", "./data/shaders/compose.frag");
	composeShader.Load();
	composeShader.SetUniform("screenTex", 0);
	composeShader.SetUniform("glowTex", 1);
	composeShader.SetUniform("trailTex", 2);
	blurShader = Shader("./data/shaders/blur.vert", "./data/shaders/blur.frag");
	blurShader.Load();
	blurShader.SetUniform("texture", 0);
}

void GPU::makeBuffers() {
	std::cout << "Allocating framebuffers..." << std::endl;
	screenBuffer = Framebuffer(screenw, screenh);
	screenBuffer.Create();
	trailBuffer = Framebuffer(screenw, screenh);
	trailBuffer.Create();
	trailBuffer.Clear(0.0f, 0.0f, 0.0f, 1.0f);
	glowBuffer = Framebuffer(screenw, screenh);
	glowBuffer.Create();
}

void GPU::makeVBs() {
	std::cout << "Allocating vertex buffers..." << std::endl;
	pointsVB = Vertbuffer(GL_POINTS, settings.MAX_POINTS, GL_STREAM_DRAW);
	pointsVB.Attribute(2, GL_FLOAT); // pos
	pointsVB.Attribute(3, GL_FLOAT); // color
	pointsVB.Attribute(1, GL_FLOAT); // size
	pointsVB.Create();
	linesVB = Vertbuffer(GL_LINES, settings.MAX_LINES, GL_STREAM_DRAW);
	linesVB.Attribute(2, GL_FLOAT); // pos
	linesVB.Attribute(3, GL_FLOAT); // color
	linesVB.Create();
	screenVB = Vertbuffer(GL_TRIANGLES, 2, GL_STATIC_DRAW);
	screenVB.Attribute(2, GL_FLOAT); // pos
	screenVB.Attribute(2, GL_FLOAT); // uv
	screenVB.Create();
	bufferVB = Vertbuffer(GL_TRIANGLES, 2, GL_STATIC_DRAW);
	bufferVB.Attribute(2, GL_FLOAT); // pos
	bufferVB.Attribute(2, GL_FLOAT); // uv
	bufferVB.Create();
}

GLFWwindow* GPU::Setup(void (*onResize)(GLFWwindow* window, int neww, int newh)) {
	std::cout << "GPU: GLFW initializing" << std::endl;
	glfwInit();

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	glfwWindowHint(GLFW_FOCUS_ON_SHOW, GLFW_TRUE);
	glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);
	window = glfwCreateWindow(w, h, "VEX", NULL, NULL);
	if (window == NULL) {
		glfwTerminate();
		std::cout << "GPU: GLFW init failed!" << std::endl;
		throw "GLFW init failed!";
	}
	glfwMakeContextCurrent(window);
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "GPU: Failed to initialize GLAD" << std::endl;
		throw "GLFW init failed!";
	}
	glfwSetFramebufferSizeCallback(window, onResize);
	std::cout << "GPU: GLFW window initialized" << std::endl;

	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_PROGRAM_POINT_SIZE);
	glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
	glDepthMask(false);
	glDisable(GL_DEPTH_TEST);

	makeShaders();
	Resize(w, h);  // this also makes the framebuffers
	makeVBs();

	Reset();
	std::cout << "GPU initialized" << std::endl;
	return window;
}

void GPU::Stop() {
	glfwTerminate();
}

// Copy all our abstract prims into vertex buffers, to prep for render
void GPU::Assemble() {
	pointsVB.Reset();
	linesVB.Reset();
	for (int i = 0; i < pointc; i++) {
		points[i].PushData(pointsVB.vertdata, &pointsVB.vertdatac);  // internals exposed for speed
	}
	for (int i = 0; i < linec; i++) {
		lines[i].PushData(linesVB.vertdata, &linesVB.vertdatac);
	}
	for (int i = 0; i < spritec; i++) {
		if (sprites[i].active) {
			sprites[i].PushData(linesVB.vertdata, &linesVB.vertdatac);
		}
	}
	pointsVB.Update();
	linesVB.Update();
	bufferVB.BulkLoad(screendata, 24);
	screenVB.BulkLoad(scaledscreen, 24);
}

void GPU::drawPrims(float pointThick, float lineThick, float pointBright, float lineBright) {
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	glLineWidth(lineThick);

	pointShader.Use("brightness", pointBright);
	pointShader.SetUniform("time", (float)glfwGetTime());
	pointShader.SetUniform("spread", settings.POINT_SPREAD);
	pointShader.SetUniform("stability", settings.POINT_STABILITY);
	pointShader.SetUniform("sustain", settings.BEAM_SUSTAIN);
	pointShader.SetUniform("drop", settings.BEAM_DROP);
	pointShader.SetUniform("thickness", pointThick);
	pointsVB.Draw();

	lineShader.Use("brightness", lineBright);
	lineShader.SetUniform("time", (float)glfwGetTime());
	lineShader.SetUniform("spread", settings.LINE_SPREAD);
	lineShader.SetUniform("stability", settings.LINE_STABILITY);
	lineShader.SetUniform("sustain", settings.BEAM_SUSTAIN);
	lineShader.SetUniform("drop", settings.BEAM_DROP);
	linesVB.Draw();

	glDisable(GL_BLEND);
}

void GPU::Render() {
	glViewport(0, 0, screenw, screenh);

	// Draw to trailbuffer, then blur it
	trailBuffer.Target();
	if (settings.DRAW_TRAILS) drawPrims(settings.POINT_THICKNESS, settings.LINE_THICKNESS, settings.POINT_TRAIL_BRIGHTNESS, settings.LINE_TRAIL_BRIGHTNESS);
	blurShader.Use();
	trailBuffer.BindAsTexture(GL_TEXTURE0, blurShader, "texture", 0);
	blurShader.Blur(bufferVB, screenw, screenh, settings.TRAIL_BLUR);

	// Fade trailbuffer
	trailBuffer.Target();
	trailBuffer.BindAsTexture(GL_TEXTURE0, fadeShader, "texture", 0);
	fadeShader.Use("decay", settings.TRAIL_DECAY);
	trailBuffer.Blit(fadeShader, bufferVB);

	// Draw to glowbuffer, then blur it
	glowBuffer.Target();
	glowBuffer.Clear(0.0f, 0.0f, 0.0f, 0.0f);
	if (settings.DRAW_GLOW) drawPrims(settings.POINT_GLOW_THICKNESS, settings.LINE_GLOW_THICKNESS, settings.POINT_GLOW_BRIGHTNESS, settings.LINE_GLOW_BRIGHTNESS);
	glowBuffer.Target();
	blurShader.Use();
	glowBuffer.BindAsTexture(GL_TEXTURE0, blurShader, "texture", 0);
	blurShader.Blur(bufferVB, screenw, screenh, 0.5f);
	blurShader.Blur(bufferVB, screenw, screenh, 1.0f);
	blurShader.Blur(bufferVB, screenw, screenh, 2.0f);

	// Draw to screenbuffer
	screenBuffer.Target();
	screenBuffer.Clear(0.0f, 0.0f, 0.0f, 1.0f);
	if (settings.DRAW_SCREEN) drawPrims(settings.POINT_THICKNESS, settings.LINE_THICKNESS, settings.POINT_BRIGHTNESS, settings.LINE_BRIGHTNESS);

	// Compose to screen
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f); // letterbox
	glClear(GL_COLOR_BUFFER_BIT);
	glViewport(0, 0, w, h);
	composeShader.Use();
	screenBuffer.BindAsTexture(GL_TEXTURE0, composeShader, "screenTex", 0);
	glowBuffer.BindAsTexture(GL_TEXTURE1, composeShader, "glowTex", 1);
	trailBuffer.BindAsTexture(GL_TEXTURE2, composeShader, "trailTex", 2);
	screenVB.Draw();

	// Finish the frame
	glfwSwapBuffers(window);
	glfwPollEvents();
}

void GPU::toggleLayer(int layer) {
	if (layer == 0) settings.DRAW_SCREEN = (settings.DRAW_SCREEN ? false : true);
	if (layer == 1) settings.DRAW_GLOW = (settings.DRAW_GLOW ? false : true);
	if (layer == 2) settings.DRAW_TRAILS = (settings.DRAW_TRAILS ? false : true);
}
