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

layout(location = 0) out vec2 frag_uv;

void main() {
  // Generates a single triangle covering [-1,1] clip space:
  //   index 0: (-1, -1), UV (0, 0)
  //   index 1: ( 3, -1), UV (2, 0)
  //   index 2: (-1,  3), UV (0, 2)
  frag_uv = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
  gl_Position = vec4(frag_uv * 2.0f - 1.0f, 0.0f, 1.0f);
}
