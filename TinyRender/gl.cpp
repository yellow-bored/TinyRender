#include"gl.h"

Matrix viewport;
Matrix projection;
Matrix lookat;
Matrix scale;

void Viewport(int width, int height) {
	viewport = Matrix::identity();
	viewport[0][3] = width / 2.0f;
	viewport[1][3] = height / 2.0f;
	viewport[2][3] = 255.0f / 2.0f;
	
	viewport[0][0] = width / 2.0f;
	viewport[1][1] = height / 2.0f;
	viewport[2][2] = 255.0f / 2.0f;
}	

void Projection(float camera_z) {
	projection = Matrix::identity();
	projection[3][2] = camera_z;
}

void Lookat(Vec3f eye_pos, Vec3f center, Vec3f up) {
	Vec3f z = (eye_pos - center).normalize();
	Vec3f x = cross(up, z).normalize();
	Vec3f y = cross(z, x).normalize();
	Matrix rotation = Matrix::identity();
	Matrix translation = Matrix::identity();

	for (int i = 0; i < 3; ++i) {
		rotation[0][i] = x[i];
		rotation[1][i] = y[i];
		rotation[2][i] = z[i];
	}
	for (int i = 0; i < 3; ++i) {
		translation[i][3] = -eye_pos[i];
	}
	// 先平移后旋转
	lookat = rotation * translation;
}
void Scale(float size) {
	scale = Matrix::identity();
	scale[0][0] = size;
	scale[1][1] = size;
	scale[2][2] = size;
	//scale[3][3] = 1;
}

// 三个点算面积
float Cal_S(Vec2f Pt1, Vec2f Pt2, Vec2f Pt3) {
	return (Pt2.x - Pt1.x) * (Pt3.y - Pt1.y) - (Pt3.x - Pt1.x) * (Pt2.y - Pt1.y);
}

// 重心坐标
Vec3f Barycentric(Vec2f Pt1, Vec2f Pt2, Vec2f Pt3, Vec2f Pt0) {
	Vec3f bc; // 存abc值
	float S = Cal_S(Pt1, Pt2, Pt3);
	bc.y = Cal_S(Pt0, Pt3, Pt1) / S;
	bc.z = Cal_S(Pt0, Pt1, Pt2) / S;
	bc.x = 1 - bc.y - bc.z;
	return bc;
}

void Triangle(Vec4f* pts, Shader& shader, TgaImage& image, float* zBuffer) {
	float x_l = std::min(std::min(pts[0][0] / pts[0][3], pts[1][0] / pts[1][3]), pts[2][0] / pts[2][3]);
	float x_r = std::max(std::max(pts[0][0] / pts[0][3], pts[1][0] / pts[1][3]), pts[2][0] / pts[2][3]);
	float y_b = std::min(std::min(pts[0][1] / pts[0][3], pts[1][1] / pts[1][3]), pts[2][1] / pts[2][3]);
	float y_u = std::max(std::max(pts[0][1] / pts[0][3], pts[1][1] / pts[1][3]), pts[2][1] / pts[2][3]);
	x_l = std::min(std::max(x_l, 0.0f), 1.0f*image.Width());
	x_r = std::max(0, std::min(int(x_r), image.Width() - 1));
	y_b = std::min(std::max(y_b, 0.0f), image.Height() * 1.0f);
	y_u = std::max(std::min(int(y_u), image.Height() - 1), 0);

	for (int i = x_l; i <= x_r; ++i) {
		for (int j = y_b; j <= y_u; ++j) {
			Vec2f Pt0(i, j);
			Vec3f bc = Barycentric(proj<2>(pts[0] / pts[0][3]), proj<2>(pts[1] / pts[1][3]), proj<2>(pts[2] / pts[2][3]), Pt0); // 重心坐标
			Vec3f bc_re = { 0,0,0 }; //根据世界坐标来的重心坐标
			for (int k = 0; k < 3; ++k) {
				bc_re[k] = bc[k] / pts[k][3];
			}
			float abg = bc_re[0] + bc_re[1] + bc_re[2];
			for (int k = 0; k < 3; ++k) {
				bc_re[k] /= abg;
			}
			if (bc.x < 0 || bc.y < 0 || bc.z < 0) continue;// Inside Triangle
				
				float z = bc_re.x * pts[0][2] / pts[0][3] + bc_re.y * pts[1][2] / pts[1][3] + bc_re.z * pts[2][2] / pts[2][3];
				//z = std::max(0.0f, std::min(255.0f, z));
				if (z >= zBuffer[i+j*image.Width()]) {
					TgaColor color;

					bool drop = shader.Fragment(bc_re, color);
					if (!drop) {
						TgaColor z_color = { int(z) };
						zBuffer[i+ j*image.Width()] = z;
						//if((j>=999))
							//std::cout << i << ' ' << j << std::endl;
						image.Set(i, j, color);
					}
				}
		}
	}
}