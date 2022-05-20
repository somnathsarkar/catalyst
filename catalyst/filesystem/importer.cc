#include <catalyst/filesystem/importer.h>

#include <algorithm>
#include <cctype>

#define STB_IMAGE_IMPLEMENTATION
#include <catalyst/external/stb_image.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <catalyst/dev/dev.h>

namespace catalyst {
FileType Importer::InferFiletype(const std::filesystem::path& filepath) {
  std::string extension = filepath.extension().string();
  std::transform(extension.begin(), extension.end(), extension.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  if (extension == ".png") return FileType::kImage;
  if (extension == ".dae") return FileType::kModel;
  if (extension == ".blend") return FileType::kModel;
  if (extension == ".fbx") return FileType::kModel;
  if (extension == ".obj") return FileType::kModel;
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
    case FileType::kModel: {
      AddModelResources(scene,path);
      break;
    }
    default: {
      ASSERT(false, "Unhandled file type!");
    }
  }
}
void Importer::AddModelResources(Scene& scene,
                                 const std::filesystem::path& path) {
  Assimp::Importer* importer = new Assimp::Importer();
  const aiScene* ai_scene = importer->ReadFile(
      path.string(),
      aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
  std::string stem_string = path.stem().string();
  ASSERT(ai_scene != nullptr, "Failed to load scene!");
  for (uint32_t mesh_i = 0; mesh_i<ai_scene->mNumMeshes; mesh_i++){
    const aiMesh* ai_mesh = ai_scene->mMeshes[mesh_i];
    Mesh* mesh = scene.AddMesh(stem_string);
    mesh->material_id = 0;
    for (uint32_t vi = 0; vi < ai_mesh->mNumVertices; vi++) {
      const aiVector3D& vert = ai_mesh->mVertices[vi];
      const aiVector3D& norm = ai_mesh->mNormals[vi];
      const aiVector3D& tex = ai_mesh->mTextureCoords[0][vi];
      const aiVector3D& tan = ai_mesh->mTangents[vi];
      const aiVector3D& bitan = ai_mesh->mBitangents[vi];
      Vertex v = {
          {vert.x, vert.y, vert.z}, {norm.x, norm.y, norm.z}, {tex.x, tex.y}};
      mesh->vertices.push_back(v);
    }
    for (uint32_t fi = 0; fi < ai_mesh->mNumFaces; fi++) {
      const aiFace& face = ai_mesh->mFaces[fi];
      for (uint32_t idx = 0; idx < face.mNumIndices; idx++) {
        mesh->indices.push_back(face.mIndices[idx]);
      }
    }
  }
  delete importer;
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
