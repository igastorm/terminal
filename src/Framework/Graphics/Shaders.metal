#include <metal_stdlib>
using namespace metal;

// [[position]] とかはセマンティクスと同じ

struct VertexOut {
  float4 potition [[position]];
  float4 color;
};

vertex VertexOut vertex_main(uint vertedID [[vertex_id]]) {
  VertexOut out;
  return out;
}

// ピクセルシェーダに該当するやつっぽい
fragment float4 fragment_main(VertexOut in [[stage_in]]) { return in.color; }
