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

layout(location = 0) out float frag_terrain_h;
layout(location = 1) out float frag_water;
layout(location = 2) out float frag_sediment;
layout(location = 3) out vec3 frag_normal;
layout(location = 4) out float frag_dist;

struct Cell {
  float bedrock;
  float sediment;
  float water;
  float moisture;
};

layout(std430, set=0, binding=0) readonly buffer WorldBuffer {
  Cell cells[];
};

layout(push_constant) uniform PushConstants{
  mat4 mvp;
  vec4 cam_pos;     // w = vertical height scale; x,y,z = camera world position
  ivec4 grid_info;  // w = view_mode, x = grid_w; y = grid_h; z = view_radius
} pc;

Cell sample_cell(int x, int y) {
  int w = pc.grid_info.x;
  int h = pc.grid_info.y;
  int ix = ((x%w) + w) % w;
  int iy = ((y%h) + h) % h;
  return cells[iy*w + ix];
}

float surface_z(int x, int y) {
  Cell c = sample_cell(x,y);
  return (c.bedrock + c.sediment + c.water) * pc.cam_pos.w;
};

void main() {
  // 2D top-down mode: 3-vertex fullscreen triangle
  if (pc.grid_info.w == 1) {
    vec2 uv = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    frag_normal    = vec3(uv, 1.0);
    frag_terrain_h = 0.0;
    frag_water     = 0.0;
    frag_sediment  = 0.0;
    frag_dist      = 0.0;
    gl_Position    = vec4(uv * 2.0f - 1.0f, 0.0f, 1.0f);
    return;
  }

  // 3D perspective
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

  Cell c = sample_cell(v_cell.x, v_cell.y);
  frag_terrain_h = c.bedrock + c.sediment;
  frag_water = c.water;
  frag_sediment = c.sediment;

  float z_pos = surface_z(v_cell.x, v_cell.y);

  // Central differences for periodic surface normal
  float hL = surface_z(v_cell.x-1, v_cell.y);
  float hR = surface_z(v_cell.x+1, v_cell.y);
  float hD = surface_z(v_cell.x, v_cell.y-1);
  float hU = surface_z(v_cell.x, v_cell.y+1);
  frag_normal = normalize(vec3(hL-hR, hD-hU, 2.0));

  vec3 world_pos = vec3(float(v_cell.x), float(v_cell.y), z_pos);
  frag_dist = length(world_pos.xy - pc.cam_pos.xy);

  gl_Position = pc.mvp * vec4(world_pos, 1.0);
}
