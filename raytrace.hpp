#include <vector>
#include <glm/glm.hpp>

using namespace glm;

struct camera_t {
	vec3 position;
	vec3 direction;
	float aspect;
	float fovy;
};

struct scene_t {
		
};

void raytrace(std::vector<vec3>& framebuffer, int width, int height, const camera_t& camera, const scene_t& scene) {
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			framebuffer[y * width + x] = vec3(1.f, 0.f, 1.f);
		}
	}
}









