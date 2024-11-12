#include "thumbnail.h"

#include <openssl/evp.h>

#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "libyuv.h"
#include "rd_log.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

static std::string test;

void ScaleYUV420pToABGR(char* dst_buffer_, int video_width_, int video_height_,
                        int scaled_video_width_, int scaled_video_height_,
                        char* rgba_buffer_) {
  int src_y_size = video_width_ * video_height_;
  int src_uv_size = (video_width_ + 1) / 2 * (video_height_ + 1) / 2;
  int dst_y_size = scaled_video_width_ * scaled_video_height_;
  int dst_uv_size =
      (scaled_video_width_ + 1) / 2 * (scaled_video_height_ + 1) / 2;

  uint8_t* src_y = reinterpret_cast<uint8_t*>(dst_buffer_);
  uint8_t* src_u = src_y + src_y_size;
  uint8_t* src_v = src_u + src_uv_size;

  std::unique_ptr<uint8_t[]> dst_y(new uint8_t[dst_y_size]);
  std::unique_ptr<uint8_t[]> dst_u(new uint8_t[dst_uv_size]);
  std::unique_ptr<uint8_t[]> dst_v(new uint8_t[dst_uv_size]);

  try {
    libyuv::I420Scale(src_y, video_width_, src_u, (video_width_ + 1) / 2, src_v,
                      (video_width_ + 1) / 2, video_width_, video_height_,
                      dst_y.get(), scaled_video_width_, dst_u.get(),
                      (scaled_video_width_ + 1) / 2, dst_v.get(),
                      (scaled_video_width_ + 1) / 2, scaled_video_width_,
                      scaled_video_height_, libyuv::kFilterBilinear);
  } catch (const std::exception& e) {
    LOG_ERROR("I420Scale failed: %s", e.what());
    return;
  }

  try {
    libyuv::I420ToABGR(
        dst_y.get(), scaled_video_width_, dst_u.get(),
        (scaled_video_width_ + 1) / 2, dst_v.get(),
        (scaled_video_width_ + 1) / 2, reinterpret_cast<uint8_t*>(rgba_buffer_),
        scaled_video_width_ * 4, scaled_video_width_, scaled_video_height_);
  } catch (const std::exception& e) {
    LOG_ERROR("I420ToRGBA failed: %s", e.what());
    return;
  }
}

Thumbnail::Thumbnail() { std::filesystem::create_directory(image_path_); }

Thumbnail::~Thumbnail() {
  if (rgba_buffer_) {
    delete[] rgba_buffer_;
    rgba_buffer_ = nullptr;
  }
}

void remove_colons(const char* input, char* output) {
  int j = 0;
  for (int i = 0; input[i] != '\0'; i++) {
    if (input[i] != ':') {
      output[j++] = input[i];
    }
  }
  output[j] = '\0';  // 添加字符串结束符
}

void restore_colons(const char* input, char* output) {
  int input_len = strlen(input);
  int j = 0;

  for (int i = 0; i < input_len; i++) {
    if (i > 0 && i % 2 == 0) {
      output[j++] = ':';
    }
    output[j++] = input[i];
  }
  output[j] = '\0';  // 添加字符串结束符
}

int Thumbnail::SaveToThumbnail(const char* yuv420p, int width, int height,
                               const std::string& host_name,
                               const std::string& remote_id,
                               const std::string& password) {
  if (!rgba_buffer_) {
    rgba_buffer_ = new char[thumbnail_width_ * thumbnail_height_ * 4];
  }

  if (yuv420p) {
    ScaleYUV420pToABGR((char*)yuv420p, width, height, thumbnail_width_,
                       thumbnail_height_, rgba_buffer_);

    std::string image_name = password + "@" + host_name + "@" + remote_id;
    LOG_ERROR("1 Save thumbnail: {}", image_name);
    int ciphertext_len = AES_encrypt((unsigned char*)image_name.c_str(),
                                     image_name.size(), key_, iv_, ciphertext_);

    int decryptedtext_len =
        AES_decrypt(ciphertext_, ciphertext_len, key_, iv_, decryptedtext_);
    decryptedtext_[decryptedtext_len] = '\0';

    LOG_ERROR("ciphertext [{}]", (char*)ciphertext_);
    LOG_ERROR("decryptedtext [{}]", (char*)decryptedtext_);
    char* hex_str = OPENSSL_buf2hexstr(ciphertext_, ciphertext_len);
    LOG_ERROR("hex [{}]", hex_str);

    char* hex_str_rm = new char[256];
    remove_colons(hex_str, hex_str_rm);
    LOG_ERROR("hex_str_rm [{}]", hex_str_rm);

    char* hex_str_re = new char[256];
    restore_colons(hex_str_rm, hex_str_re);
    LOG_ERROR("hex_str_re [{}]", hex_str_re);

    long text_buf_len = 0;
    unsigned char* text_buf = OPENSSL_hexstr2buf(hex_str, &text_buf_len);
    text_buf[text_buf_len] = '\0';
    LOG_ERROR("text_buf [{}]", (char*)text_buf);

    std::string save_path = image_path_ + hex_str;
    LOG_ERROR("Save thumbnail: {}", save_path);
    stbi_write_png(save_path.data(), thumbnail_width_, thumbnail_height_, 4,
                   rgba_buffer_, thumbnail_width_ * 4);
  }
  return 0;
}

bool LoadTextureFromMemory(const void* data, size_t data_size,
                           SDL_Renderer* renderer, SDL_Texture** out_texture,
                           int* out_width, int* out_height) {
  int image_width = 0;
  int image_height = 0;
  int channels = 4;
  unsigned char* image_data =
      stbi_load_from_memory((const unsigned char*)data, (int)data_size,
                            &image_width, &image_height, NULL, 4);
  if (image_data == nullptr) {
    fprintf(stderr, "Failed to load image: %s\n", stbi_failure_reason());
    return false;
  }

  // ABGR
  SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(
      (void*)image_data, image_width, image_height, channels * 8,
      channels * image_width, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000);
  if (surface == nullptr) {
    fprintf(stderr, "Failed to create SDL surface: %s\n", SDL_GetError());
    return false;
  }

  SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
  if (texture == nullptr)
    fprintf(stderr, "Failed to create SDL texture: %s\n", SDL_GetError());

  *out_texture = texture;
  *out_width = image_width;
  *out_height = image_height;

  SDL_FreeSurface(surface);
  stbi_image_free(image_data);

  return true;
}

bool LoadTextureFromFile(const char* file_name, SDL_Renderer* renderer,
                         SDL_Texture** out_texture, int* out_width,
                         int* out_height) {
  std::filesystem::path file_path(file_name);
  if (!std::filesystem::exists(file_path)) return false;
  std::ifstream file(file_path, std::ios::binary);
  if (!file) return false;
  file.seekg(0, std::ios::end);
  size_t file_size = file.tellg();
  file.seekg(0, std::ios::beg);
  if (file_size == -1) return false;
  char* file_data = new char[file_size];
  if (!file_data) return false;
  file.read(file_data, file_size);
  bool ret = LoadTextureFromMemory(file_data, file_size, renderer, out_texture,
                                   out_width, out_height);
  delete[] file_data;

  return ret;
}

std::vector<std::filesystem::path> Thumbnail::FindThumbnailPath(
    const std::filesystem::path& directory) {
  std::vector<std::filesystem::path> thumbnails_path;
  // std::string image_extensions = ".png";

  if (!std::filesystem::is_directory(directory)) {
    LOG_ERROR("No such directory [{}]", directory.string());
    return thumbnails_path;
  }

  thumbnails_sorted_by_write_time_.clear();

  for (const auto& entry : std::filesystem::directory_iterator(directory)) {
    if (entry.is_regular_file()) {
      std::time_t last_write_time = std::chrono::system_clock::to_time_t(
          time_point_cast<std::chrono::system_clock::duration>(
              entry.last_write_time() -
              std::filesystem::file_time_type::clock::now() +
              std::chrono::system_clock::now()));

      // if (entry.path().extension() == image_extensions) {
      thumbnails_sorted_by_write_time_[last_write_time] = entry.path();
      // }
    }
  }

  for (auto it = thumbnails_sorted_by_write_time_.rbegin();
       it != thumbnails_sorted_by_write_time_.rend(); ++it) {
    thumbnails_path.push_back(it->second);
  }

  return thumbnails_path;
}

int Thumbnail::LoadThumbnail(SDL_Renderer* renderer,
                             std::map<std::string, SDL_Texture*>& textures,
                             int* width, int* height) {
  // textures.clear();
  // std::vector<std::filesystem::path> image_paths =
  //     FindThumbnailPath(image_path_);

  // if (image_paths.size() == 0) {
  //   return -1;
  // } else {
  //   for (int i = 0; i < image_paths.size(); i++) {
  //     size_t pos1 = image_paths[i].string().find('/') + 1;
  //     std::string cipher_image_name = image_paths[i].string().substr(pos1);
  //     LOG_ERROR("cipher_image_name: {}", cipher_image_name);
  //     std::string original_image_name = AES_decrypt(key_, cipher_image_name);
  //     std::string image_path = image_path_ + original_image_name;
  //     LOG_ERROR("image_path: {}", image_path);
  //     // size_t pos1 = original_image_name[i].string().find('@') + 1;
  //     // size_t pos2 = original_image_name[i].string().rfind('@');
  //     // std::string password = original_image_name[i].string().substr(0,
  //     pos1);
  //     // std::string host_name =
  //     //     original_image_name[i].string().substr(pos1, pos2 - pos1);
  //     // std::string remote_id = original_image_name[i].string().substr(pos2
  //     +
  //     // 1);

  //     textures[original_image_name] = nullptr;
  //     LoadTextureFromFile(image_path.c_str(), renderer,
  //                         &(textures[original_image_name]), width, height);
  //   }
  //   return 0;
  // }
  return 0;
}

int Thumbnail::DeleteThumbnail(const std::string& file_path) {
  if (std::filesystem::exists(file_path)) {
    std::filesystem::remove(file_path);
    LOG_INFO("File [{}] removed successfully", file_path);
    return 0;
  } else {
    LOG_ERROR("File [{}] does not exist", file_path);
    return -1;
  }
}

int Thumbnail::AES_encrypt(unsigned char* plaintext, int plaintext_len,
                           unsigned char* key, unsigned char* iv,
                           unsigned char* ciphertext_) {
  EVP_CIPHER_CTX* ctx;
  int len;
  int ciphertext_len;
  if (!(ctx = EVP_CIPHER_CTX_new())) {
    return -1;
  }

  if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv)) {
    return -1;
  }

  if (1 !=
      EVP_EncryptUpdate(ctx, ciphertext_, &len, plaintext, plaintext_len)) {
    return -1;
  }

  ciphertext_len = len;
  if (1 != EVP_EncryptFinal_ex(ctx, ciphertext_ + len, &len)) {
    return -1;
  }

  ciphertext_len += len;
  EVP_CIPHER_CTX_free(ctx);
  return ciphertext_len;
}

int Thumbnail::AES_decrypt(unsigned char* ciphertext_, int ciphertext_len,
                           unsigned char* key, unsigned char* iv,
                           unsigned char* plaintext) {
  EVP_CIPHER_CTX* ctx;
  int len;
  int plaintext_len;
  if (!(ctx = EVP_CIPHER_CTX_new())) {
    return -1;
  }

  if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv)) {
    return -1;
  }

  if (1 !=
      EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext_, ciphertext_len)) {
    return -1;
  }

  plaintext_len = len;
  if (1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len)) {
    return -1;
  }

  plaintext_len += len;
  EVP_CIPHER_CTX_free(ctx);

  return plaintext_len;
}