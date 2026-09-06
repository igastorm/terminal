#include <metal_stdlib>
using namespace metal;

// [[position]] とかはセマンティクスと同じ

struct Vertex {
  float position[2];
  uint8_t color[4];
};

struct VertexOut {
  vector_float4 position [[position]];
  vector_float4 color;
};

vertex VertexOut vertex_main(const device Vertex *vertices [[buffer(0)]],
                             uint vertexID [[vertex_id]]) {
  VertexOut out;
  out.position = float4(vertices[vertexID].position[0],
                        vertices[vertexID].position[1], 0.0f, 1.0f);
  out.color = float4(vertices[vertexID].color[2], vertices[vertexID].color[1],
                     vertices[vertexID].color[0], vertices[vertexID].color[3]) / 255.0f;
  return out;
}

// ピクセルシェーダに該当するやつっぽい
fragment float4 fragment_main(VertexOut in [[stage_in]]) {
  return in.color;
}
