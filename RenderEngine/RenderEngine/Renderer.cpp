#include "Renderer.h"
#include "MeshRLoader.h"
#include "Texture.h"
#include "clip.h"
#include "Material.h"
#include <stdexcept>
#include <functional>

Renderer* Renderer::instance_ = nullptr;
uint32_t  Renderer::framebuffer_[Renderer::defaultWidth_ * Renderer::defaultHeight_];
float     Renderer::zbuffer_[Renderer::defaultWidth_ * Renderer::defaultHeight_];

Renderer::Renderer()
{
	if (instance_ != nullptr)
		throw std::runtime_error("Renderer deja instancie !");
	instance_ = this;
}

Renderer::~Renderer()
{
	if (instance_ == this)
		instance_ = nullptr;
}

void Renderer::clearColor(uint32_t color)
{
	for (int i = 0; i < getSizeWxH(); i++) framebuffer_[i] = color;
}

void Renderer::clearZbuffer()
{
	for (int i = 0; i < getSizeWxH(); i++) zbuffer_[i] = std::numeric_limits<float>::max();
}

void Renderer::clearAllBuffer()
{
	clearColor(getBcColor().getRgb());
	clearZbuffer();
}

void Renderer::drawPx(int sizeX, int sizeY)
{
	for (int y = 0; y < defaultHeight_; ++y)
		for (int x = 0; x < defaultWidth_; ++x)
			framebuffer_[y * defaultWidth_ + x] = Color(0xFF000000 | (x * 255 / defaultWidth_) << 16 | (y * 255 / defaultHeight_) << 8).getRgb();

	glDrawPixels(defaultWidth_, defaultHeight_, GL_RGBA, GL_UNSIGNED_BYTE, framebuffer_);
}

void Renderer::putPixel(int x, int y, Color color)
{
	if (x < 0 || x >= defaultWidth_ || y < 0 || y >= defaultHeight_) return;
	framebuffer_[y * defaultWidth_ + x] = color.getRgb();
}

int Renderer::edge(Maths::Vec2S v1, Maths::Vec2S v2, Maths::Vec2S p)
{
	return static_cast<int>((v2.x - v1.x) * (p.y - v1.y) - (v2.y - v1.y) * (p.x - v1.x));
}

int Renderer::clampi(int v, int lo, int hi)
{
	return v < lo ? lo : (v > hi ? hi : v);
}

void Renderer::computeFaceNormals(const Mesh* mesh, Maths::Vec3S* out)
{
	for (int f = 0; f < (int)mesh->faces.size() / 3; f++) {
		Maths::Vec3S v0 = mesh->verts[mesh->faces[f * 3 + 0]];
		Maths::Vec3S v1 = mesh->verts[mesh->faces[f * 3 + 1]];
		Maths::Vec3S v2 = mesh->verts[mesh->faces[f * 3 + 2]];
		Maths::Vec3S e1 = v1 - v0;
		Maths::Vec3S e2 = v2 - v0;
		out[f] = (e1.Cross(e2)).Normalize();
	}
}

void Renderer::computeVertexNormals(const Mesh* mesh, const Maths::Vec3S* faceNormals, Maths::Vec3S* out)
{
	for (int i = 0; i < (int)mesh->verts.size(); i++) out[i] = Maths::Vec3S(0, 0, 0);
	for (int f = 0; f < (int)mesh->faces.size() / 3; f++) {
		for (int k = 0; k < 3; k++) {
			int v = mesh->faces[f * 3 + k];
			out[v] = out[v] + faceNormals[f];
		}
	}
	for (int i = 0; i < (int)mesh->verts.size(); i++) out[i] = out[i].Normalize();
}

// Bresenham, tous octants
void Renderer::drawLine(Maths::Vec2S v1, Maths::Vec2S v2, uint32_t color)
{
	int x0 = (int)v1.x, y0 = (int)v1.y;
	int x1 = (int)v2.x, y1 = (int)v2.y;
	int dx = abs(x1 - x0);
	int dy = -abs(y1 - y0);
	int sx = (x0 < x1) ? 1 : -1;
	int sy = (y0 < y1) ? 1 : -1;
	int err = dx + dy;

	for (;;) {
		putPixel(x0, y0, Color(color));
		if (x0 == x1 && y0 == y1) break;
		int e2 = 2 * err;
		if (e2 >= dy) { err += dy; x0 += sx; }
		if (e2 <= dx) { err += dx; y0 += sy; }
	}
}

void Renderer::buildTiles()
{
	tiles_.clear();
	tilesY_ = (defaultHeight_ + tileSize_ - 1) / tileSize_;
	tilesX_ = (defaultWidth_ + tileSize_ - 1) / tileSize_;
	for (int ty = 0; ty < tilesY_; ty++)
		for (int tx = 0; tx < tilesX_; tx++)
			tiles_.push_back({
				tx * tileSize_, ty * tileSize_,
				std::min((tx + 1) * tileSize_, defaultWidth_),
				std::min((ty + 1) * tileSize_, defaultHeight_) });
}

// clear + rasterize
void Renderer::renderTile(int tileIndex, const Material& mat)
{
	const Tile& tile = tiles_[tileIndex];
	for (const ScreenTri& tri : tris_)
		rasterizeTri(tri, tile, mat);
}

void Renderer::processTiles(const Material& mat)
{
	int tileIndex;
	while ((tileIndex = nextTile_.fetch_add(1)) < static_cast<int>(tiles_.size()))
		renderTile(tileIndex, mat);
}

void Renderer::renderAllTiles(const Material& mat)
{
	currentMaterial_ = &mat;
	nextTile_ = 0;
	startBarrier_->arrive_and_wait();
	processTiles(mat);
	endBarrier_->arrive_and_wait();
}

void Renderer::workerLoop()
{
	for (;;) {
		startBarrier_->arrive_and_wait();
		if (shouldQuit_.load()) return;
		processTiles(*currentMaterial_);
		endBarrier_->arrive_and_wait();
	}
}

void Renderer::startThreadPool()
{
	int threadCount = static_cast<int>(std::thread::hardware_concurrency());
	if (threadCount < 1) threadCount = 1;
	int workers = threadCount - 1;

	startBarrier_ = std::make_unique<std::barrier<>>(workers + 1);
	endBarrier_ = std::make_unique<std::barrier<>>(workers + 1);
	for (int i = 0; i < workers; i++)
		threads_.emplace_back(workerLoop);   // cref : pas de copie du Material
}

void Renderer::stopThreadPool()
{
	shouldQuit_ = true;
	startBarrier_->arrive_and_wait();      // wakey wakey
	for (auto& t : threads_) t.join();
	threads_.clear();
}

void Renderer::rasterizeTri(const ScreenTri& t, const Tile& tile, const Material& mat)
{
	int minX = std::max(t.minX, tile.x0);
	int maxX = std::min(t.maxX, tile.x1 - 1);
	int minY = std::max(t.minY, tile.y0);
	int maxY = std::min(t.maxY, tile.y1 - 1);
	if (minX > maxX || minY > maxY) return;

	const Maths::Vec3S light = (Maths::Vec3S(0.4f, 0.8f, 0.45f)).Normalize();
	const Maths::Vec3S viewDir = Maths::Vec3S(0, 0, 1);
	const Maths::Vec3S halfVec = (light + viewDir).Normalize();

	const bool writeZ = mat.getOpacity() >= 1.0f;

	for (int y = minY; y <= maxY; y++) {
		for (int x = minX; x <= maxX; x++) {
			int e0 = edge(Maths::Vec2S{ static_cast<float>(t.x[1]), static_cast<float>(t.y[1]) }, Maths::Vec2S{ static_cast<float>(t.x[2]), static_cast<float>(t.y[2]) }, Maths::Vec2S{ static_cast<float>(x), static_cast<float>(y) });
			int e1 = edge(Maths::Vec2S{ static_cast<float>(t.x[2]), static_cast<float>(t.y[2]) }, Maths::Vec2S{ static_cast<float>(t.x[0]), static_cast<float>(t.y[0]) }, Maths::Vec2S{ static_cast<float>(x), static_cast<float>(y) });
			int e2 = edge(Maths::Vec2S{ static_cast<float>(t.x[0]), static_cast<float>(t.y[0]) }, Maths::Vec2S{ static_cast<float>(t.x[1]), static_cast<float>(t.y[1]) }, Maths::Vec2S{ static_cast<float>(x), static_cast<float>(y) });
			if ((e0 | e1 | e2) < 0) continue;

			float l0 = e0 * t.invArea;
			float l1 = e1 * t.invArea;
			float l2 = e2 * t.invArea;

			float z = l0 * t.z[0] + l1 * t.z[1] + l2 * t.z[2];

			int idx = y * defaultWidth_ + x;
			if (z >= zbuffer_[idx]) continue;

			float iw = l0 * t.iw[0] + l1 * t.iw[1] + l2 * t.iw[2];
			float invIw = 1.0f / iw;

			float u = (l0 * t.uw[0] + l1 * t.uw[1] + l2 * t.uw[2]) * invIw;
			float v = (l0 * t.vw[0] + l1 * t.vw[1] + l2 * t.vw[2]) * invIw;

			// Perspective-correct interpolated normal, then re-normalise:
			// averaging unit vectors gives a non-unit result.
			Maths::Vec3S n = Maths::Vec3S((l0 * t.nx[0] + l1 * t.nx[1] + l2 * t.nx[2]) * invIw,
				(l0 * t.ny[0] + l1 * t.ny[1] + l2 * t.ny[2]) * invIw,
				(l0 * t.nz[0] + l1 * t.nz[1] + l2 * t.nz[2]) * invIw);
			n = n.Normalize();

			float nDotL = n.Dot(light);
			if (nDotL < 0.0f) nDotL = 0.0f;

			float nDotH = n.Dot(halfVec);
			if (nDotH < 0.0f) nDotH = 0.0f;

			uint32_t src = mat.shade(u, v, nDotL, nDotH);
			if ((src >> 24) == 0) continue;    // texel totalement transparent

			framebuffer_[idx] = mat.rdPass(src, framebuffer_[idx]);
			if (writeZ) zbuffer_[idx] = z;
		}
	}
}

void Renderer::project(const ClipVertex* tri)
{
	int   sx[3], sy[3];
	float ndcZ[3], invW[3];
	for (int k = 0; k < 3; k++) {
		float inv = 1.0f / tri[k].clip.w;
		sx[k] = (int)((tri[k].clip.x * inv + 1.0f) * 0.5f * defaultWidth_);
		sy[k] = (int)((tri[k].clip.y * inv + 1.0f) * 0.5f * defaultHeight_);
		ndcZ[k] = tri[k].clip.z * inv;
		invW[k] = inv;
	}

	int area = edge(Maths::Vec2S(sx[0], sy[0]), Maths::Vec2S(sx[1], sy[1]), Maths::Vec2S(sx[2], sy[2])); // cull (ton sens actuel)
	if (area <= 0) return;

	int rMinX = std::min({ sx[0], sx[1], sx[2] });
	int rMaxX = std::max({ sx[0], sx[1], sx[2] });
	int rMinY = std::min({ sy[0], sy[1], sy[2] });
	int rMaxY = std::max({ sy[0], sy[1], sy[2] });

	// bye bye not on screen
	if (rMaxX < 0 || rMinX >= defaultWidth_ || rMaxY < 0 || rMinY >= defaultHeight_) return;

	ScreenTri st;
	for (int k = 0; k < 3; k++) {
		st.x[k] = sx[k];   st.y[k] = sy[k];
		st.z[k] = ndcZ[k]; st.iw[k] = invW[k];
		st.uw[k] = tri[k].uv.x * invW[k];
		st.vw[k] = tri[k].uv.y * invW[k];
		st.nx[k] = tri[k].normal.x * invW[k];
		st.ny[k] = tri[k].normal.y * invW[k];
		st.nz[k] = tri[k].normal.z * invW[k];
	}
	st.invArea = 1.0f / static_cast<float>(area);
	st.minX = clampi(rMinX, 0, defaultWidth_ - 1);
	st.maxX = clampi(rMaxX, 0, defaultWidth_ - 1);
	st.minY = clampi(rMinY, 0, defaultHeight_ - 1);
	st.maxY = clampi(rMaxY, 0, defaultHeight_ - 1);

	tris_.push_back(st);  // a voir encore avec le binning
}

void Renderer::drawMeshTextured(const Mesh& mesh, const Maths::Matrix4x4SIMD& mvp, Maths::Matrix4x4SIMD model,
	Maths::Vec3S* faceNormals, Maths::Vec3S* vertexNormals, const Material& mat)
{
	const int vertexCount = (int)mesh.verts.size();

	// Étape 1 : par sommet du mesh -> clip space seulement (PAS de division par w)
	std::vector<Maths::Vec4S> clipV(vertexCount);
	std::vector<Maths::Vec3S> normalWorld(vertexCount);
	for (int i = 0; i < vertexCount; i++) {

		clipV[i] = mvp * Maths::Vec4S::FromPoint(mesh.verts[i]);
		const Maths::Vec4S n = model * Maths::Vec4S(vertexNormals[i].x, vertexNormals[i].y, vertexNormals[i].z, 0.0f);
		normalWorld[i] = Maths::Vec3S(n.x, n.y, n.z).Normalize();
	}

	tris_.clear();

	// Étape 2 : par triangle -> clipping, puis projection + dessin
	const int faceCount = static_cast<int>(mesh.faces.size()) / 3;
	for (int f = 0; f < faceCount; f++) {
		const int a = mesh.faces[f * 3 + 0];
		const int b = mesh.faces[f * 3 + 1];
		const int c = mesh.faces[f * 3 + 2];
		const int ta = mesh.texFaces[f * 3 + 0];
		const int tb = mesh.texFaces[f * 3 + 1];
		const int tc = mesh.texFaces[f * 3 + 2];

		ClipVertex v0, v1, v2;
		v0.clip = clipV[a]; v0.normal = normalWorld[a]; v0.uv = (ta >= 0) ? mesh.textures[ta] : Maths::Vec2S(0, 0);
		v1.clip = clipV[b]; v1.normal = normalWorld[b]; v1.uv = (tb >= 0) ? mesh.textures[tb] : Maths::Vec2S(0, 0);
		v2.clip = clipV[c]; v2.normal = normalWorld[c]; v2.uv = (tc >= 0) ? mesh.textures[tc] : Maths::Vec2S(0, 0);

		// Clip contre le plan near -> 0, 1 ou 2 triangles
		ClipVertex out[6];
		int triCount;
		clipTriangleNear(v0, v1, v2, out, &triCount);

		for (int t = 0; t < triCount; t++)
			project(&out[t * 3]);
	}
	renderAllTiles(mat);
}

 void Renderer::drawBitmap(const Bitmap& bmp, int x, int y, Color color)
 {
	 for (int row = 0; row < bmp.height; ++row)
		 for (int col = 0; col < bmp.width; ++col)
			 if (bmp.pixels[row * bmp.width + col])
				 putPixel(x + col, defaultHeight_ - 1 - (y + row), color);
 }

void Renderer::init()
{
	if (!glfwInit()) {
		std::cout << "Échec de glfwInit\n";
	}
}

void Renderer::update()
{

	auto texture = std::make_shared<Texture>();
	texture->load("assets/spot_texture.png");

	Font font;
	font.loadFont("assets/SIXTY.TTF");
	Bitmap label = font.renderText("Gaming Campus", 40.0f);

	Mesh mesh;
	MeshLoader::loadMesh("assets/spot.obj", mesh);

	
	std::vector<Maths::Vec3S> faceNormals(mesh.faces.size() / 3);
	std::vector<Maths::Vec3S> vertexNormals(mesh.verts.size());

	computeFaceNormals(&mesh, faceNormals.data());
	computeVertexNormals(&mesh, faceNormals.data(), vertexNormals.data());

	
	Material opaqueMat;
	opaqueMat.setAlbedoMap(texture);
	opaqueMat.setOpacity(1.0f);

	Material glassMat;
	glassMat.setAlbedoMap(texture);
	glassMat.setOpacity(0.3f);


	window_ = glfwCreateWindow(defaultWidth_, defaultHeight_, "hello", NULL, NULL);

	const uint64_t freq = glfwGetTimerFrequency();
	const uint64_t start = glfwGetTimerValue();

	const float aspect = static_cast<float>(defaultWidth_) / static_cast<float>(defaultHeight_);

	const Maths::Matrix4x4SIMD projection =
	Maths::Matrix4x4SIMD::Perspective(Maths::Matrix4x4SIMD::radians(60.0f), aspect, 0.1f, 100.0f);

	const Maths::Vec3S eye{ 0, 0, 3 };
	const Maths::Vec3S target{ 0, 0, 0 };
	const Maths::Vec3S up{ 0, 1, 0 };
	const Maths::Matrix4x4SIMD view = Maths::Matrix4x4SIMD::LookAt(eye, target, up);
	const Maths::Matrix4x4SIMD viewProj = projection * view;
	setBcColor(Color(26, 31, 41));
	glfwMakeContextCurrent(window_);
	glfwSwapInterval(0);
	buildTiles();
	startThreadPool();

	double lastTime = glfwGetTime();
	int frameCount = 0;

	// boucle glfw
	while (!glfwWindowShouldClose(window_)) {
		glfwPollEvents();

		const double seconds = static_cast<double>(glfwGetTimerValue() - start) / static_cast<double>(freq);
		const float angle = static_cast<float>(seconds);

		// Un model par vache : Translation * Rotation = tourne sur place à sa position
		const Maths::Matrix4x4SIMD rot = Maths::Matrix4x4SIMD::RotationY(angle * 0.7f);

		const Maths::Matrix4x4SIMD modelOpaque =
			Maths::Matrix4x4SIMD::Translation(Maths::Vec3S(0.0f, 0.0f, -0.2f)) * rot;
		const Maths::Matrix4x4SIMD modelGlass =
			Maths::Matrix4x4SIMD::Translation(Maths::Vec3S(0.0f, 0.0f, 1.0f)) * rot;

		clearColor(getBcColor().getRgb());
		clearZbuffer();


		drawMeshTextured(mesh, viewProj * modelOpaque, modelOpaque,faceNormals.data(), vertexNormals.data(), opaqueMat);

		drawMeshTextured(mesh, viewProj * modelGlass, modelGlass,faceNormals.data(), vertexNormals.data(), glassMat);

		drawBitmap(label, 20, 10, Color(200, 255, 255));

		glDrawPixels(defaultWidth_, defaultHeight_, GL_RGBA, GL_UNSIGNED_BYTE, framebuffer_);
		glfwSwapBuffers(window_);

		const double currentTime = glfwGetTime();
		frameCount++;
		if (currentTime - lastTime >= 1) {
			char title[256];
			snprintf(title, sizeof(title), "FPS Counter [%.1f FPS]", frameCount / (currentTime - lastTime));
			glfwSetWindowTitle(window_, title);
			frameCount = 0;
			lastTime = currentTime;
		}
	}
	stopThreadPool();
	glfwDestroyWindow(window_);
	glfwTerminate();
}