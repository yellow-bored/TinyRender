#include "TgaImage.h"
#include "model.h"
#include <algorithm>
#include <random>
#include <chrono>
#include <limits>
const TgaColor blue = TgaColor{ 255,0,0,255 };
const TgaColor white = TgaColor{ 255,255,255,255 };

const int WIDTH = 1000;
const int HEIGHT = 1000;
const int DEPTH = 255;

Model* model = NULL;

// 坐标系
Vec3f camera(0.5, 1, 2); // 相机坐标


void line(int x0, int y0, int x1, int y1, TgaColor color, TgaImage& image) {
	bool is_steep = false; // 判断斜率是都大于一，因为是一个x方向像素对应一个y方向像素、
						//斜率很大的时候，理论上一个x方向像素对应多个y像素
						//也与对称性有些关系
	if (std::abs(x1 - x0) < std::abs(y1 - y0)) {
		std::swap(x0, y0);
		std::swap(x1, y1);
		is_steep = true;
	}
	if (x0 > x1) {
		std::swap(x0, x1);
		std::swap(y0, y1);
	}
	int dx = x1 - x0;
	int dy = y1 - y0;
	int error = 0;
	int derror = std::abs(dy) * 2;
	int y = y0;
	int step_y = (y0 < y1) ? 1 : -1;
	for (int x = x0; x <= x1; ++x) {
		if (is_steep) {
			image.Set(y, x, color);
		}
		else {
			image.Set(x, y, color);
		}
		error += derror;
		if (error > dx) {
			y += step_y;
			error -= dx * 2;
		}
	}
}
void Triangle_sweep(int x0, int y0, int x1, int y1, int x2, int y2, TgaColor color, TgaImage& image) {

	if (x0 > x1) { std::swap(x0, x1); std::swap(y0, y1); }
	if (x1 > x2) { std::swap(x1, x2); std::swap(y1, y2); }
	if (x0 > x1) { std::swap(x0, x1); std::swap(y0, y1); }
	for (int i = x0; i < x1; ++i) {
		int y_u = y0 + (y1 - y0) * (i-x0) / (x1 - x0);
		int y_b = y0 + (y2 - y0) * (i-x0) / (x2 - x0);
		if (y_u < y_b) {
			std::swap(y_u, y_b);
		}
		for (int j = y_b; j <= y_u; ++j) {
			image.Set(i, j, color);
		}
	}
	for (int i = x2; i >= x1; --i) {
		int y_u = y2 + (y2 - y1) * (i-x2) / (x2 - x1);
		int y_b = y2 + (y2 - y0) * (i-x2) / (x2 - x0);
		if (y_u < y_b) {
			std::swap(y_u, y_b);
		}
		for (int j = y_b; j <= y_u; ++j) {
			image.Set(i, j, color);
		}
	}
}

// 齐次化
Matrix V2M(Vec3f v) {
	Matrix m(4, 1);
	m[0][0] = v.x;
	m[1][0] = v.y;
	m[2][0] = v.z;
	m[3][0] = 1.0f;
	return m;
}

Vec3f M2V(Matrix m) {
	Vec3f v(m[0][0] / m[3][0], m[1][0] / m[3][0], m[2][0] / m[3][0]);
	return v;
}

Matrix viewport() {
	Matrix m = Matrix::identity(4);
	m[0][3] = WIDTH / 2.0f;
	m[1][3] = HEIGHT / 2.0f;
	m[2][3] = DEPTH / 2.0f;

	m[0][0] = WIDTH / 2.0f;
	m[1][1] = HEIGHT / 2.0f;
	m[2][2] = DEPTH / 2.0f;

	return m;
}
// 三个点算面积
float Cal_S(Vec3f Pt1, Vec3f Pt2, Vec3f Pt3) {
	return (Pt2.x - Pt1.x) * (Pt3.y - Pt1.y) - (Pt3.x - Pt1.x) * (Pt2.y - Pt1.y);
}

// 重心坐标
Vec3f Barycentric(Vec3f Pt1, Vec3f Pt2, Vec3f Pt3, Vec3f Pt0) {
	Vec3f bc; // 存abc值
	float S = Cal_S(Pt1, Pt2, Pt3);
	bc.y = Cal_S(Pt0, Pt3, Pt1) / S;
	bc.z = Cal_S(Pt0, Pt1, Pt2) / S;
	bc.x = 1 - bc.y - bc.z;
	return bc;
}


TgaColor Shadow(double z) {
	//std::cout << z << std::endl;
	int c = ((z+1.0f)/2.0f) * 255.f;
	TgaColor res = { c,c,c,255 };
	return res;
}

void Triangle(Vec3f pt1, Vec3f pt2, Vec3f pt3, Vec2i uv1, Vec2i uv2, Vec2i uv3, TgaImage& image, float* zBuffer) {
	
	//背面剔除
	//if ((x1 - x0) * (y2 - y0) - (x2 - x0) * (y1 - y0) < 0) {
	//	return;
	//}
	
	int x_l = std::min(std::min(pt1.x, pt2.x), pt3.x);
	int x_r = std::max(std::max(pt1.x, pt2.x), pt3.x);
	int y_b = std::min(std::min(pt1.y, pt2.y), pt3.y);
	int y_u = std::max(std::max(pt1.y, pt2.y), pt3.y);

	for (int i = x_l; i <= x_r; ++i) { 
		for (int j = y_b; j <= y_u; ++j) {
			Vec3f Pt0(i, j, 0);
			Vec3f bc = Barycentric(pt1, pt2, pt3, Pt0); // 重心坐标
			if (bc.x >= 0 && bc.y >= 0 && bc.z >= 0) { // Inside Triangle
				Pt0.z = bc.x * pt1.z + bc.y * pt2.z + bc.z * pt3.z;

				// 纹理坐标插值
				int x = uv1.x * bc.x + uv2.x * bc.y + uv3.x * bc.z;
				int y = uv1.y * bc.x + uv2.y * bc.y + uv3.y * bc.z;
				Vec2i uv(x, y);
				TgaColor Tex = model->diffuse(uv);
				if (Pt0.z > zBuffer[j * WIDTH + i]) {
					zBuffer[j * WIDTH + i] = Pt0.z;
					//TgaColor Tex = model->diffuse(bc_uv_i);
					image.Set(i, j, Tex);
				}

			}

		}
	}
}

TgaColor RandomColor() {
	std::mt19937 e(std::chrono::system_clock::now().time_since_epoch().count());
	std::uniform_int_distribution<int> u(0, 255);
	TgaColor res = { u(e),u(e),u(e),255 };
	return res;
}

void Trans(Matrix &m) {
	for (int i = 0; i < m.ncols(); ++i) {
		for (int j = i + 1; j < m.nrows(); ++j) {
			std::swap(m[i][j], m[j][i]);
		}
	}
}

Vec3f World2Creame(Vec3f v) {
	return Vec3f(int((v.x + 1.0f) * WIDTH / 2.0f - 0.5f), int((v.y + 1.0f) * HEIGHT / 2.0f - 0.5f), v.z);
}

// 摄像机的变化
// 摄像机是始终沿z轴负方向看向原点的
//eye_pos就是摄像机位置，center是看向的坐标，up是变换后的（0，1，0）
//对应旋转，				对应平移
// 摄像机先旋转后平移，取逆后就是先平移后旋转。
// eye_pos - center = z，再根据z算xy
Matrix Lookat(Vec3f eye_pos, Vec3f center, Vec3f up) {
	Vec3f z = (eye_pos - center).normalize();
	Vec3f x = (up ^ z).normalize();
	Vec3f y = (z ^ x).normalize();
	Matrix rotation = Matrix::identity(4);
	Matrix translation = Matrix::identity(4);
	
	for (int i = 0; i < 3; ++i) {
		rotation[i][0] = x[i];
		rotation[i][1] = y[i];
		rotation[i][2] = z[i];
	}
	Trans(rotation);
	// translation不是正交矩阵，直接求逆
	for (int i = 0; i < 3; ++i) {
		translation[i][3] = -center[i];
	}
	// 先平移后旋转
	Matrix res = rotation * translation;
	return res;
}


int main() {
	TgaImage image(WIDTH, HEIGHT, TgaImage::RGB);
	float* zBuffer = new float[WIDTH * HEIGHT];
	std::fill(zBuffer, zBuffer + WIDTH * HEIGHT, std::numeric_limits<float>::lowest());
	
	//model = new Model("obj/african_head/african_head.obj");
	model = new Model("obj/floor.obj");
	//std::cout <<"infomation" << model->nfaces() << std::endl;
	for (int i = 0; i < model->nfaces(); ++i) {
		// 顶点坐标
		std::vector<int> face = model->face(i);
		Vec3f v0 = model->vert(face[0]);
		Vec3f v1 = model->vert(face[1]);
		Vec3f v2 = model->vert(face[2]);

		Matrix Projection = Matrix::identity(4);
		Projection[3][2] = -1.0f / camera.z;
		Matrix Viewport = viewport(); // 世界坐标到相机坐标
		Matrix Scale = Matrix::identity(4);
		Scale[0][0] = 0.9f;
		Scale[1][1] = 0.9f;
		Scale[2][2] = 0.9f;
		Vec3f center(0, 0, 1);
		Vec3f up(0, 1, 0);
		Matrix lookat = Lookat(camera, center, up);
		v0 = M2V(Viewport *  Projection * Scale * lookat * V2M(v0));
		v1 = M2V(Viewport *  Projection * Scale * lookat * V2M(v1));
		v2 = M2V(Viewport *  Projection * Scale * lookat * V2M(v2));
	//	v0 = World2Creame(v0);
	//	v1 = World2Creame(v1);
	//	v2 = World2Creame(v2);
		
		// 纹理坐标
		Vec2i uv0 = model->uv(i, 0);
		Vec2i uv1 = model->uv(i, 1);
		Vec2i uv2 = model->uv(i, 2);

		Triangle(v0, v1, v2, uv0, uv1, uv2, image, zBuffer);
	}
	
	image.Write_Tga_File("output.tga");
	return 0;
}