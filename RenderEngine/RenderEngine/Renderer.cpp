#include "Renderer.h"
#include "GLFW/glfw3.h"

void Renderer::clearColor(uint32_t color)
{
	for (int i = 0; i < SizeWxH; i++) framebuffer[i] = color;
}

void Renderer::clearZbuffer()
{
	for (int i = 0; i < SizeWxH; i++) zbuffer[i] = std::numeric_limits<float>::max();
}


void Renderer::clearAllBuffer()
{

	clearColor(GetBcColor().Getrgb());
	clearZbuffer();
}

void Renderer::drawPx(int sizeX, int sizeY)
{
	for (int y = 0; y < defaultHeight; ++y)
		for (int x = 0; x < defaultWidth; ++x)
			framebuffer[y * defaultWidth + x] = Color( 0xFF000000 | (x * 255 / defaultWidth) << 16 | (y * 255 / defaultHeight) << 8).Getrgb();

	glDrawPixels(defaultWidth, defaultHeight, GL_RGBA, GL_UNSIGNED_BYTE, framebuffer);
}

inline void Renderer::put_pixel(int x, int y, Color color)
{
	if (x < 0 || x >= defaultWidth || y < 0 || y >= defaultHeight) return;
	framebuffer[y * defaultWidth + x] = color.Getrgb();

}

inline int Renderer::edge(Maths::Vec2S v1, Maths::Vec2S v2, Maths::Vec2S P)
{
	return (v2.x - v1.x) * (P.y - v1.y) - (v2.y - v1.y) * (P.x - v1.x);
}


inline int Renderer::clampi(int v, int lo, int hi) {
	return v < lo ? lo : (v > hi ? hi : v);
}


inline void Renderer::compute_face_normals(const Mesh* mesh, Maths::Vec3S* out) {
	 for (int f = 0; f < mesh->faces.size() / 3; f++) {
		 Maths::Vec3S v0 = mesh->verts[mesh->faces[f * 3 + 0]];
		 Maths::Vec3S v1 = mesh->verts[mesh->faces[f * 3 + 1]];
		 Maths::Vec3S v2 = mesh->verts[mesh->faces[f * 3 + 2]];
		 Maths::Vec3S e1 = v1 - v0;
		 Maths::Vec3S e2 = v2 - v0;
		 out[f] = (e1.Cross(e2)).Normalize();
	 }
 }


inline void Renderer::compute_vertex_normals(const Mesh* mesh, const Maths::Vec3S* face_normals, Maths::Vec3S* out) {
	for (int i = 0; i < mesh->verts.size(); i++) out[i] = Maths::Vec3S(0, 0, 0);
	for (int f = 0; f < mesh->faces.size() / 3; f++) {
		for (int k = 0; k < 3; k++) {
			int v = mesh->faces[f * 3 + k];
			out[v] = out[v] + face_normals[f];
		}
	}
	for (int i = 0; i < mesh->verts.size(); i++) out[i] = out[i].Normalize();
}

void Renderer::draw_line(Maths::Vec2S v1, Maths::Vec2S v2, uint32_t color) // draw une ligne x1 et y1 doivent etre plus grand que x0 et y0 et ce sont eux qui modifie lal size de la ligne
{

	int dx = v2.x - v1.x; // delta de x
	int dy = v2.y - v1.y; // delta de y
	int E = 2 * dy - dx; // calcul de E par rapport à delta de x et y 
	int y = v1.y;
	for (int x = v1.x; x < v2.x; x++)
	{
		put_pixel(x, y,Color(0,0,250));
		if (E > 0) 
		{
			y = y + 1;
			E = E + 2 * (dy - dx);
		}
		else 
		{
			E = E + 2 * dy;
		}
	}
	int dx = abs(v2.x - v1.x);
	int dy = -abs(v2.y - v1.y);          // dy is NEGATIVE on purpose, see below
	int sx = (v1.x < v2.x) ? 1 : -1;     // step direction in x
	int sy = (v1.y < v2.y) ? 1 : -1;     // step direction in y
	int err = dx + dy;               // = dx - |dy|

	while (true) {
		put_pixel(v1.x, v2.y, Color(color).Getrgb());
		if (v1.x == v2.x && y0 == y1) break;
		int e2 = 2 * err;
		if (e2 >= dy) { err += dy; v1.x += sx; }   // step in x?
		if (e2 <= dx) { err += dx; v1.y += sy; }   // step in y?
	}
}


inline void Renderer::draw_triangle_phong(
	Maths::Vec3S v0, float iw0, Maths::Vec2S uv0, Maths::Vec3S n0,
	Maths::Vec3S v1, float iw1, Maths::Vec2S uv1, Maths::Vec3S n1,
	Maths::Vec3S v2, float iw2, Maths::Vec2S uv2, Maths::Vec3S n2,
	const Material* mat, Maths::Vec3S L, Maths::Vec3S H) {

	int area = edge(Maths::Vec2S(v0.x, v0.y),Maths::Vec2S(v1.x, v1.y),Maths::Vec2S(v2.x, v2.y));
	if (area == 0) return;

	if (area < 0) {
		int   tx = x1, ty = y1; float tz = z1, tiw = iw1; Maths::Vec2S tuv = uv1; Maths::Vec3S	 tn = n1;
		x1 = x2; y1 = y2; z1 = z2; iw1 = iw2; uv1 = uv2; n1 = n2;
		x2 = tx; y2 = ty; z2 = tz; iw2 = tiw; uv2 = tuv; n2 = tn;
		area = -area;
	}

	int minx = clampi(x0 < x1 ? (x0 < x2 ? x0 : x2) : (x1 < x2 ? x1 : x2), 0, WIDTH - 1);
	int miny = clampi(y0 < y1 ? (y0 < y2 ? y0 : y2) : (y1 < y2 ? y1 : y2), 0, HEIGHT - 1);
	int maxx = clampi(x0 > x1 ? (x0 > x2 ? x0 : x2) : (x1 > x2 ? x1 : x2), 0, WIDTH - 1);
	int maxy = clampi(y0 > y1 ? (y0 > y2 ? y0 : y2) : (y1 > y2 ? y1 : y2), 0, HEIGHT - 1);

	float inv_area = 1.0f / (float)area;

	// Pre-divide every perspective-interpolated attribute by w.
	float u0w = uv0.x * iw0, v0w = uv0.y * iw0;
	float u1w = uv1.x * iw1, v1w = uv1.y * iw1;
	float u2w = uv2.x * iw2, v2w = uv2.y * iw2;
	Maths::Vec3S n0w = n0 * iw0;
	Maths::Vec3S n1w = n1 * iw1;
	Maths::Vec3S n2w = n2 * iw2;

	for (int y = miny; y <= maxy; y++) {
		for (int x = minx; x <= maxx; x++) {
			int e0 = edge(x1, y1, x2, y2, x, y);
			int e1 = edge(x2, y2, x0, y0, x, y);
			int e2 = edge(x0, y0, x1, y1, x, y);
			if ((e0 | e1 | e2) < 0) continue;

			float l0 = e0 * inv_area;
			float l1 = e1 * inv_area;
			float l2 = e2 * inv_area;

			float z = l0 * z0 + l1 * z1 + l2 * z2;

			int idx = y * WIDTH + x;
			if (z >= zbuffer[idx]) continue;

			float iw = l0 * iw0 + l1 * iw1 + l2 * iw2;
			float inv_iw = 1.0f / iw;

			float u = (l0 * u0w + l1 * u1w + l2 * u2w) * inv_iw;
			float v = (l0 * v0w + l1 * v1w + l2 * v2w) * inv_iw;

			// Perspective-correct interpolated normal, then re-normalise:
			// averaging unit vectors gives a non-unit result.
			Maths::Vec3S N = Maths::Vec3S((l0 * n0w.x + l1 * n1w.x + l2 * n2w.x) * inv_iw,
				(l0 * n0w.y + l1 * n1w.y + l2 * n2w.y) * inv_iw,
				(l0 * n0w.z + l1 * n1w.z + l2 * n2w.z) * inv_iw);
			N = N.Normalize();

			float ndotl = N.Dot(L);
			if (ndotl < 0.0f) ndotl = 0.0f;

			float ndoth = N.Dot(H);
			if (ndoth < 0.0f) ndoth = 0.0f;


			uint32_t src = mat->Shade(u, v, ndotl, ndoth);
			uint32_t dst = framebuffer[idx];



			zbuffer[idx] = z;
			framebuffer[idx] = mat->rdPass(src, dst);



		}
	}
}



void Renderer::init()
{


}
