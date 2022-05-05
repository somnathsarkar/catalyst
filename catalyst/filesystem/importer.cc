#include <catalyst/filesystem/importer.h>

#include <algorithm>
#include <cctype>

#define STB_IMAGE_IMPLEMENTATION
#include <catalyst/external/stb_image.h>

#include <catalyst/dev/dev.h>

namespace catalyst {
FileType Importer::InferFiletype(const std::filesystem::path& filepath) {
  std::string extension = filepath.extension().string();
  std::transform(extension.begin(), extension.end(), extension.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  if (extension == ".png") return FileType::kImage;
  return FileType::kUnknown;
}
void Importer::AddResources(Scene& scene, const std::filesystem::path& path) {
  FileType type = InferFiletype(path);
  ASSERT(type != FileType::kUnknown, "Unknown file type!");
  std::string path_string = path.string();
  std::string stem_string = path.stem().string();
  switch (type) { 
    case FileType::kImage: {
      Texture* tex = scene.AddTexture(stem_string);
      tex->path_ = path_string;
      break;
    }
    default: {
      ASSERT(false, "Unhandled file type!");
    }
  }
}
TextureImporter::TextureImporter() : data(nullptr) {}
TextureImporter::~TextureImporter() { DestroyData(); }
bool TextureImporter::ReadFile(const std::filesystem::path& path) {
  int x, y, n;
  unsigned char* tex_data = stbi_load(path.string().c_str(), &x, &y, &n, 4);
  if (tex_data == nullptr) return false;
  data = new TextureData();
  data->width = x;
  data->height = y;
  data->channels = 4;
  data->data = tex_data;
  return true;
}
const TextureData* TextureImporter::GetData() {
  ASSERT(data != nullptr, "No data loaded!");
  return data;
}
void TextureImporter::DestroyData() { delete data; }
TextureData::TextureData() : width(0),height(0),channels(0),data(nullptr) {}
TextureData::~TextureData() {
  if (data != nullptr) {
    stbi_image_free(data);
    data = nullptr;
  }
  width = 0;
  height = 0;
  channels = 0;
}
}  // namespace catalyst
