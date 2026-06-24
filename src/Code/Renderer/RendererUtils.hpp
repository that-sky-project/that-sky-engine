#ifndef __RENDERER_RENDERERUTILS_H__
#define __RENDERER_RENDERERUTILS_H__

class Renderer {
public:
  Renderer() = default;

  virtual ~Renderer() = default;
  virtual void BeginFrame(bool) = 0;
};

#endif
