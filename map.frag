/*==============================================================================*
 * MAP_FRAG
 *------------------------------------------------------------------------------*
 * Author:  Ed Higgins <ed.j.higgins@gmail.com>
 *------------------------------------------------------------------------------*
 * Version: 0.1.1, 2026-11-03
 *------------------------------------------------------------------------------*
 * This code is distributed under the GLWTPL
 *==============================================================================*/

#version 460

layout(location = 0) in vec2 frag_uv;
layout(location = 0) out vec4 out_color;

// Shared Fortran-GPU heightmap buffer (SSBO)
layout(std430, set=0, binding=0) readonly buffer HeightmapBuffer {
  float heights[];
};

// Fast 8-byte parameter block sent directly to the command buffer
layout(push_constant) uniform PushConstants {
  uint grid_w;
  uint grid_h;
} pc;

void main() {
  uvec2 coord = min(uvec2(frag_uv * vec2(pc.grid_w, pc.grid_h)),
                    uvec2(pc.grid_w-1, pc.grid_h-1));

  float h = heights[coord.y*pc.grid_w + coord.x];
  
  // Basic hyposometric tinting based on height h in [0.0, 1.0]
  vec3 color;
  if (h < 0.5) {
    // Deep to shallow water
    color = mix(vec3(0.05, 0.15, 0.40), vec3(0.15, 0.40, 0.70), h/0.5);
  } else if (h < 0.55) {
    // Sand/beach
    color = mix(vec3(0.76, 0.70, 0.50), vec3(0.25, 0.55, 0.20), (h-0.5)/0.05);
  } else if (h < 0.85) {
    // iLowland forest to upland
    color = mix(vec3(0.25, 0.55, 0.20), vec3(0.45, 0.40, 0.30), (h-0.55)/0.30);
  } else {
    // Bare rock to snow peaks
    color = mix(vec3(0.45, 0.40, 0.30), vec3(0.95, 0.95, 0.98), (h-0.85)/0.15);
  }

  out_color = vec4(color, 1.0);
}
