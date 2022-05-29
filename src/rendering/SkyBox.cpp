//
// Created by depau on 5/29/22.
//

#include "rendering/SkyBox.h"

void SkyBox::Draw() {
  // We are inside the cube, we need to disable backface culling!
  rlgl::rlDisableBackfaceCulling();
  rlgl::rlDisableDepthMask();

  skybox.Draw((Vector3){ 0, 0, 0 }, 1.0f, WHITE);

  rlgl::rlEnableBackfaceCulling();
  rlgl::rlEnableDepthMask();
}

SkyBox::SkyBox(const std::string &imagePath) {
  skybox.materials[0].shader = shader;

  int environmentMapVal[] = { MATERIAL_MAP_CUBEMAP };
  int doGammaVal[] = { 0 };
  int vflippedVal[] = { 0 };
  shader.SetValue(shader.GetLocation("environmentMap"), environmentMapVal, SHADER_UNIFORM_INT);
  shader.SetValue(shader.GetLocation("doGamma"), doGammaVal, SHADER_UNIFORM_INT);
  shader.SetValue(shader.GetLocation("vflipped"), vflippedVal, SHADER_UNIFORM_INT);

  LoadTexture(imagePath);
}

void SkyBox::LoadTexture(const std::string &imagePath) {
  raylib::Image image(imagePath);
  texture = std::make_unique<raylib::TextureCubemap>(image, CUBEMAP_LAYOUT_AUTO_DETECT);
  skybox.materials[0].maps[MATERIAL_MAP_CUBEMAP].texture = *texture;
}
