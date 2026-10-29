#include "Renderer.h"

#include "MeshRLoader.h"
#include "Texture.h"
#include "clip.h"
#include "Material.h"
#include "Font.h"
#include <stdexcept>


Renderer* Renderer::r_instance = nullptr;
uint32_t  Renderer::framebuffer[Renderer::defaultWidth * Renderer::defaultHeight];
float     Renderer::zbuffer[Renderer::defaultWidth * Renderer::defaultHeight];

Renderer::Renderer()
{
	if (r_instance != nullptr)
		throw std::runtime_error("Renderer deja instancie !");
	r_instance = this;
}

Renderer::~Renderer()
{
	if (r_instance == this)
		r_instance = nullptr;
}

void Renderer::clearColor(uint32_t color)
{
	for (int i = 0; i < GetSizeWxH(); i++) framebuffer[i] = color;
}

void Renderer::clearZbuffer()
{
	for (int i = 0; i < GetSizeWxH(); i++) zbuffer[i] = std::numeric_limits<float>::max();
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
	dx = abs(v2.x - v1.x);
	dy = -abs(v2.y - v1.y);          // dy is NEGATIVE on purpose, see below
	int sx = (v1.x < v2.x) ? 1 : -1;     // step direction in x
	int sy = (v1.y < v2.y) ? 1 : -1;     // step direction in y
	int err = dx + dy;               // = dx - |dy|

	while (true) {
		put_pixel(v1.x, v2.y, Color(color).Getrgb());
		if (v1.x == v2.x && v1.y == v1.y) break;
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
		Maths::Vec3S t = v1;
		int tiw = iw1; 
		Maths::Vec2S tuv = uv1; 
		Maths::Vec3S tn = n1;
		v1 = v2;
		iw1 = iw2; uv1 = uv2; n1 = n2;
		v2 = t;
		iw2 = tiw; uv2 = tuv; n2 = tn;
		area = -area;
	}
	int minx = clampi(v0.x < v1.x ? (v0.x < v2.x ? v0.x : v2.x) : (v1.x < v2.x ? v1.x : v2.x), 0, defaultWidth - 1);
	int miny = clampi(v0.y < v1.y ? (v0.y < v2.y ? v0.y : v2.y) : (v1.y < v2.y ? v1.y : v2.y), 0, defaultHeight - 1);
	int maxx = clampi(v0.x > v1.x ? (v0.x > v2.x ? v0.x : v2.x) : (v1.x > v2.x ? v1.x : v2.x), 0, defaultWidth - 1);
	int maxy = clampi(v0.y > v1.y ? (v0.y > v2.y ? v0.y : v2.y) : (v1.y > v2.y ? v1.y : v2.y), 0, defaultHeight - 1);

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
			int e0 = edge(Maths::Vec2S(v1.x, v1.y), Maths::Vec2S(v2.x, v2.y), Maths::Vec2S(x, y));
			int e1 = edge(Maths::Vec2S(v2.x, v2.y), Maths::Vec2S(v0.x, v0.y), Maths::Vec2S(x, y));
			int e2 = edge(Maths::Vec2S(v0.x, v0.y), Maths::Vec2S(v1.x, v1.y), Maths::Vec2S(x, y));
			if ((e0 | e1 | e2) < 0) continue;

			float l0 = e0 * inv_area;
			float l1 = e1 * inv_area;
			float l2 = e2 * inv_area;

			float z = l0 * v0.z + l1 * v1.z + l2 * v2.z;

			int idx = y * defaultWidth + x;
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



inline void Renderer::projectAndDraw(const ClipVertex* tri, const Material* mat, Maths::Vec3S L, Maths::Vec3S H) {
	int   sx[3], sy[3];
	float ndcZ[3], invW[3];
	for (int k = 0; k < 3; k++) {
		float inv = 1.0f / tri[k].clip.w;
		sx[k] = (tri[k].clip.x * inv + 1.0f) * 0.5f * defaultWidth;
		sy[k] = (tri[k].clip.y * inv + 1.0f) * 0.5f * defaultHeight;
		ndcZ[k] = tri[k].clip.z * inv;
		invW[k] = inv;
	}

	if (edge(Maths::Vec2S(sx[0], sy[0]), Maths::Vec2S(sx[1], sy[1]), Maths::Vec2S(sx[2], sy[2])) <= 0) return;  // cull (ton sens actuel)

	draw_triangle_phong(
		Maths::Vec3S(sx[0], sy[0], ndcZ[0]), invW[0], tri[0].uv, tri[0].normal,
		Maths::Vec3S(sx[1], sy[1], ndcZ[1]), invW[1], tri[1].uv, tri[1].normal,
		Maths::Vec3S(sx[2], sy[2], ndcZ[2]), invW[2], tri[2].uv, tri[2].normal,
		mat, L, H);
}


 void Renderer::drawMeshTextured(const Mesh& mesh, const Maths::Matrix4x4SIMD& mvp, Maths::Matrix4x4SIMD model, const Material& mat, Maths::Vec3S* face_normals, Maths::Vec3S* vertex_normals)
{
	const int vcount = (int)mesh.verts.size();

	const Maths::Vec3S light = (Maths::Vec3S(0.4f, 0.8f, 0.45f)).Normalize();
	const Maths::Vec3S view_dir = Maths::Vec3S(0, 0, 1);
	const Maths::Vec3S half_vec = (light + view_dir).Normalize();

	// Étape 1 : par sommet du mesh -> clip space seulement (PAS de division par w)
	std::vector<Maths::Vec4S> clipV(vcount);
	std::vector<Maths::Vec3S> normalWorld(vcount);
	for (int i = 0; i < vcount; i++) {
		clipV[i] = mvp.operator*(Maths::Vec4S::FromPoint(mesh.verts[i]));
		normalWorld[i] = model * vertex_normals[i];
	}

	// Étape 2 : par triangle -> clipping, puis projection + dessin
	for (int fidx = 0; fidx < (int)mesh.faces.size() / 3; fidx++) {
		Maths::Vec3S abc;
		abc.x = mesh.faces[fidx * 3 + 0];
		abc.y = mesh.faces[fidx * 3 + 1];
		abc.z = mesh.faces[fidx * 3 + 2];

		Maths::Vec3S u;
		u.x = mesh.texFaces[fidx * 3 + 0];
		u.y = mesh.texFaces[fidx * 3 + 1];
		u.z = mesh.texFaces[fidx * 3 + 2];

		ClipVertex v0, v1, v2;
		v0.clip = clipV[abc.x]; v0.normal = normalWorld[abc.x]; v0.uv = (u.x >= 0) ? mesh.textures[u.x] : Maths::Vec2S(0, 0);
		v1.clip = clipV[abc.y]; v1.normal = normalWorld[abc.y]; v1.uv = (u.y >= 0) ? mesh.textures[u.y] : Maths::Vec2S(0, 0);
		v2.clip = clipV[abc.z]; v2.normal = normalWorld[abc.z]; v2.uv = (u.z >= 0) ? mesh.textures[u.z] : Maths::Vec2S(0, 0);

		// Clip contre le plan near -> 0, 1 ou 2 triangles
		ClipVertex out[6];
		int nTris;
		ClipTriangleNear(v0, v1, v2, out, &nTris);

		for (int t = 0; t < nTris; t++)
			projectAndDraw(&out[t * 3], &mat, light, half_vec);
	}
}


void Renderer::init()
{
	if (!glfwInit()) {
		std::cout << "Échec de glfwInit\n";
	}


	
}


void Renderer::update()
{

	// Les load vont etre deplacer dans un loader global avec des vec de texture et font en shared ce qui va permettre de juste selectionner par "id" chaque élément pour le draw un struct sera plus opti pour ça 
	auto texture = std::make_shared<Texture>();
	texture->load("assets/spot_texture.png");

	Font font;
	font.loadFont("assets/SIXTY.TTF");

	for (char c : std::string("A B~"))
		std::cout << "'" << c << "' -> " << font.getGlyphIndex(c) << "\n";
	std::cout << "U+4E2D -> " << font.getGlyphIndex(0x4E2D) << "\n";

	Mesh mesh;
	MeshLoader::loadMesh("assets/spot.obj", mesh);


	Material material;
	material.setAlbedoMap(texture);
	material.setOpacity(0.9f);




	// One model-space normal per triangle, computed once.
	Maths::Vec3S* face_normals = (Maths::Vec3S*)malloc(mesh.faces.size() * sizeof(Maths::Vec3S));
	Maths::Vec3S* vertex_normals = (Maths::Vec3S*)malloc(mesh.verts.size() * sizeof(Maths::Vec3S));
	compute_face_normals(&mesh, face_normals);
	compute_vertex_normals(&mesh, face_normals, vertex_normals);




	const uint64_t freq = glfwGetTimerFrequency();
	const uint64_t start = glfwGetTimerValue();

	Renderer::window = glfwCreateWindow(defaultWidth, defaultHeight, "hello", NULL, NULL);
	glfwSwapInterval(0);

	// Projection and view stay fixed; only the model spins.
	const float aspect = defaultWidth / defaultHeight;

	// 60-degree vertical field of view (radians() converts to the
	// radians the math needs); window shape; near and far clip
	// distances. See README section 5.6.
	const Maths::Matrix4x4SIMD projection = Maths::Matrix4x4SIMD::Perspective(Maths::Matrix4x4SIMD::radians(60.0f), aspect, 0.1f, 100.0f);

	// Camera: positioned 5 units back on +z (eye), aimed at the
	// origin (target), with world-up being +y. See README section 5.10.
	const Maths::Vec3S eye{ 0, 0, 3 };
	const Maths::Vec3S target{ 0, 0, 0 };
	const Maths::Vec3S up{ 0, 1, 0 };
	const Maths::Matrix4x4SIMD view = Maths::Matrix4x4SIMD::LookAt(eye, target, up);


	glfwMakeContextCurrent(window);

	double lastTime = glfwGetTime();
	int frameCount = 0;
	Color color(16, 16, 24);






	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		double seconds = (double)(glfwGetTimerValue() - start) / (double)freq;		//tick / frequence  = temps en seconde					
		float angle = (float)seconds;   // 1 radian par seconde

		Maths::Matrix4x4SIMD model = Maths::Matrix4x4SIMD::RotationY(angle * 0.7f);
		Maths::Matrix4x4SIMD mvp = projection * view * model;

		clearAllBuffer();


		drawMeshTextured(mesh, mvp, model, material, face_normals, vertex_normals);

		glDrawPixels(defaultWidth, defaultHeight, GL_RGBA, GL_UNSIGNED_BYTE, framebuffer);
		glfwSwapBuffers(window);

		double currentTime = glfwGetTime();
		frameCount++;

		if (currentTime - lastTime >= 1) {
			double fps = frameCount / (currentTime - lastTime);

			char title[256];
			snprintf(title, sizeof(title), "FPS Counter [%.1f FPS]", fps);
			glfwSetWindowTitle(window, title);

			frameCount = 0;
			lastTime = currentTime;
		}
	}
	glfwDestroyWindow(window);
	glfwTerminate();
}