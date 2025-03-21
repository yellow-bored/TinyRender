#include "TgaImage.h"
#include "model.h"
#include <algorithm>
#include <random>
#include <chrono>
#include <limits>
#include "gl.h"
#include "geometry.h"

const int WIDTH = 1000;
const int HEIGHT = 1000;
const int DEPTH = 255;

Model* model = NULL;

// 坐标系
Vec3f camera(1, 1, 3); // 相机坐标
Vec3f light_dir(1, 1, 1);
Vec3f center(0, 0, 0);
Vec3f up(0, 1, 0);

class GouraudShader :public Shader {
	Vec3f m_intensity; // 记录三个顶点光线，fragmen内做插值
public:
	virtual Vec4f Vertex(int face, int vert) { // 返回计算后屏幕上的坐标
		Vec4f m_vertex = embed<4>(model->vert(face, vert)); //	 齐次化
		m_vertex = viewport * projection * scale * lookat * m_vertex; // 转换到屏幕坐标
		m_intensity[vert] = std::max(0.0f,model->normal(face, vert) * light_dir.normalize()); // 计算光照
		return m_vertex;
	}
	virtual bool Fragment(Vec3f bar, TgaColor& color) { // abr是重心坐标的三个参数
		float intensity = bar * m_intensity;
		unsigned char c = 255 * intensity;
		//if (c >=255) std::cout << intensity;
		color = { c,c,c };
		return false; // 表示不drop此点。
	}
};

class TextureShader :public Shader {
	Vec3f m_intensity;
	Vec2f m_uv[3];
public:
	virtual Vec4f Vertex(int face, int vert) { // 返回计算后屏幕上的坐标
		Vec4f m_vertex = embed<4>(model->vert(face, vert)); //	 齐次化
		m_vertex = viewport * projection * scale * lookat * m_vertex; // 转换到屏幕坐标
		m_intensity[vert] = std::max(0.0f, model->normal(face, vert) * light_dir.normalize()); // 计算光照

		m_uv[vert] = model->uv(face, vert);
		return m_vertex;
	}
	virtual bool Fragment(Vec3f bar, TgaColor& color) { // abr是重心坐标的三个参数
		float intensity = bar * m_intensity;
		Vec2f uv = m_uv[0] * bar.x + m_uv[1] * bar[1] + m_uv[2] * bar.z;
		color = model->diffuse(uv) * intensity;
		//unsigned char c = 255 * intensity;
		return false; // 表示不drop此点。
	}

};

// Todo:: 法线，phone

class NormalShader :public Shader {
	Vec3f m_intensity;
	Vec2f m_uv[3];
	mat<4, 4, float> m_M; // projection * scale * lookat 用于计算法线
public:
	virtual Vec4f Vertex(int face, int vert) { // 返回计算后屏幕上的坐标
		Vec4f m_vertex = embed<4>(model->vert(face, vert)); //	 齐次化
		m_vertex = viewport * projection * scale * lookat * m_vertex; // 转换到屏幕坐标
		m_intensity[vert] = std::max(0.0f, model->normal(face, vert) * light_dir.normalize()); // 计算光照
		m_uv[vert] = model->uv(face, vert);
		m_M = projection * scale * lookat;
		return m_vertex;
	}
	virtual bool Fragment(Vec3f bar, TgaColor& color) { // abr是重心坐标的三个参数
		Vec2f uv = m_uv[0] * bar.x + m_uv[1] * bar[1] + m_uv[2] * bar.z;
		Vec3f n = proj<3>(m_M.invert_transpose() * embed<4>(model->normal(uv))).normalize();
		Vec3f l = proj<3>(m_M * embed<4>(light_dir.normalize()));
		float intensity = std::max(0.f,(n *l));

		color = model->diffuse(uv) * intensity;
		return false; // 表示不drop此点。
	}
};

// ambient+diffuse+specular
class PhoneShader :public Shader {
	Vec3f m_intensity;
	Vec2f m_uv[3];
	mat<4, 4, float> m_M; // projection * scale * lookat 用于计算法线
	const float m_ambient = 5;
public:
	virtual Vec4f Vertex(int face, int vert) { // 返回计算后屏幕上的坐标
		Vec4f m_vertex = embed<4>(model->vert(face, vert)); //	 齐次化
		m_vertex = viewport * projection * scale * lookat * m_vertex; // 转换到屏幕坐标
		m_intensity[vert] = std::max(0.0f, model->normal(face, vert) * light_dir.normalize()); // 计算光照
		m_uv[vert] = model->uv(face, vert);
		m_M = projection * scale * lookat;
		return m_vertex;
	}
	virtual bool Fragment(Vec3f bar, TgaColor& color) { // abr是重心坐标的三个参数
		Vec2f uv = m_uv[0] * bar.x + m_uv[1] * bar[1] + m_uv[2] * bar.z;
		Vec3f n = proj<3>(m_M.invert_transpose() * embed<4>(model->normal(uv))).normalize();
		Vec3f l = proj<3>(m_M * embed<4>(light_dir.normalize()));
		Vec3f half_dir = (l + n).normalize();// 半程向量
		float specular = std::pow(std::max(0.0f, n * half_dir), 32);
		float diffuse = std::max(0.0f, n * l);

		color = model->diffuse(uv);
		for (int i = 0; i < 3; ++i) {
			color[i] = std::min(255.f, color[i]*(specular+diffuse) + m_ambient);
		}
		return false; // 表示不drop此点。
	}
};
int main() {
	TgaImage image(WIDTH, HEIGHT, TgaImage::RGB);
	TgaImage zBuffer(WIDTH, HEIGHT, TgaImage::GRAYSCALE);

	model = new Model("obj/african_head/african_head.obj");
	//model = new Model("obj/floor.obj");

	// 计算投影，lookat，viewport,scale矩阵
	Lookat(camera, center, up);
	Projection((camera-center).norm());
	Viewport(WIDTH, HEIGHT);
	Scale(0.9f);

	// 新建shader
	PhoneShader shader;

	for (int i = 0; i < model->nfaces(); ++i) {
		//std::cout << i << " face" << std::endl;
		Vec4f pts[3];
		for (int j = 0; j < 3; ++j) {
			pts[j] = shader.Vertex(i, j); // 记录顶点，已经是齐次坐标
		}
		Triangle(pts, shader, image, zBuffer);
	}

	image.Write_Tga_File("output.tga");
	delete model;
	return 0;
}