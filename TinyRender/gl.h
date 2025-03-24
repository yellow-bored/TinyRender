#pragma once

#include"geometry.h"
#include"TgaImage.h"


extern Matrix viewport;
extern Matrix projection;
extern Matrix lookat;
extern Matrix scale;


void Viewport(int width, int height);
void Projection(float camera_z);
void Lookat(Vec3f eye_pos, Vec3f center, Vec3f up);
void Scale(float size);

class Shader {
public:
	virtual ~Shader() {}
	virtual Vec4f Vertex(int face, int vert) = 0; // ∂¡»°∂•µ„
	virtual bool Fragment(Vec3f bar, TgaColor& color) = 0; //∆¨∂Œ
};

void Triangle(Vec4f* pts, Shader& shader, TgaImage& image, float* zBuffer);