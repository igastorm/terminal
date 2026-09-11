#include "../Application/ImpApplication.hpp"
#include "../Application/MacApplication.h"
#include "../Window/MacWindow.h"
#include "IRenderPass.hpp"
#include "ISurface.hpp"
#include "ImpGraphics.hpp"
#include "MacGraphics.h"
#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/QuartzCore.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <new>

//  ========================================================
//
//  Render Pass
//
//  ========================================================

bool MacRenderPass::isReady() const { return this->data.is_ready; }

MacRenderPass::MacRenderPass(
    id<MTLRenderCommandEncoder> encoder,
    id<MTLRenderPipelineState> pipeline_state,
    id<MTLRenderPipelineState> pipeline_state_tex,
    id<MTLRenderPipelineState> pipeline_state_tex_outline,
    id<MTLSamplerState> sampler_state) {
  if (encoder == nil || pipeline_state == nil || pipeline_state_tex == nil ||
      pipeline_state_tex_outline == nil || sampler_state == nil) {
    this->data.is_ready = false;
    return;
  }

  this->data.encoder = encoder;
  [encoder retain];

  this->data.pipeline_state = pipeline_state;
  [this->data.pipeline_state retain];

  this->data.pipeline_state_tex = pipeline_state_tex;
  [this->data.pipeline_state_tex retain];

  this->data.pipeline_state_tex_outline = pipeline_state_tex_outline;
  [this->data.pipeline_state_tex_outline retain];

  this->data.sampler_state = sampler_state;
  [this->data.sampler_state retain];
  // this->data.vertex_buffer = vertex_buf;
  this->data.is_ready = true;
}

MacRenderPass::~MacRenderPass() {
  if (this->data.encoder != nil) {
    [this->data.encoder release];
  }
  if (this->data.pipeline_state != nil) {
    [this->data.pipeline_state release];
  }
  if (this->data.pipeline_state_tex != nil) {
    [this->data.pipeline_state_tex release];
  }
  if (this->data.pipeline_state_tex_outline != nil) {
    [this->data.pipeline_state_tex_outline release];
  }
  if (this->data.sampler_state != nil) {
    [this->data.sampler_state release];
  }
  this->data.is_ready = false;
}

template <>
bool ImpRenderPass::drawVertices(const Vertex *vertices, int vertex_count) {
  // render() 内でしか呼ばれない, 呼び出し元で既に @autoreleasepool してる
  // そもそもここで使ってるメソッドはリソース生成しないらしい
  if (this->data.is_ready == false || vertices == nil || vertex_count <= 0) {
    return false;
  }

  [this->data.encoder setRenderPipelineState:this->data.pipeline_state];

  // void *ptr = [this->data.vertex_buffer contents];
  // std::memcpy(ptr, vertices, sizeof(Vertex) * vertex_count);

  // スロット 0 に頂点データ をセット(4KB
  // 以内ならバッファを使わなくていいらしい)
  [this->data.encoder setVertexBytes:vertices
                              length:sizeof(Vertex) * vertex_count
                             atIndex:0];

  // スロット 0 に頂点バッファをセット
  //[this->data.encoder setVertexBuffer:this->data.vertex_buffer
  //                             offset:0
  //                            atIndex:0];

  [this->data.encoder drawPrimitives:MTLPrimitiveTypeTriangle
                         vertexStart:0
                         vertexCount:vertex_count];

  return true;
}

template <>
bool ImpRenderPass::drawVerticesTex(ITexture *itexture,
                                    const VertexTex *vertices,
                                    int vertex_count) {
  // render() 内でしか呼ばれない, 呼び出し元で既に @autoreleasepool してる
  // そもそもここで使ってるメソッドはリソース生成しないらしい
  if (this->data.is_ready == false || vertices == nil || vertex_count <= 0 ||
      itexture == nullptr) {
    return false;
  }

  MacTexture *texture = static_cast<MacTexture *>(itexture);
  id<MTLTexture> mtl_texrure = texture->getPlatformData().texture;
  TextureFormat format = texture->getPlatformData().format;

  if (mtl_texrure == nil) {
    return false;
  }

  // テクスチャのフォーマットにより適切なパイプラインを自動選択
  [this->data.encoder
      setRenderPipelineState:format == TextureFormat::Color
                                 ? this->data.pipeline_state_tex
                                 : this->data.pipeline_state_tex_outline];
  
  [this->data.encoder setFragmentSamplerState:this->data.sampler_state
                                      atIndex:0];

  // void *ptr = [this->data.vertex_buffer contents];
  // std::memcpy(ptr, vertices, sizeof(Vertex) * vertex_count);

  [this->data.encoder setFragmentTexture:mtl_texrure atIndex:0];

  // スロット 0 に頂点データ をセット(4KB
  // 以内ならバッファを使わなくていいらしい)
  [this->data.encoder setVertexBytes:vertices
                              length:sizeof(VertexTex) * vertex_count
                             atIndex:0];

  // スロット 0 に頂点バッファをセット
  //[this->data.encoder setVertexBuffer:this->data.vertex_buffer
  //                             offset:0
  //                            atIndex:0];

  [this->data.encoder drawPrimitives:MTLPrimitiveTypeTriangle
                         vertexStart:0
                         vertexCount:vertex_count];

  return true;
}
