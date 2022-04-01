#include <vector>

#include "ppm.h"
#include "stb_image.h"
#include "glsupport.h"
#include "texture.h"
#include "asstcommon.h"

using namespace std;

ImageTexture::ImageTexture(const char* filename, bool srgb) {
  stbi_set_flip_vertically_on_load(true);

  int width, height, channel;

  unsigned char *data = stbi_load(filename, &width, &height, &channel, 0);
  if (data)
  {
    GLenum format;
    if (channel == 1)
      format = GL_RED;
    else if (channel == 3)
      format = (!srgb) || g_Gl2Compatible ? GL_RGB : GL_SRGB;
    else if (channel == 4)
      format = (!srgb) || g_Gl2Compatible ? GL_RGBA : GL_SRGB_ALPHA;
    else {
      fprintf(stderr, "Unexpected number of channel: %d in file %s\n", channel, filename);
      assert(false);
    }

    glBindTexture(GL_TEXTURE_2D, tex);

    if (g_Gl2Compatible)
      glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);

    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

    if (!g_Gl2Compatible)
      glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    stbi_image_free(data);

    checkGlErrors();
  }
  else
  {
    std::cout << "Texture failed to load at path: " << filename << std::endl;
    stbi_image_free(data);
  }
}

DepthTexture::DepthTexture(int width, int height, GLenum internalFormat, GLenum format, GLenum type) {
  glBindTexture(GL_TEXTURE_2D, tex);
  glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, type, NULL);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

  float borderColor[] = { 0.0, 0.0, 0.0, 1.0 };
  glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

  checkGlErrors();
}
