#include <catalyst/scene/scene.h>

#include <numeric>
#include <sstream>

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <catalyst/dev/dev.h>

namespace catalyst {
Scene::Scene() {
  root_ = new SceneObject(this, "root");
  object_name_map_["root"] = root_;
  CreatePrimitiveMeshes();
  CreatePrimitiveMaterials();
}
Scene::~Scene() { delete root_; }
MeshObject* Scene::AddPrimitive(SceneObject* parent, PrimitiveType type) {
  std::string object_name =
      GetAvailableObjectName(kPrimitiveNames[static_cast<uint32_t>(type)]);
  MeshObject* mesh_object =
      new MeshObject(this, object_name,
                     static_cast<uint32_t>(type));
  mesh_object->parent_ = parent;
  parent->children_.push_back(mesh_object);
  object_name_map_[object_name] = mesh_object;
  return mesh_object;
}
CameraObject* Scene::AddCamera(SceneObject* parent, CameraType type) {
  Camera camera;
  cameras_.push_back(camera);
  uint32_t camera_id = static_cast<uint32_t>(cameras_.size())-1;
  std::string object_name = GetAvailableObjectName("Camera");
  CameraObject* camera_object = new CameraObject(this, object_name,camera_id);
  camera_object->parent_ = parent;
  parent->children_.push_back(camera_object);
  object_name_map_[object_name] = camera_object;
  return camera_object;
}
DirectionalLightObject* Scene::AddDirectionalLight(SceneObject* parent,
                                                   glm::vec3 color) {
  std::string object_name =
      GetAvailableObjectName("Directional Light");
  DirectionalLightObject* light_object =
      new DirectionalLightObject(this, object_name, color);
  light_object->parent_ = parent;
  parent->children_.push_back(light_object);
  object_name_map_[object_name] = light_object;
  return light_object;
}
MeshObject* Scene::AddMeshObject(SceneObject* parent, Mesh* mesh) {
  std::string object_name =
      GetAvailableObjectName(mesh->name_);
  uint32_t mesh_index = 0;
  bool mesh_found = false;
  for (uint32_t mesh_i = 0; mesh_i < static_cast<uint32_t>(meshes_.size());
       mesh_i++) {
    if (mesh == meshes_[mesh_i]) {
      mesh_index = mesh_i;
      mesh_found = true;
      break;
    }
  }
  ASSERT(mesh_found, "Could not find mesh!");
  MeshObject* mesh_object =
      new MeshObject(this, object_name, mesh_index);
  mesh_object->parent_ = parent;
  parent->children_.push_back(mesh_object);
  object_name_map_[object_name] = mesh_object;
  return mesh_object;
}
SceneObject* Scene::AddResourceToScene(Resource* resource) { 
  switch (resource->type_) {
    case ResourceType::kMesh: {
      Mesh* mesh = static_cast<Mesh*>(resource);
      return AddMeshObject(root_, mesh);
      break;
    }
    default: {
      break;
    }
  }
  return nullptr;
}
Mesh* Scene::AddMesh(const std::string& name) {
  std::string mesh_name = GetAvailableResourceName(name);
  Mesh* mesh = new Mesh(this, mesh_name);
  meshes_.push_back(mesh);
  resource_name_map_[mesh_name] = mesh;
  return mesh;
}
Material* Scene::AddMaterial(const std::string& name) {
  std::string mat_name = GetAvailableResourceName(name);
  Material* mat = new Material(this, mat_name);
  materials_.push_back(mat);
  resource_name_map_[mat_name] = mat;
  return mat;
}
Texture* Scene::AddTexture(const std::string& name) {
  std::string tex_name = GetAvailableResourceName(name);
  Texture* tex = new Texture(this,tex_name);
  textures_.push_back(tex);
  resource_name_map_[tex_name] = tex;
  return tex;
}
void Scene::DuplicateResource(const Resource* resource) {
  const std::string& initial_name = resource->name_;
  std::string new_name = GetAvailableResourceName(initial_name);
  switch (resource->type_) {
    case ResourceType::kMesh: {
      const Mesh* old_mesh = static_cast<const Mesh*>(resource);
      Mesh *new_mesh = AddMesh(new_name);
      new_mesh->vertices = old_mesh->vertices;
      new_mesh->indices = old_mesh->indices;
      new_mesh->material_id = old_mesh->material_id;
      break;
    }
    case ResourceType::kMaterial: {
      const Material* old_mat = static_cast<const Material*>(resource);
      Material* new_mat = AddMaterial(new_name);
      new_mat->albedo_ = old_mat->albedo_;
      new_mat->albedo_texture_id_ = old_mat->albedo_texture_id_;
      new_mat->metallic_ = old_mat->metallic_;
      new_mat->metallic_texture_id_ = old_mat->metallic_texture_id_;
      new_mat->reflectance_ = old_mat->reflectance_;
      new_mat->roughness_ = old_mat->roughness_;
      new_mat->roughness_texture_id_ = old_mat->roughness_texture_id_;
      new_mat->normal_texture_id_ = old_mat->normal_texture_id_;
      break;
    }
    default: {
      ASSERT(false,"Unhandled resource type!");
      break;
    }
  }
}
SceneObject* Scene::GetObjectByName(const std::string& name) const {
  return object_name_map_.find(name)->second;
}
Resource* Scene::GetResourceByName(const std::string& name){
  return resource_name_map_.find(name)->second;
}
glm::mat4 Scene::GetParentTransform(
    const SceneObject* scene_object) const {
  const SceneObject* focus = scene_object;
  glm::mat4 result(1.0f);
  while (focus != nullptr && focus->parent_!=nullptr) {
    focus = focus->parent_;
    result = focus->transform_.GetTransformationMatrix() * result;
  }
  return result;
}
Aabb Scene::ComputeAabb(
    const std::vector<const SceneObject*> scene_objects) const {
  ASSERT(!scene_objects.empty(), "Attempting to compute AABB of empty set!");
  Aabb aabb = ComputeAabb(scene_objects[0]);
  for (uint32_t obj_i = 1; obj_i < static_cast<uint32_t>(scene_objects.size());
       obj_i++) {
    aabb.Extend(ComputeAabb(scene_objects[obj_i]));
  }
  return aabb;
}
inline bool Scene::CheckObjectNameExists(const std::string& s) {
  return (object_name_map_.find(s) != object_name_map_.end());
}
std::string Scene::GetAvailableObjectName(const std::string& prefix) {
  if (!CheckObjectNameExists(prefix)) return prefix;
  uint32_t off_i = 1;
  while (true) {
    std::ostringstream name_stream;
    name_stream << prefix << " (" << off_i << ")";
    if (!CheckObjectNameExists(name_stream.str())) return name_stream.str();
    off_i++;
  }
}
inline bool Scene::CheckResourceNameExists(const std::string& s) {
  return (resource_name_map_.find(s) != resource_name_map_.end());
}
std::string Scene::GetAvailableResourceName(const std::string& prefix) {
  if (!CheckResourceNameExists(prefix)) return prefix;
  uint32_t off_i = 1;
  while (true) {
    std::ostringstream name_stream;
    name_stream << prefix << " (" << off_i << ")";
    if (!CheckResourceNameExists(name_stream.str())) return name_stream.str();
    off_i++;
  }
}
void Scene::CreatePrimitiveMeshes() {
  // Empty
  Mesh* empty =
      AddMesh(kPrimitiveNames[static_cast<uint32_t>(PrimitiveType::kEmpty)]);
  empty->vertices.clear();
  empty->indices.clear();
  empty->material_id = 0;
  // Cube
  Mesh* cube =
      AddMesh(kPrimitiveNames[static_cast<uint32_t>(PrimitiveType::kCube)]);
  cube->vertices = {
      {{-0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.875f, 0.5f}},
      {{0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.625f, 0.75f}},
      {{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.625f, 0.5f}},
      {{0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}, {0.625f, 0.75f}},
      {{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {0.375f, 1.0f}},
      {{0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {0.375f, 0.75f}},
      {{-0.5f, -0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}, {0.625f, 0.0f}},
      {{-0.5f, 0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.375f, 0.25f}},
      {{-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.375f, 0.0f}},
      {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.375f, 0.5f}},
      {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.125f, 0.75f}},
      {{-0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.125f, 0.5f}},
      {{0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {0.625f, 0.5f}},
      {{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.375f, 0.75f}},
      {{0.5f, 0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.375f, 0.5f}},
      {{-0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {0.625f, 0.25f}},
      {{0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.375f, 0.5f}},
      {{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.375f, 0.25f}},
      {{-0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.875f, 0.5f}},
      {{-0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.875f, 0.75f}},
      {{0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.625f, 0.75f}},
      {{0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}, {0.625f, 0.75f}},
      {{-0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}, {0.625f, 1.0f}},
      {{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {0.375f, 1.0f}},
      {{-0.5f, -0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}, {0.625f, 0.0f}},
      {{-0.5f, 0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}, {0.625f, 0.25f}},
      {{-0.5f, 0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.375f, 0.25f}},
      {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.375f, 0.5f}},
      {{0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.375f, 0.75f}},
      {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.125f, 0.75f}},
      {{0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {0.625f, 0.5f}},
      {{0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {0.625f, 0.75f}},
      {{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.375f, 0.75f}},
      {{-0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {0.625f, 0.25f}},
      {{0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {0.625f, 0.5f}},
      {{0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.375f, 0.5f}}};
  cube->indices.resize(cube->vertices.size());
  std::iota(cube->indices.begin(), cube->indices.end(), 0);
  cube->material_id = 0;
  // Teapot
  Mesh* teapot =
      AddMesh(kPrimitiveNames[static_cast<uint32_t>(PrimitiveType::kTeapot)]);
  Assimp::Importer *importer = new Assimp::Importer();
  const aiScene* teapot_scene = importer->ReadFile(
      "../assets/models/teapot.obj",
      aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_GenUVCoords);
  const aiMesh* teapot_mesh = teapot_scene->mMeshes[0];
  uint32_t num_vertices = teapot_mesh->mNumVertices;
  glm::mat4 teapot_transform3 =
      glm::rotate(glm::half_pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f));
  teapot_transform3 = glm::scale(teapot_transform3, glm::vec3(0.5f));
  glm::mat3 teapot_transform = glm::mat3(teapot_transform3);
  for (uint32_t vertex_i = 0; vertex_i < num_vertices; vertex_i++) {
    const aiVector3D ai_vertex = teapot_mesh->mVertices[vertex_i];
    const aiVector3D ai_normal = teapot_mesh->mNormals[vertex_i].Normalize();
    glm::vec3 vertex = {ai_vertex.x, ai_vertex.y, ai_vertex.z};
    glm::vec3 normal = {ai_normal.x, ai_normal.y, ai_normal.z};
    vertex = teapot_transform * vertex;
    vertex.z -= 0.5;
    normal = glm::normalize(teapot_transform * normal);
    teapot->vertices.push_back({vertex, normal, {0.0f, 0.0f}});
  }
  teapot->indices.resize(teapot->vertices.size());
  std::iota(teapot->indices.begin(), teapot->indices.end(), 0);
  teapot->material_id = 0;
  // Bunny 
  Mesh* bunny =
      AddMesh(kPrimitiveNames[static_cast<uint32_t>(PrimitiveType::kBunny)]);
  importer->FreeScene();
  const aiScene* bunny_scene =
      importer->ReadFile("../assets/models/bun_zipper.obj",
                         aiProcess_Triangulate | aiProcess_GenSmoothNormals |
                             aiProcess_GenUVCoords);
  const aiMesh* bunny_mesh = bunny_scene->mMeshes[0];
  num_vertices = bunny_mesh->mNumVertices;
  glm::mat4 bunny_transform3 =
      glm::rotate(glm::half_pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f));
  bunny_transform3 = glm::scale(bunny_transform3, glm::vec3(2.0f));
  glm::mat3 bunny_transform = glm::mat3(bunny_transform3);
  for (uint32_t vertex_i = 0; vertex_i < num_vertices; vertex_i++) {
    const aiVector3D ai_vertex = bunny_mesh->mVertices[vertex_i];
    const aiVector3D ai_normal = bunny_mesh->mNormals[vertex_i].Normalize();
    glm::vec3 vertex = {ai_vertex.x, ai_vertex.y, ai_vertex.z};
    glm::vec3 normal = {ai_normal.x, ai_normal.y, ai_normal.z};
    vertex = bunny_transform * vertex;
    normal = glm::normalize(bunny_transform * normal);
    bunny->vertices.push_back({vertex, normal, {0.0f, 0.0f}});
  }
  bunny->indices.resize(bunny->vertices.size());
  std::iota(bunny->indices.begin(), bunny->indices.end(), 0);
  bunny->material_id = 0;
}
void Scene::CreatePrimitiveMaterials() {
  Material* standard = AddMaterial("Standard");
  standard->albedo_ = glm::vec3(1.0f, 1.0f, 1.0f);
  standard->reflectance_ = 1.0f;
  standard->metallic_ = 1.0f;
  standard->roughness_ = 0.1f;
  standard->albedo_texture_id_ = -1;
  standard->metallic_texture_id_ = -1;
  standard->normal_texture_id_ = -1;
  standard->roughness_texture_id_ = -1;
}
Aabb Scene::ComputeAabb(const SceneObject* scene_object) const {
  glm::mat4 parent_transform = GetParentTransform(scene_object);
  Aabb aabb(
      glm::vec3(parent_transform *
                glm::vec4(scene_object->transform_.GetTranslation(), 1.0f)));
  AabbDfs(scene_object, aabb, parent_transform);
  return aabb;
}
void Scene::AabbDfs(const SceneObject* focus, Aabb& aabb,
                    glm::mat4 transform) const {
  glm::vec3 translate_point = glm::vec3(
      transform * glm::vec4(focus->transform_.GetTranslation(), 1.0f));
  aabb.Extend(translate_point);
  transform *= focus->transform_.GetTransformationMatrix();
  if (focus->type_ == SceneObjectType::kMesh) {
    const MeshObject* mesh_object = static_cast<const MeshObject*>(focus);
    const Mesh* mesh = meshes_[mesh_object->mesh_id_];
    for (const Vertex& vert : mesh->vertices) {
      glm::vec3 vpos = glm::vec3(transform * glm::vec4(vert.position, 1.0f));
      aabb.Extend(vpos);
    }
  }
  for (const SceneObject* child : focus->children_) {
    if (!child->external_) {
      AabbDfs(child, aabb, transform);
    }
  }
}

Camera::Camera() {
  type_ = CameraType::kPerspective;
  fovx_ = 100;
}
glm::mat4 Camera::GetViewToClipTransform(uint32_t screen_width, uint32_t screen_height) const{
  glm::mat4 clip_transform = glm::mat4(1.0f);
  clip_transform[1][1] = clip_transform[2][2] = 0.0f;
  clip_transform[2][1] = -1.0f;
  clip_transform[1][2] = 1.0f;
  switch (type_) {
    case CameraType::kPerspective: {
      return clip_transform*GetPerspectiveTransform(screen_width, screen_height);
      break;
    }
    case CameraType::kOrthographic: {
      return clip_transform *
             GetOrthographicTransform(screen_width, screen_height);
      break;
    }
    default: {
      ASSERT(false, "Unhandled Camera Type!");
      break;
    }
  }
}
glm::mat4 Camera::GetPerspectiveTransform(uint32_t screen_width,
                                          uint32_t screen_height) const {
  glm::mat4 m(0.0f);
  float aspect = static_cast<float>(screen_width) / static_cast<float>(screen_height);
  float tanf = std::tan(glm::radians(fovx_/2.0f));
  float near = 0.1f, far = 10.0f;
  m[0][0] = 1.0f/tanf;
  m[1][1] = -far / (near - far);
  m[3][1] = (near * far) / (near - far);
  m[2][2] = aspect / (tanf);
  m[1][3] = 1.0f;
  return m;
}
glm::mat4 Camera::GetOrthographicTransform(uint32_t screen_width,
                                           uint32_t screen_height) const{
  ASSERT(false, "Not yet implemented!");
}
}  // namespace catalyst
