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

void Renderer::BuildTiles() {
	tiles.clear();
	tilesY = (defaultHeight + tileSize - 1) / tileSize;
	tilesX = (defaultWidth + tileSize - 1) / tileSize;
	for (int ty = 0; ty < tilesY; ty++)
		for (int tx = 0; tx < tilesX; tx++)
			tiles.push_back({
				tx * tileSize, ty * tileSize,
				std::min((tx + 1) * tileSize, defaultWidth),
				std::min((ty + 1) * tileSize, defaultHeight) });

}

// clear + rasterize
void Renderer::RenderTile(int tileIndex)
{
	const Tile& tile = tiles[tileIndex];

	for (int y = tile.y0; y < tile.y1; y++) {
		std::fill(framebuffer + y * defaultWidth + tile.x0, framebuffer + y * defaultWidth + tile.x1, 0xFF1A1F29);
		std::fill(zbuffer + y * defaultWidth + tile.x0, zbuffer + y * defaultWidth + tile.x1, std::numeric_limits<float>::max());
	}

	// si pas ds boxTile dégage
	for (const ScreenTri& tri : tris)
		RasterizeTri(tri, tile);
}

void Renderer::ProcessTiles()
{
	int t;
	while ((t = nextTile.fetch_add(1)) < (int)tiles.size())
		RenderTile(t);
}

void Renderer::RenderAllTiles()
{
	nextTile = 0;
	startBarrier->arrive_and_wait();
	ProcessTiles();
	endBarrier->arrive_and_wait();
}

void Renderer::WorkerLoop()
{
	for (;;) {
		startBarrier->arrive_and_wait();
		if (quit.load()) return;
		ProcessTiles();
		endBarrier->arrive_and_wait();
	}
}


void Renderer::StartThreadPool()
{
	int nbT = (int)std::thread::hardware_concurrency();
	if (nbT < 1) nbT = 1;
	int workers = nbT - 1;

	startBarrier = std::make_unique<std::barrier<>>(workers + 1);
	endBarrier = std::make_unique<std::barrier<>>(workers + 1);
	for (int i = 0; i < workers; i++)
		threads.emplace_back(WorkerLoop);
}

void Renderer::StopThreadPool()
{
	quit = true;
	startBarrier->arrive_and_wait();      // wakey wakey
	for (auto& t : threads) t.join();
	threads.clear();
}

inline void Renderer::RasterizeTri(const ScreenTri& t, const Tile& tile, const Material* mat) {

	int minx = std::max(t.minx, tile.x0);
	int maxx = std::min(t.maxx, tile.x1 - 1);
	int miny = std::max(t.miny, tile.y0);
	int maxy = std::min(t.maxy, tile.y1 - 1);
	if (minx > maxx || miny > maxy) return;

	const Maths::Vec3S light = (Maths::Vec3S(0.4f, 0.8f, 0.45f)).Normalize();
	const Maths::Vec3S view_dir = Maths::Vec3S(0, 0, 1);
	const Maths::Vec3S half_vec = (light + view_dir).Normalize();

	for (int y = miny; y <= maxy; y++) {
		for (int x = minx; x <= maxx; x++) {
			int e0 = edge(Maths::Vec2S{t.x[1], t.y[1]}, Maths::Vec2S{t.x[2], t.y[2]}, Maths::Vec2S{x, y});
			int e1 = edge(Maths::Vec2S{t.x[2], t.y[2]}, Maths::Vec2S{t.x[0], t.y[0]}, Maths::Vec2S{x, y});
			int e2 = edge(Maths::Vec2S{t.x[0], t.y[0]}, Maths::Vec2S{t.x[1], t.y[1]}, Maths::Vec2S{x, y});
			if ((e0 | e1 | e2) < 0) continue;

			float l0 = e0 * t.invArea;
			float l1 = e1 * t.invArea;
			float l2 = e2 * t.invArea;

			float z = l0 * t.z[0] + l1 * t.z[1] + l2 * t.z[2];

			int idx = y * defaultWidth + x;
			if (z >= zbuffer[idx]) continue;


			float iw = l0 * t.iw[0] + l1 * t.iw[1] + l2 * t.iw[2];
			float inv_iw = 1.0f / iw;

			float u = (l0 * t.uw[0] + l1 * t.uw[1] + l2 * t.uw[2]) * inv_iw;
			float v = (l0 * t.vw[0] + l1 * t.vw[1] + l2 * t.vw[2]) * inv_iw;

			// Perspective-correct interpolated normal, then re-normalise:
			// averaging unit vectors gives a non-unit result.
			Maths::Vec3S N = Maths::Vec3S((l0 * t.nx[0] + l1 * t.nx[1] + l2 * t.nx[2]) * inv_iw,
				(l0 * t.ny[0] + l1 * t.ny[1] + l2 * t.ny[2]) * inv_iw,
				(l0 * t.nz[0] + l1 * t.nz[1] + l2 * t.nz[2]) * inv_iw);
			N = N.Normalize();

			float ndotl = N.Dot(light);
			if (ndotl < 0.0f) ndotl = 0.0f;

			float ndoth = N.Dot(half_vec);
			if (ndoth < 0.0f) ndoth = 0.0f;

			uint32_t src = mat->Shade(u, v, ndotl, ndoth);
			uint32_t dst = framebuffer[idx];

			zbuffer[idx] = z;
			framebuffer[idx] = mat->rdPass(src, dst);
		}
	}
}

inline void Renderer::project(const ClipVertex* tri) {
	int   sx[3], sy[3];
	float ndcZ[3], invW[3];
	for (int k = 0; k < 3; k++) {
		float inv = 1.0f / tri[k].clip.w;
		sx[k] = (tri[k].clip.x * inv + 1.0f) * 0.5f * defaultWidth;
		sy[k] = (tri[k].clip.y * inv + 1.0f) * 0.5f * defaultHeight;
		ndcZ[k] = tri[k].clip.z * inv;
		invW[k] = inv;
	}

	int area = edge(Maths::Vec2S(sx[0], sy[0]), Maths::Vec2S(sx[1], sy[1]), Maths::Vec2S(sx[2], sy[2])); // cull (ton sens actuel)
	if (area <= 0) return;   

	int rminx = std::min({ sx[0], sx[1], sx[2] });
	int rmaxx = std::max({ sx[0], sx[1], sx[2] });
	int rminy = std::min({ sy[0], sy[1], sy[2] });
	int rmaxy = std::max({ sy[0], sy[1], sy[2] });

	// bye bye not on screen
	if (rmaxx < 0 || rminx >= defaultWidth || rmaxy < 0 || rminy >= defaultHeight) return;

	ScreenTri st;
	for (int k = 0; k < 3; k++) {
		st.x[k] = sx[k];  st.y[k] = sy[k];
		st.z[k] = ndcZ[k]; st.iw[k] = invW[k];
		st.uw[k] = tri[k].uv.x * invW[k];
		st.vw[k] = tri[k].uv.y * invW[k];
		st.nx[k] = tri[k].normal.x * invW[k];
		st.ny[k] = tri[k].normal.y * invW[k];
		st.nz[k] = tri[k].normal.z * invW[k];
	}
	st.invArea = 1.0f / (float)area;
	st.minx = clampi(rminx, 0, defaultWidth - 1);
	st.maxx = clampi(rmaxx, 0, defaultWidth - 1);
	st.miny = clampi(rminy, 0, defaultHeight - 1);
	st.maxy = clampi(rmaxy, 0, defaultHeight - 1);

	tris.push_back(st);  //a voir encore avec le binning 
}


 void Renderer::drawMeshTextured(const Mesh& mesh, const Maths::Matrix4x4SIMD& mvp, Maths::Matrix4x4SIMD model, Maths::Vec3S* face_normals, Maths::Vec3S* vertex_normals)
{
	const int vcount = (int)mesh.verts.size();

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
			project(&out[t * 3]);
	}
	RenderAllTiles();
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

	BuildTiles();
	StartThreadPool();

	double lastTime = glfwGetTime();
	int frameCount = 0;
	Color color(16, 16, 24);

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		double seconds = (double)(glfwGetTimerValue() - start) / (double)freq;		//tick / frequence  = temps en seconde					
		float angle = (float)seconds;   // 1 radian par seconde

		Maths::Matrix4x4SIMD model = Maths::Matrix4x4SIMD::RotationY(angle * 0.7f);
		Maths::Matrix4x4SIMD mvp = projection * view * model;

		drawMeshTextured(mesh, mvp, model, face_normals, vertex_normals);

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
	StopThreadPool();
	glfwDestroyWindow(window);
	glfwTerminate();
}