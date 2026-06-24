#ifndef __VERTEX_H__
#define __VERTEX_H__

class VertexData {
public:
  static constexpr int kMaxVertexBuffers = 4;

  void AddVertexBuffer();
  void AddIndexBuffer();
  void BeginDefinition();
  void EndDefinition();
};

class VertexRender {
public:

};

#endif
