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

layout(location = 0) in float frag_terrain_h;
layout(location = 1) in float frag_water;
layout(location = 2) in float frag_sediment;
layout(location = 3) in vec3 frag_normal;
layout(location = 4) in float frag_dist;

layout(location = 0) out vec4 out_color;

layout(push_constant) uniform PushConstants {
  mat4 mvp;
  vec4 cam_pos;
  ivec4 grid_info;
} pc;

void main() {
  vec3 n = normalize(frag_normal);
  float slope = 1.0 - n.z;

  // Basic hyposometric tinting based on height h in [0.0, 1.0]
  vec3 color;
  if (frag_water > 0.001) {
    // Water shaded by _actual_ depth
    float depth_factor = clamp(frag_water / 0.25, 0.0, 1.0);
    color = mix(vec3(0.18, 0.48, 0.72), vec3(0.04, 0.14, 0.38), depth_factor);
  } else {
    float h = frag_terrain_h;
    if (h < 0.41) { // Coastal sand
      color = mix(vec3(0.76, 0.70, 0.50), vec3(0.30, 0.55, 0.22), (h-0.38) / 0.03);

    } else if (h < 0.75) { // Grass/forest on flat slopes, rock on steep cliffs
      vec3 veg = mix(vec3(0.25, 0.52, 0.20), vec3(0.42, 0.48, 0.28), (h-0.41)/0.34);
      vec3 rock = vec3(0.45, 0.42, 0.38);
      color = mix(veg, rock, smoothstep(0.21, 0.35, slope));

    } else { // Snow on flat/moderate slopes, dark rock on steep crags
      vec3 snow = vec3(0.95, 0.96, 0.98);
      vec3 rock = vec3(0.38, 0.36, 0.35);
      color = mix(snow, rock, smoothstep(0.2, 0.45, slope));
    }
  }

  // Directional sunlight + ambienht sky light
  vec3 sun_dir = normalize(vec3(0.6, 0.4, 0.7));
  float diff = max(dot(n, sun_dir), 0.0);
  float light = (frag_water > 0.001) ? (0.80 + 0.20*diff) : (0.28 + 0.72*diff);
  vec3 lit_color = color * light;

  // Smooth atmospheric fog matching the clear colour (0.15, 0.35, 0.65)
  float max_dist = float(pc.grid_info.z);
  float fog = clamp((frag_dist - max_dist*0.65) / (max_dist*0.3), 0.0, 1.0);
  vec3 sky_color = vec3(0.15, 0.35, 0.65);

  out_color = vec4(mix(lit_color, sky_color, fog), 1.0);
}
