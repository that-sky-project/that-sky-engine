#include "Utils/Assert.hpp"
#include "Renderer/RendererUtils.hpp"

static Renderer *g_Renderer = nullptr;
static RendererType g_RendererType = kRendererType_Undefined;

Renderer *GetRenderer() {
  SkyAssertMsg(
    g_Renderer,
    "No Renderer has been initialized. If you need to make rendering related calls, "
    "be sure to link a Renderer library, and then call SetRenderer() on boot.");

  return g_Renderer;
}

void SetRenderer(
  Renderer *renderer
) {
  SkyAssert(renderer);

  g_Renderer = renderer;

  char id[4];
  renderer->GetIdentifier(id);

  if (!memcmp(id, "METL", 4))
    g_RendererType = kRendererType_Metal;
  else if (!memcmp(id, "VLKN", 4))
    g_RendererType = kRendererType_Vulkan;
  else if (!memcmp(id, "STUB", 4))
    g_RendererType = kRendererType_Stub;
}
