#pragma once
#include <filesystem>
#include <cstdint>

#include <catalyst/scene/scene.h>

namespace catalyst {
enum class FileType {
  kUnknown = 0,
  kImage = 1,
  kModel = 2,
};
class Importer {
 public:
  static FileType InferFiletype(const std::filesystem::path& filepath);
  static void AddResources(Scene& scene, const std::filesystem::path& path);
  static void AddModelResources(Scene& scene,
                                const std::filesystem::path& path);
 
private:
};
class TextureData {
  friend class TextureImporter;

 public:
  uint32_t width;
  uint32_t height;
  uint32_t channels;
  unsigned char* data;

 private:
  TextureData();
  ~TextureData();

  // Uncopyable
  TextureData(const TextureData&) = delete;
  const TextureData& operator=(const TextureData&) = delete;
};
class TextureImporter {
 public:
  TextureImporter();
  ~TextureImporter();
  bool ReadFile(const std::filesystem::path& path);
  const TextureData* GetData();
  void DestroyData();

  private:
  TextureData* data;

  // Uncopyable
  TextureImporter(const TextureImporter&) = delete;
  const TextureImporter& operator=(const TextureImporter&) = delete;
};
};  // namespace catalylst