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

layout(location = 0) in float frag_h;
layout(location = 1) in vec3 frag_normal;
layout(location = 2) in float frag_dist;

layout(location = 0) out vec4 out_color;

layout(push_constant) uniform PushConstants {
  mat4 mvp;
  vec4 cam_pos;
  ivec4 grid_info;
} pc;

void main() {
  float h = frag_h;

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

  // Directional sunlight + ambienht sky light
  vec3 sun_dir = normalize(vec3(0.6, 0.4, 0.7));
  float diff = max(dot(normalize(frag_normal), sun_dir), 0.0);
  float light = (h<0.35) ? 0.95 : (0.25 + 0.65*diff);
  vec3 lit_color = color * light;

  // Smooth atmospheric fog matching the clear colour (0.15, 0.35, 0.65)
  float max_dist = float(pc.grid_info.z);
  float fog = clamp((frag_dist - max_dist*0.65) / (max_dist*0.3), 0.0, 1.0);
  vec3 sky_color = vec3(0.15, 0.35, 0.65);

  out_color = vec4(mix(lit_color, sky_color, fog), 1.0);
}
