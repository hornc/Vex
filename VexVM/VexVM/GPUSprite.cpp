#include "GPUSprite.h"
#include <cmath>
#include <iostream>

GPUSprite::GPUSprite() {
	active = false;
	reset();
}

void GPUSprite::reset() {
	x = 0.0f;
	y = 0.0f;
	xscale = 1.0f;
	yscale = 1.0f;
	rot = 0.0f;
	dirty = true;
	datac = 0;
}

void GPUSprite::allocateLines(int newc) {
	if (newc > datac) {
		data = new Line[newc];
		data_out = new Line[newc];
	}
	datac = newc;
	dirty = true;
}

void GPUSprite::writeLine(int linei, float x1, float y1, float x2, float y2, float r, float g, float b) {
	data[linei].x1 = x1;
	data[linei].y1 = y1;
	data[linei].x2 = x2;
	data[linei].y2 = y2;
	data[linei].r = r;
	data[linei].g = g;
	data[linei].b = b;
	dirty = true;
}

void GPUSprite::loadImage(Image* image) {
	allocateLines(image->lines.size());
	for (int i = 0; i < datac; i++) {
		data[i].x1 = image->lines[i].x1;
		data[i].y1 = image->lines[i].y1;
		data[i].x2 = image->lines[i].x2;
		data[i].y2 = image->lines[i].y2;
		data[i].r = image->lines[i].r;
		data[i].g = image->lines[i].g;
		data[i].b = image->lines[i].b;
	}
	dirty = true;
}