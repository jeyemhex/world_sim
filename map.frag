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

struct Cell {
  float bedrock;
  float sediment;
  float water;
  float moisture;
};

layout(std430, set=0, binding=0) readonly buffer WorldBuffer {
  Cell cells[];
};

layout(push_constant) uniform PushConstants {
  mat4 mvp;
  vec4 cam_pos;
  ivec4 grid_info;
} pc;

void main() {
  float terrain_h = frag_terrain_h;
  float water_d   = frag_water;
  vec3 n = normalize(frag_normal);

  // In 2D mode, frag_normalxy holds the screen uv [0,1]
  if (pc.grid_info.w == 1) {
    vec2 uv = clamp(frag_normal.xy, 0.0, 1.0);
    int w = pc.grid_info.x;
    int h = pc.grid_info.y;
    ivec2 coord = min(ivec2(uv*vec2(w,h)), ivec2(w-1, h-1));

    // Highlight the 3D camera's current position on the 2D map with a red marker
    ivec2 cam_wrapped = ivec2(((int(floor(pc.cam_pos.x)) % w) + w) % w,
                              ((int(floor(pc.cam_pos.y)) % h) + h) % h);

    if (length(vec2(coord - cam_wrapped)) < 3.0) {
      out_color = vec4(0.95, 0.20, 0.15, 1.0);
      return;
    }

    Cell c = cells[coord.y*w + coord.x];
    terrain_h = c.bedrock + c.sediment;
    water_d   = c.water;
    n         = vec3(0.0, 0.0, 1.0); // Flat model for classic 2D unshaded look
  } else {
    n = normalize(frag_normal);
  }

  float slope = 1.0 - n.z;
  vec3 color;

  // Basic hyposometric tinting based on height h in [0.0, 1.0]
  if (water_d > 0.001) {
    // Water shaded by _actual_ depth
    float depth_factor = clamp(water_d / 0.25, 0.0, 1.0);
    color = mix(vec3(0.18, 0.48, 0.72), vec3(0.04, 0.14, 0.38), depth_factor);
  } else {
    float h = terrain_h;
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

  if (pc.grid_info.w == 1) {
    out_color = vec4(color, 1.0);
  } else {
    // Directional sunlight + ambienht sky light
    vec3 sun_dir = normalize(vec3(0.6, 0.4, 0.7));
    float diff = max(dot(n, sun_dir), 0.0);
    float light = (water_d > 0.001) ? (0.80 + 0.20*diff) : (0.28 + 0.72*diff);
    vec3 lit_color = color * light;

    // Smooth atmospheric fog matching the clear colour (0.15, 0.35, 0.65)
    float max_dist = float(pc.grid_info.z);
    float fog = clamp((frag_dist - max_dist*0.65) / (max_dist*0.3), 0.0, 1.0);
    vec3 sky_color = vec3(0.15, 0.35, 0.65);

    out_color = vec4(mix(lit_color, sky_color, fog), 1.0);
  }
}
