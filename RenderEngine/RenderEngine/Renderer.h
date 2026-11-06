#pragma once
#include <vector>
#include <atomic>
#include <barrier>
#include <memory>
#include <thread>
#include "Color.h"
#include "GLFW/glfw3.h"
#include "Font.h"
#include "ParticleSystem.h"
#include "External Lib/MathLibrary/MathLib.h"


struct Mesh;
struct ClipVertex;
class Material;


struct ScreenTri {
	int   x[3], y[3];
	float z[3], iw[3];
	float uw[3], vw[3];
	float nx[3], ny[3], nz[3];
	float invArea;
	int   minX, minY, maxX, maxY;
};


struct Tile { int x0, y0, x1, y1; };


class Renderer {
private:
	static constexpr int defaultWidth_ = 800;
	static constexpr int defaultHeight_ = 600;
	static constexpr int tileSize_ = 32; // 32 ou 64 mais same pour le test a voir avec ajout du binning
	static constexpr int sizeWxH_ = defaultWidth_ * defaultHeight_;

	Color defaultBcColor_;
	static Renderer* instance_;
	GLFWwindow* window_ = nullptr;

	static inline std::vector<std::thread> threads_;
	static inline std::unique_ptr<std::barrier<>> startBarrier_;
	static inline std::unique_ptr<std::barrier<>> endBarrier_;
	static inline std::atomic<int>  nextTile_{ 0 };
	static inline std::atomic<bool> shouldQuit_{ false };
	static inline const Material* currentMaterial_ = nullptr;

	static inline std::vector<Tile>      tiles_;
	static inline std::vector<ScreenTri> tris_;
	static inline int tilesX_ = 0, tilesY_ = 0;

	static int  edge(Maths::Vec2S v1, Maths::Vec2S v2, Maths::Vec2S p);
	static void clearColor(uint32_t color);
	static void clearZbuffer();

	static int  clampi(int v, int lo, int hi);
	static void computeFaceNormals(const Mesh* mesh, Maths::Vec3S* out);
	static void computeVertexNormals(const Mesh* mesh, const Maths::Vec3S* faceNormals, Maths::Vec3S* out);

	static void startThreadPool();
	static void stopThreadPool();
	static void workerLoop();

protected:
	static void buildTiles(); // Done once or window changes size
	static void renderTile(int tileIndex, const Material& mat);
	static void processTiles(const Material& mat);
	static void renderAllTiles(const Material& mat);

	static void drawMeshTextured(const Mesh& mesh, const Maths::Matrix4x4SIMD& mvp, Maths::Matrix4x4SIMD model,
		Maths::Vec3S* faceNormals, Maths::Vec3S* vertexNormals, const Material& mat);
	static void project(const ClipVertex* tri);
	void drawPx(int sizeX, int sizeY);
	static void putPixel(int x, int y, Color color);
	static void drawLine(Maths::Vec2S v1, Maths::Vec2S v2, uint32_t color);
	void clearAllBuffer();

	static void rasterizeTri(const ScreenTri& t, const Tile& tile, const Material& mat);
	static void drawParticleSprite(const ParticleSprite& sprite, const Material& mat);
	void drawBitmap(const Bitmap& bmp, int x, int y, Color color);

public:
	static uint32_t framebuffer_[defaultWidth_ * defaultHeight_];
	static float    zbuffer_[defaultWidth_ * defaultHeight_];

	void init();
	void update();

	Renderer();
	~Renderer();

	Renderer(const Renderer&) = default;
	Renderer& operator=(const Renderer&) = default;

	static Renderer* instance() { return instance_; }

	Color getBcColor() const { return defaultBcColor_; }
	void setBcColor(Color color) { defaultBcColor_ = color; }
	static constexpr int getSizeWxH() { return sizeWxH_; }
};