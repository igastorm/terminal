#include <metal_stdlib>
using namespace metal;

// [[position]] とかはセマンティクスと同じ

struct VertexOut {
  float4 position [[position]];
  float4 color;
};

vertex VertexOut vertex_main(uint vertexID [[vertex_id]]) {
  // 画面中央に表示する四角形 (正規化デバイス座標系: -1.0 〜 1.0)
  // 幅 0.6, 高さ 0.4 の矩形
  float2 positions[6] = {
      float2(-0.3, -0.2), float2( 0.3, -0.2), float2(-0.3,  0.2),
      float2(-0.3,  0.2), float2( 0.3, -0.2), float2( 0.3,  0.2)
  };

  VertexOut out;
  out.position = float4(positions[vertexID], 0.0, 1.0);
  out.color = float4(0.0, 0.9, 0.46, 1.0);
  return out;
}

// ピクセルシェーダに該当するやつっぽい
fragment float4 fragment_main(VertexOut in [[stage_in]]) { return in.color; }
