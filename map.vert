/*==============================================================================*
 * MAP_VERT
 *------------------------------------------------------------------------------*
 * Author:  Ed Higgins <ed.j.higgins@gmail.com>
 *------------------------------------------------------------------------------*
 * Version: 0.1.1, 2026-11-03
 *------------------------------------------------------------------------------*
 * This code is distributed under the GLWTPL
 *==============================================================================*/

#version 460

layout(location = 0) out float frag_h;
layout(location = 1) out vec3 frag_normal;
layout(location = 2) out float frag_dist;

layout(std430, set=0, binding=0) readonly buffer HeightmapBuffer {
  float heights[];
};

layout(push_constant) uniform PushConstants{
  mat4 mvp;
  vec4 cam_pos;     // w = vertical height scale; x,y,z = camera world position
  ivec4 grid_info;  // w = grid_w; y = grid_h; z = view_radius
} pc;

float sample_h(int x, int y) {
  int w = pc.grid_info.x;
  int h = pc.grid_info.y;
  int ix = ((x%w) + w) % w;
  int iy = ((y%h) + h) % h;
  return heights[iy*w + ix];
}

void main() {
  int R = pc.grid_info.z;
  int side = 2*R;

  int quad_idx = gl_VertexIndex / 6;
  int vert_in_quad = gl_VertexIndex % 6;

  // Two triangles per cell
  const ivec2 offsets[6] = ivec2[6](
    ivec2(0,0), ivec2(1,0), ivec2(0,1),  // Triangle 1
    ivec2(1,0), ivec2(1,1), ivec2(0,1)   // Triangle 2
  );

  // Center the rendered patch on the camera's integer xy cell
  ivec2 cam_cell = ivec2(floor(pc.cam_pos.xy));
  int qx = (quad_idx % side) - R + cam_cell.x;
  int qy = (quad_idx / side) - R + cam_cell.y;

  ivec2 v_cell = ivec2(qx, qy) + offsets[vert_in_quad];

  float h_raw = sample_h(v_cell.x, v_cell.y);
  frag_h = h_raw;

  // Flatten water surface at sea level (0.35) in 3D space while keepign frag_h for water depth colour
  float h_geom = max(h_raw, 0.35);
  float z_scale = pc.cam_pos.w;

  // Central differences for periodic surface normal
  float hL = max(sample_h(v_cell.x-1, v_cell.y), 0.35) * z_scale;
  float hR = max(sample_h(v_cell.x+1, v_cell.y), 0.35) * z_scale;
  float hD = max(sample_h(v_cell.x, v_cell.y-1), 0.35) * z_scale;
  float hU = max(sample_h(v_cell.x, v_cell.y+1), 0.35) * z_scale;
  frag_normal = normalize(vec3(hL-hR, hD-hU, 2.0));

  vec3 world_pos = vec3(float(v_cell.x), float(v_cell.x), h_geom * z_scale); 
  frag_dist = length(world_pos.xy - pc.cam_pos.xy);

  gl_Position = pc.mvp * vec4(world_pos, 1.0);
}
