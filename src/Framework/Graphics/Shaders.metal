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
  float inv_255;  // 255 の逆数
};

VertexOut vertexConvert(float2 pos, uint4 col, ViewportUniform uniforms) {

  // float ndc_x = (pixel_pos.x / uniforms.r_width) * 2.0 - 1.0;
  float ndc_x = pos.x * 2.0 - uniforms.width;
  float ndc_y = (1.0 - (pos.y * uniforms.r_height)) * uniforms.width;

  VertexOut out;
  // ARGB ↓配列のようにアクセスするとリトルエンディアンにより逆読みになる
  // BGRA ←リトルエンディアン
  // BGRA
  // CPU 側で設定したフォーマットに関わらず RGBA の順になる
  out.color = float4(col[2], col[1], col[0], col[3]) * uniforms.inv_255;
  // 前の処理に依存する処理はパフォーマンス的に離れた場所に書いた方がいいのか
  // (前の処理待つ間に別の処理を行う的な)
  out.position = float4(ndc_x, ndc_y, 0.0f, uniforms.width);
  return out;
}

struct VertexWithUV {
  float position[2];
  float uv[2];
  uint8_t color[4];
};

struct VertexOutWithUV {
  vector_float4 position [[position]];
  vector_float4 color;
  vector_float2 uv;
};

vertex VertexOut vertexMain(const device Vertex *vertices [[buffer(0)]],
                            constant ViewportUniform &uniforms [[buffer(1)]],
                            uint vertexID [[vertex_id]]) {

  return vertexConvert(
      float2(vertices[vertexID].position[0], vertices[vertexID].position[1]),
      uint4(vertices[vertexID].color[0], vertices[vertexID].color[1],
            vertices[vertexID].color[2], vertices[vertexID].color[3]),
      uniforms);
}

vertex VertexOutWithUV vertexMainUV(const device VertexWithUV *vertices
                                    [[buffer(0)]],
                                    constant ViewportUniform &uniforms
                                    [[buffer(1)]],
                                    uint vertexID [[vertex_id]]) {

  VertexOut tmp = vertexConvert(
      float2(vertices[vertexID].position[0], vertices[vertexID].position[1]),
      uint4(vertices[vertexID].color[0], vertices[vertexID].color[1],
            vertices[vertexID].color[2], vertices[vertexID].color[3]),
      uniforms);
  VertexOutWithUV out;
  out.position = tmp.position;
  out.color = tmp.color;
  out.uv = float2(vertices[vertexID].uv[0], vertices[vertexID].uv[1]);
  return out;
}

// ピクセルシェーダに該当するやつっぽい
fragment float4 fragmentMain(VertexOut in [[stage_in]]) { return in.color; }

fragment float4 fragmentMainTex(VertexOutWithUV in [[stage_in]],
                                texture2d<float> tex [[texture(0)]],
                                sampler samp [[sampler(0)]]) {
  // テクスチャサンプリング
  // float4 out = in.color + tex.sample(samp, in.uv) - 1.0f;
  float4 out = tex.sample(samp, in.uv);
  return out;
}
