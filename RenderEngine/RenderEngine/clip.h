#pragma once
#include "External Lib/MathLibrary/MathLib.h"

// Un sommet avec tout ce qu'il faut interpoler pendant le clipping
struct ClipVertex {
	Maths::Vec4S clip;    // position clip space (après MVP, avant division par w)
	Maths::Vec2S uv;
	Maths::Vec3S normal;  // normale monde
};

// r = a + t * (b - a), pour TOUS les attributs
static inline ClipVertex ClipLerp(const ClipVertex& a, const ClipVertex& b, float t) {
	ClipVertex r;
	r.clip.x = a.clip.x + t * (b.clip.x - a.clip.x);
	r.clip.y = a.clip.y + t * (b.clip.y - a.clip.y);
	r.clip.z = a.clip.z + t * (b.clip.z - a.clip.z);
	r.clip.w = a.clip.w + t * (b.clip.w - a.clip.w);
	r.uv.x = a.uv.x + t * (b.uv.x - a.uv.x);
	r.uv.y = a.uv.y + t * (b.uv.y - a.uv.y);
	r.normal.x = a.normal.x + t * (b.normal.x - a.normal.x);
	r.normal.y = a.normal.y + t * (b.normal.y - a.normal.y);
	r.normal.z = a.normal.z + t * (b.normal.z - a.normal.z);
	return r;
}

// Distance signée au plan near : >= 0 = devant (on garde)
static inline float ClipNearDist(const ClipVertex& v) {
	return v.clip.z + v.clip.w;
}

// Sutherland-Hodgman contre le plan near.
// Écrit 0, 3 ou 6 sommets dans out (= 0, 1 ou 2 triangles). out doit faire >= 6.
static inline void ClipTriangleNear(const ClipVertex& v0, const ClipVertex& v1, const ClipVertex& v2,
	ClipVertex* out, int* outNTris) {
	const ClipVertex in[3] = { v0, v1, v2 };
	ClipVertex poly[4];
	int n = 0;

	for (int e = 0; e < 3; e++) {
		const ClipVertex& a = in[e];
		const ClipVertex& b = in[(e + 1) % 3];
		float da = ClipNearDist(a);
		float db = ClipNearDist(b);
		bool aIn = da >= 0.0f;
		bool bIn = db >= 0.0f;

		if (aIn && bIn) {                 // les deux dedans : on garde b
			poly[n++] = b;
		}
		else if (aIn && !bIn) {         // on sort : point d'intersection
			poly[n++] = ClipLerp(a, b, da / (da - db));
		}
		else if (!aIn && bIn) {         // on rentre : intersection puis b
			poly[n++] = ClipLerp(a, b, da / (da - db));
			poly[n++] = b;
		}                                 // les deux dehors : rien
	}

	if (n == 3) {
		out[0] = poly[0]; out[1] = poly[1]; out[2] = poly[2];
		*outNTris = 1;
	}
	else if (n == 4) {                  // quad -> 2 triangles (0,1,2) et (0,2,3)
		out[0] = poly[0]; out[1] = poly[1]; out[2] = poly[2];
		out[3] = poly[0]; out[4] = poly[2]; out[5] = poly[3];
		*outNTris = 2;
	}
	else {
		*outNTris = 0;
	}
}