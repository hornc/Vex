#pragma once

#include "Line.h"
#include "Image.h"
#include <cmath>

// Internal GPU sprite data, for copying into VBO
// Pointed to by Sprite

class GPUSprite {
public:
	bool active;

	GPUSprite();
	void reset();
	void loadImage(Image* image);
	void PushData(float dataArray[], int* counter);
	void update();

	float x, y;
	float xscale, yscale;
	float rot;

	bool dirty;
	Line* data;
	Line* data_out;
	int datac;

private:
};

struct GPUSpriteTicket {
	GPUSprite* gpuSprite;
	int gpuSpriteID;
};

inline void GPUSprite::update() {
	float cos = std::cos(rot);
	float sin = std::sin(rot);
	float x1, y1, x2, y2;
	int i = 0;
	while (i < datac) {
		x1 = xscale * data[i].x1;
		y1 = yscale * data[i].y1;
		x2 = xscale * data[i].x2;
		y2 = yscale * data[i].y2;
		data_out[i].x1 = (x1 * cos - y1 * sin) + x;
		data_out[i].y1 = (y1 * cos + x1 * sin) + y;
		data_out[i].x2 = (x2 * cos - y2 * sin) + x;
		data_out[i].y2 = (y2 * cos + x2 * sin) + y;
		data_out[i].r = data[i].r;
		data_out[i].g = data[i].g;
		data_out[i].b = data[i].b;
		i++;
	}
	dirty = false;
}

inline void GPUSprite::PushData(float dataArray[], int* counter) {
	if (dirty) update();
	int i = 0;
	while (i < datac) {
		data_out[i].PushData(dataArray, counter);
		i++;
	}
}
