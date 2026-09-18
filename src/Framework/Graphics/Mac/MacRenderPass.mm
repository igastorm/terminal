#include "MacRenderPass.h"
#include "MacGraphicsDevice.h"
#include "MacRenderPass.h"
#include "MacTexture.h"
#include <cstdlib>
#include <new>

//  ========================================================
//
//  Render Pass
//
//  ========================================================

bool MacRenderPass::isReady() const { return this->data.is_ready; }

MacRenderPass::MacRenderPass(IGraphicsDevice *device,
                             id<MTLRenderCommandEncoder> encoder) {
  if (encoder == nil || device == nullptr) {
    this->data.is_ready = false;
    return;
  }

  this->data.device = device;
  this->data.device->addRef();

  MacGraphicsDevice *mac_device = static_cast<MacGraphicsDevice *>(device);

  id<MTLRenderPipelineState> pipeline_state =
      mac_device->getPlatformData().pipeline_state;
  id<MTLRenderPipelineState> pipeline_state_tex =
      mac_device->getPlatformData().pipeline_state_tex;
  id<MTLRenderPipelineState> pipeline_state_tex_outline =
      mac_device->getPlatformData().pipeline_state_tex_outline;
  id<MTLSamplerState> sampler_state =
      mac_device->getPlatformData().sampler_state;

  if (encoder == nil || pipeline_state == nil || pipeline_state_tex == nil ||
      pipeline_state_tex_outline == nil || sampler_state == nil) {
    this->data.is_ready = false;
    return;
  }

  this->data.encoder = encoder;
  [encoder retain];
  // this->data.vertex_buffer = vertex_buf;
  this->data.is_ready = true;
}

MacRenderPass::~MacRenderPass() {
  if (this->data.device != nullptr) {
    this->data.device->release();
    this->data.device = nullptr;
  }
  if (this->data.encoder != nil) {
    [this->data.encoder release];
  }
  this->data.is_ready = false;
}

template <>
bool RenderPass::drawVertices(const Vertex *vertices, int vertex_count) {
  // render() 内でしか呼ばれない, 呼び出し元で既に @autoreleasepool してる
  // そもそもここで使ってるメソッドはリソース生成しないらしい
  if (this->data.is_ready == false || vertices == nil || vertex_count <= 0) {
    return false;
  }

  MacGraphicsDevice *mac_device =
      static_cast<MacGraphicsDevice *>(this->data.device);

  [this->data.encoder
      setRenderPipelineState:mac_device->getPlatformData().pipeline_state];

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
bool RenderPass::drawVerticesTex(ITexture *itexture, const VertexTex *vertices,
                                 int vertex_count) {
  // render() 内でしか呼ばれない, 呼び出し元で既に @autoreleasepool してる
  // そもそもここで使ってるメソッドはリソース生成しないらしい
  if (this->data.is_ready == false || vertices == nil || vertex_count <= 0 ||
      itexture == nullptr) {
    return false;
  }

  MacTexture *texture = static_cast<MacTexture *>(itexture);
  id<MTLTexture> mtl_texrure = texture->getPlatformData().mtl_texture;
  TextureFormat format = texture->getFormat();

  if (mtl_texrure == nil) {
    return false;
  }

  MacGraphicsDevice *mac_device =
      static_cast<MacGraphicsDevice *>(this->data.device);

  // テクスチャのフォーマットにより適切なパイプラインを自動選択
  [this->data.encoder
      setRenderPipelineState:format == TextureFormat::Color
                                 ? mac_device->getPlatformData()
                                       .pipeline_state_tex
                                 : mac_device->getPlatformData()
                                       .pipeline_state_tex_outline];

  [this->data.encoder
      setFragmentSamplerState:mac_device->getPlatformData().sampler_state
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
