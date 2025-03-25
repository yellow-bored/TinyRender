#include "TgaImage.h"
#include "model.h"
#include <algorithm>
#include <random>
#include <chrono>
#include <limits>
#include "gl.h"
#include "geometry.h"
#include <cmath>

const int WIDTH = 1000;
const int HEIGHT = 1000;
const int DEPTH = 255;

const float PI = std::acos(-1.0f);



// ����ϵ
Vec3f camera(0, -1, 2); // �������
Vec3f light_dir(1, 2, 3);
Vec3f center(0, 0, 0);
Vec3f up(0, 1, 0);

TgaImage image(WIDTH, HEIGHT, TgaImage::RGB);
TgaImage depth(WIDTH, HEIGHT, TgaImage::RGB);
float* zBuffer;
float* shadowBuffer;


/*
class GouraudShader :public Shader {
	Vec3f m_intensity; // ��¼����������ߣ�fragmen������ֵ
public:
	virtual Vec4f Vertex(int face, int vert) { // ���ؼ������Ļ�ϵ�����
		Vec4f m_vertex = embed<4>(model->vert(face, vert)); //	 ��λ�
		m_vertex = viewport * projection * scale * lookat * m_vertex; // ת������Ļ����
		m_intensity[vert] = std::max(0.0f,model->normal(face, vert) * light_dir.normalize()); // �������
		return m_vertex;
	}
	virtual bool Fragment(Vec3f bar, TgaColor& color) { // abr�������������������
		float intensity = bar * m_intensity;
		unsigned char c = 255 * intensity;
		//if (c >=255) std::cout << intensity;
		color = { c,c,c };
		return false; // ��ʾ��drop�˵㡣
	}
};

class TextureShader :public Shader {
	Vec3f m_intensity;
	Vec2f m_uv[3];
public:
	virtual Vec4f Vertex(int face, int vert) { // ���ؼ������Ļ�ϵ�����
		Vec4f m_vertex = embed<4>(model->vert(face, vert)); //	 ��λ�
		m_vertex = viewport * projection * scale * lookat * m_vertex; // ת������Ļ����
		m_intensity[vert] = std::max(0.0f, model->normal(face, vert).normalize() * light_dir.normalize()); // �������

		m_uv[vert] = model->uv(face, vert);
		return m_vertex;
	}
	virtual bool Fragment(Vec3f bar, TgaColor& color) { // abr�������������������
		float intensity = bar * m_intensity;
		Vec2f uv = m_uv[0] * bar.x + m_uv[1] * bar[1] + m_uv[2] * bar.z;
		color = model->diffuse(uv) * intensity;
		//std::cout <<intensity<<' ' << int(color[0]) << ' ' << int(color[1]) << ' ' << int(color[2]) << std::endl;
		//unsigned char c = 255 * intensity;
		return false; // ��ʾ��drop�˵㡣
	}

};

class NormalShader :public Shader {
	Vec3f m_intensity;
	Vec2f m_uv[3];
	mat<4, 4, float> m_M; // projection * scale * lookat ���ڼ��㷨��
public:
	virtual Vec4f Vertex(int face, int vert) { // ���ؼ������Ļ�ϵ�����
		Vec4f m_vertex = embed<4>(model->vert(face, vert)); //	 ��λ�
		m_vertex = viewport * projection * scale * lookat * m_vertex; // ת������Ļ����
		m_intensity[vert] = std::max(0.0f, model->normal(face, vert) * light_dir.normalize()); // �������
		m_uv[vert] = model->uv(face, vert);
		m_M = projection * scale * lookat;
		return m_vertex;
	}
	virtual bool Fragment(Vec3f bar, TgaColor& color) { // abr�������������������
		Vec2f uv = m_uv[0] * bar.x + m_uv[1] * bar[1] + m_uv[2] * bar.z;
		Vec3f n = proj<3>(m_M.invert_transpose() * embed<4>(model->normal(uv))).normalize();
		Vec3f l = proj<3>(m_M * embed<4>(light_dir.normalize()));
		float intensity = std::max(0.f,(n *l));

		color = model->diffuse(uv) * intensity;
		return false; // ��ʾ��drop�˵㡣
	}
};
// ambient+diffuse+specular
class PhoneShader :public Shader {
	Vec3f m_intensity;
	mat<2, 3, float> m_uv;
	mat<4, 4, float> m_M; // projection * scale * lookat ���ڼ��㷨��
	const float m_ambient = 5;
public:
	virtual Vec4f Vertex(int face, int vert) { // ���ؼ������Ļ�ϵ�����
		Vec4f m_vertex = embed<4>(model->vert(face, vert)); //	 ��λ�
		m_vertex = viewport * projection * scale * lookat * m_vertex; // ת������Ļ����
		m_intensity[vert] = std::max(0.0f, model->normal(face, vert) * light_dir.normalize()); // �������
		m_uv.set_col(vert, model->uv(face, vert));
		m_M = projection * scale * lookat;
		return m_vertex;
	}
	virtual bool Fragment(Vec3f bar, TgaColor& color) { // abr�������������������
		Vec2f uv = m_uv * bar;
		Vec3f n = proj<3>(m_M.invert_transpose() * embed<4>(model->normal(uv))).normalize();
		Vec3f l = proj<3>(m_M * embed<4>(light_dir)).normalize();
		Vec3f half_dir = (l + n).normalize();// �������
		float specular = std::pow(std::max(0.0f, n * half_dir), 32);
		float diffuse = std::max(0.0f, n * l);

		color = model->diffuse(uv);
		for (int i = 0; i < 3; ++i) {
			color[i] = std::min(255.f, color[i]*(0.6f * specular+diffuse) + m_ambient);
		}
		return false; // ��ʾ��drop�˵㡣
	}
};

class TBNShader :public Shader {
	// uv,normal,trixy
	mat<2, 3, float> m_uv;
	mat<3, 3, float>  m_normal;
	mat<3, 3, float> m_tri;
	Vec3f m_intensity;
	mat<4, 4, float> m_M; // projection * scale * lookat ���ڼ��㷨��
	const float m_ambient = 5;
public:
	virtual Vec4f Vertex(int face, int vert) { // ���ؼ������Ļ�ϵ�����
		Vec4f m_vertex = embed<4>(model->vert(face, vert)); //	 ��λ�
		m_M = projection * scale * lookat;

		m_uv.set_col(vert, model->uv(face, vert));
		m_normal.set_col(vert, proj<3>(m_M.invert_transpose() * embed<4>(model->normal(face, vert))));// �����仯��ķ���
		m_tri.set_col(vert, proj<3>(m_M * m_vertex/m_vertex[3])); //�仯��Ķ���


		m_vertex = viewport * projection * scale * lookat * m_vertex; // ת������Ļ����
		return m_vertex;
	}
	virtual bool Fragment(Vec3f bar, TgaColor& color) { // abr�������������������
		Vec2f uv = m_uv * bar;// ����
		Vec3f bn = (m_normal * bar).normalize();//����

		mat<3, 3, float>A;
		A.set_col(0, m_tri.col(1) - m_tri.col(0));
		A.set_col(1, m_tri.col(2) - m_tri.col(0));
		A.set_col(2, bn);

		A = A.invert_transpose();

		Vec3f T = A * Vec3f(m_uv[0][1] - m_uv[0][0], m_uv[0][2] - m_uv[0][0], 0.0f);
		Vec3f B = A * Vec3f(m_uv[1][1] - m_uv[1][0], m_uv[1][2] - m_uv[1][0], 0.0f);
		mat<3, 3, float> TBN;
		TBN[0] = T.normalize();
		TBN[1] =  B.normalize();
		TBN[2] = bn;

		Vec3f n = (TBN * model->normal(uv)).normalize();// ���߿ռ䷨��ת������ռ�
		Vec3f l = proj<3>(m_M * embed<4>(light_dir)).normalize();
		float diff = std::max(0.0f, n * light_dir);

		Vec3f half_dir = (l + n).normalize();// �������
		float specular = std::pow(std::max(0.0f, n * half_dir), 32);
		float diffuse = std::max(0.0f, n * l);

		color = model->diffuse(uv);
		for (int i = 0; i < 3; ++i) {
			color[i] = std::min(255.f, color[i] * (0.6f* specular + diffuse) + m_ambient);
		}

		return false;
	}
};
*/

class ShadowShader :public Shader {
	mat<3, 3, float> m_tri;
public:
	virtual Vec4f Vertex(int face, int vert, Model *model) { // ���ؼ������Ļ�ϵ�����
		Vec4f m_vertex = embed<4>(model->vert(face, vert)); //	 ��λ�

		m_vertex = viewport * projection * scale * lookat * m_vertex; // ת������Ļ����
		m_tri.set_col(vert, proj<3>(m_vertex / m_vertex[3]));
		//std::cout << m_vertex[0] << ' ' << m_vertex[1] << ' ' << m_vertex[2] << ' ' << m_vertex[3] << std::endl;

		return m_vertex;
	}
	virtual bool Fragment(Vec3f bar, TgaColor& color) { // abr�������������������
		Vec3f pt = m_tri * bar;
		color = { 1,1,1 };
		color = color * (pt.z);
		//std::cout << pt.z << std::endl;
		return false; // ��ʾ��drop�˵㡣
	}
};
class TShadowShader :public Shader {
	mat<2, 3, float> m_uv;
	mat<3, 3, float>  m_normal;
	mat<3, 3, float> m_tri;
	Vec3f m_intensity;
	mat<4, 4, float> m_M; // projection * scale * lookat ���ڼ��㷨��
	mat<4, 4, float> m_shadowM;
	const float m_ambient = 5;
	Model* m_model;
public:
	TShadowShader(Matrix s_m) :m_shadowM(s_m){}
	virtual Vec4f Vertex(int face, int vert, Model *model) { // ���ؼ������Ļ�ϵ�����
		m_model = model;
		Vec4f m_vertex = embed<4>(model->vert(face, vert)); //	 ��λ�
		
		m_M =   scale * lookat;

		m_uv.set_col(vert, model->uv(face, vert));
		m_normal.set_col(vert, proj<3>(m_M.invert_transpose() * embed<4>(model->normal(face, vert),0.0f)));// �����仯��ķ���
		m_tri.set_col(vert, proj<3>(m_vertex / m_vertex[3])); //�仯��Ķ���

		m_vertex = viewport * projection * scale * lookat * m_vertex; // ת������Ļ����
		return m_vertex;
	}
	virtual bool Fragment(Vec3f bar, TgaColor& color) { // abr�������������������
		Vec2f uv = m_uv * bar;// ����
		Vec3f bn = (m_normal * bar).normalize();//����
		Vec4f pt_s = viewport * m_shadowM * embed<4>(m_tri * bar);// ��������Ӱ��Ⱦʱ������

		pt_s = pt_s / pt_s[3];

		// �������Ӱ��Ⱦ�ֿ��Կ�����shadowΪ1��Ӱ�죬������Ϊ0.3��������Ӱ��
		/*
		mat<3, 3, float>A;
		A.set_col(0, m_tri.col(1) - m_tri.col(0));
		A.set_col(1, m_tri.col(2) - m_tri.col(0));
		A.set_col(2, bn);

		A = A.invert_transpose();

		Vec3f T = A * Vec3f(m_uv[0][1] - m_uv[0][0], m_uv[0][2] - m_uv[0][0], 0.0f);
		Vec3f B = A * Vec3f(m_uv[1][1] - m_uv[1][0], m_uv[1][2] - m_uv[1][0], 0.0f);
		mat<3, 3, float> TBN;
		TBN[0] = T.normalize();
		TBN[1] = B.normalize();
		TBN[2] = bn;
		*/
		//Vec3f n = (TBN * model->normal(uv)).normalize();// ���߿ռ䷨��ת������ռ�
		mat<4, 4, float> res;
		for (int i = 0; i < 4; ++i) {
			res.set_col(i, m_M[i]);
		}
		//std::cout << model->normal(uv)[0]<<' '<< model->normal(uv)[1]<<' '<< model->normal(uv)[2]<<std::endl;
		Vec3f n = proj<3>(m_M.invert_transpose() * embed<4>( m_model->normal(uv),0.0f)).normalize();
		Vec3f l = proj<3>(m_M * embed<4>(light_dir,0.0f)).normalize();

		//��Ӱ������
		float bias = std::max(0.05f * (1.0f - n * l), 0.005f);

		float shadow = 1.0f;
		if (int(pt_s[0]) + WIDTH * int(pt_s[1]) >= 0 && int(pt_s[0]) + WIDTH * int(pt_s[1])<=WIDTH*HEIGHT) {
			shadow = 0.3f + 0.7f * (shadowBuffer[int(pt_s[0]) + WIDTH * int(pt_s[1])] <= pt_s[2] + bias * 255.f);
		}
		Vec3f half_dir = (l + n).normalize();// �������
		float specular = std::pow(std::max(0.0f, n * half_dir), 32);
		float diffuse = std::max(0.0f, n * l);
		//std::cout << n << ' ' << l << ' ' << n * l << std::endl;

		color = m_model->diffuse(uv);
		for (int i = 0; i < 3; ++i) {
			color[i] = std::min(255.f, color[i] * shadow * (0.6f * specular + diffuse) + m_ambient);
		}
		return false;
	}
};


ShadowShader s_shader;

void LoadShadow(Model *model) {
	for (int i = 0; i < model->nfaces(); ++i) {
		Vec4f pts[3];
		for (int j = 0; j < 3; ++j) {
			pts[j] = s_shader.Vertex(i, j, model); // ��¼���㣬�Ѿ����������
		}
		Triangle(pts, s_shader, depth, shadowBuffer);
	}
}


void LoadVertex(Model *model, Matrix M) {
	TShadowShader shader(M);
	for (int i = 0; i < model->nfaces(); ++i) {
		//std::cout << i << " face" << std::endl;
		Vec4f pts[3];
		for (int j = 0; j < 3; ++j) {
			pts[j] = shader.Vertex(i, j, model); // ��¼���㣬�Ѿ����������
		}
		Triangle(pts, shader, image, zBuffer);
	}
}

float GetMaxAngle(Vec2f pt, int dir) {
	float angle_max = 0;
	Vec2f direction;
	direction.x = std::cos(dir * PI / 4.0f);
	direction.y = std::sin(dir * PI / 4.0f);
	for (int i = 0; i < 1000; ++i) {
		Vec2f pt_c = pt + direction * (i * 1.0f);
		if (pt_c.x < 0 || pt_c.y < 0 || pt_c.x >= WIDTH || pt_c.y >= HEIGHT) return angle_max;//���߽�ֱ�ӷ���
		float h = zBuffer[int(pt_c.x) + int(pt_c.y) * WIDTH] - zBuffer[int(pt.x) + int(pt.y) * WIDTH];
		float dis = std::sqrt((pt_c.x - pt.x) * (pt_c.x - pt.x) + (pt_c.y - pt.y) * (pt_c.y - pt.y));
		angle_max = std::max(angle_max, std::atanf(h / dis));
	}
	return angle_max;
}

int main() {

	// ����ģ��
	Model* model = NULL;
	Model* model2 = NULL;
	//model2 = new Model("obj/african_head/african_head.obj");
	//model = new Model("obj/floor.obj");
	//model = new Model("obj/boggie/head.obj");
	model2 = new Model("obj/diablo3_pose/diablo3_pose.obj");

	// ����zBuffer
	zBuffer = new float[WIDTH * HEIGHT];
	shadowBuffer = new float[WIDTH * HEIGHT];
	std::fill(zBuffer, zBuffer + WIDTH * HEIGHT, std::numeric_limits<float>::lowest());
	std::fill(shadowBuffer, shadowBuffer + WIDTH * HEIGHT, std::numeric_limits<float>::lowest());

	Lookat(light_dir, center, up); // ����Դλ����Ϊ�����������һ����Ⱦ
	Projection(0); // ������ͶӰ
	Viewport(WIDTH, HEIGHT);
	Scale(0.9f);
	light_dir.normalize();


	//LoadShadow(model);
	//LoadShadow(model2);

	//depth.Write_Tga_File("depth.tga");


	// ����ͶӰ��lookat��viewport,scale����
	Matrix M = projection * scale * lookat;

	Lookat(camera, center, up);
	Projection(-1.0f/(camera-center).norm());
	Viewport(WIDTH, HEIGHT);
	Scale(0.9f);

	//LoadVertex(model,M);
	LoadVertex(model2,M);

	for (int x = 0; x < WIDTH; ++x) {
		for (int y = 0; y < HEIGHT; ++y) {
			if (zBuffer[x + y * WIDTH] < -1e5) continue;// �����Ǳ���
			float irate = 0; //����Χ�ڽ��ܵ��Ĺ���
			for (int k = 0; k < 8; ++k) {
				irate += GetMaxAngle(Vec2f(x, y), k);
			}
			irate = 1 - irate / PI / 4;
			irate = std::pow(irate, 100);
			TgaColor c = { irate * 255,irate * 255,irate * 255 };
			image.Set(x, y, c);
		}
	}

	image.Write_Tga_File("output.tga");
	delete model;
	delete model2;
	return 0;
}