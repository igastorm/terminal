#include <metal_stdlib>
using namespace metal;

// [[position]] とかはセマンティクスと同じ

// simd を使うと CPU 側とメモリの配置がズレるっぽいから共通部分は C 言語スタイル
struct Vertex {
  float position[2];
  uint8_t color[4];
};

struct VertexOut {
  vector_float4 position [[position]];
  vector_float4 color;
};

// 画面サイズ
struct ViewportUniform {
  float width;
  float r_height; // 2.0 x 逆数
};

vertex VertexOut vertex_main(const device Vertex *vertices [[buffer(0)]],
                             constant ViewportUniform &uniforms [[buffer(1)]],
                             uint vertexID [[vertex_id]]) {
  float2 pixel_pos =
      float2(vertices[vertexID].position[0], vertices[vertexID].position[1]);

  // float ndc_x = (pixel_pos.x / uniforms.r_width) * 2.0 - 1.0;
  float ndc_x = pixel_pos.x * 2.0 - uniforms.width;
  float ndc_y = (1.0 - (pixel_pos.y * uniforms.r_height)) * uniforms.width;

  VertexOut out;
  out.position = float4(ndc_x, ndc_y, 0.0f, uniforms.width);

  // ARGB ↓配列のようにアクセスするとリトルエンディアンにより逆読みになる
  // BGRA ←リトルエンディアン
  // BGRA
  // CPU 側で設定したフォーマットに関わらず RGBA の順になる
  out.color = float4(vertices[vertexID].color[2], vertices[vertexID].color[1],
                     vertices[vertexID].color[0], vertices[vertexID].color[3]) /
              255.0f;
  return out;
}

// ピクセルシェーダに該当するやつっぽい
fragment float4 fragment_main(VertexOut in [[stage_in]]) { return in.color; }
