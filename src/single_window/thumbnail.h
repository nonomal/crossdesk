/*
 * @Author: DI JUNKUN
 * @Date: 2024-11-07
 * Copyright (c) 2024 by DI JUNKUN, All Rights Reserved.
 */

#ifndef _THUMBNAIL_H_
#define _THUMBNAIL_H_

#include <SDL.h>

#include <filesystem>
#include <map>

class Thumbnail {
 public:
  Thumbnail();
  ~Thumbnail();

 public:
  int SaveToThumbnail(const char* yuv420p, int width, int height,
                      const std::string& host_name,
                      const std::string& remote_id,
                      const std::string& password);

  int LoadThumbnail(SDL_Renderer* renderer,
                    std::map<std::string, SDL_Texture*>& textures, int* width,
                    int* height);

  int DeleteThumbnail(const std::string& file_path);

 private:
  std::vector<std::filesystem::path> FindThumbnailPath(
      const std::filesystem::path& directory);

  int AES_encrypt(unsigned char* plaintext, int plaintext_len,
                  unsigned char* key, unsigned char* iv,
                  unsigned char* ciphertext);

  int AES_decrypt(unsigned char* ciphertext, int ciphertext_len,
                  unsigned char* key, unsigned char* iv,
                  unsigned char* plaintext);

 private:
  int thumbnail_width_ = 160;
  int thumbnail_height_ = 90;
  char* rgba_buffer_ = nullptr;
  std::string image_path_ = "thumbnails/";
  std::map<std::time_t, std::filesystem::path> thumbnails_sorted_by_write_time_;

  unsigned char* key_ = (unsigned char*)"01234567890123456789012345678901";
  unsigned char* iv_ = (unsigned char*)"01234567890123456";
  unsigned char ciphertext_[64];
  unsigned char decryptedtext_[64];
};

#endif