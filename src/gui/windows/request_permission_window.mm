#include "layout.h"
#include "localization.h"
#include "rd_log.h"
#include "render.h"

#ifdef __APPLE__
#include <ApplicationServices/ApplicationServices.h>
#include <CoreGraphics/CoreGraphics.h>
#import <Foundation/Foundation.h>
#include <unistd.h>
#include <cstdlib>
#endif

namespace crossdesk {

#ifdef __APPLE__
bool Render::CheckScreenRecordingPermission() {
  // CGPreflightScreenCaptureAccess is available on macOS 10.15+
  if (@available(macOS 10.15, *)) {
    bool granted = CGPreflightScreenCaptureAccess();
    LOG_INFO("CGPreflightScreenCaptureAccess returned: {}", granted);
    return granted;
  }
  // For older macOS versions, assume permission is granted
  return true;
}

bool Render::CheckAccessibilityPermission() {
  // Check if the process is trusted for accessibility
  // Note: This may require app restart to reflect permission changes
  NSDictionary* options = @{(__bridge id)kAXTrustedCheckOptionPrompt : @NO};
  bool trusted = AXIsProcessTrustedWithOptions((__bridge CFDictionaryRef)options);
  LOG_INFO("AXIsProcessTrustedWithOptions returned: {}", trusted);
  return trusted;
}

void Render::OpenSystemPreferences() {
  // Open System Preferences to the Privacy & Security > Screen Recording or
  // Accessibility section
  system("open "
         "\"x-apple.systempreferences:com.apple.preference.security?Privacy_"
         "ScreenCapture\"");
}

void Render::OpenScreenRecordingPreferences() {
  // Request screen recording permission first to ensure app appears in System Settings
  if (@available(macOS 10.15, *)) {
    CGRequestScreenCaptureAccess();
  }
  // Open System Preferences to the Privacy & Security > Screen Recording section
  system("open "
         "\"x-apple.systempreferences:com.apple.preference.security?Privacy_"
         "ScreenCapture\"");
}

void Render::OpenAccessibilityPreferences() {
  // Request accessibility permission first to ensure app appears in System Settings
  NSDictionary* options = @{(__bridge id)kAXTrustedCheckOptionPrompt : @YES};
  AXIsProcessTrustedWithOptions((__bridge CFDictionaryRef)options);
  // Open System Preferences to the Privacy & Security > Accessibility section
  system("open "
         "\"x-apple.systempreferences:com.apple.preference.security?Privacy_"
         "Accessibility\"");
}
#endif

int Render::RequestPermissionWindow() {
#ifdef __APPLE__
  // Check permissions - recheck every frame to update status immediately after user grants
  // permission
  bool screen_recording_granted = CheckScreenRecordingPermission();
  bool accessibility_granted = CheckAccessibilityPermission();

  // Update show_request_permission_window_ based on permission status
  // Keep window visible if any permission is not granted
  show_request_permission_window_ = !screen_recording_granted || !accessibility_granted;

  // Log permission status for debugging
  LOG_INFO("Screen recording permission: {}, Accessibility permission: {}",
           screen_recording_granted, accessibility_granted);

  if (!show_request_permission_window_) {
    LOG_INFO("Request permission window is not shown");
    return 0;
  }
  LOG_INFO("Request permission window is shown");

  const ImGuiViewport* viewport = ImGui::GetMainViewport();
  float window_width = localization_language_index_ == 0 ? REQUEST_PERMISSION_WINDOW_WIDTH_CN
                                                         : REQUEST_PERMISSION_WINDOW_WIDTH_EN;
  float window_height = localization_language_index_ == 0 ? REQUEST_PERMISSION_WINDOW_HEIGHT_CN
                                                          : REQUEST_PERMISSION_WINDOW_HEIGHT_EN;

  // Center the window on screen
  ImVec2 center_pos = ImVec2((viewport->WorkSize.x - window_width) * 0.5f + viewport->WorkPos.x,
                             (viewport->WorkSize.y - window_height) * 0.5f + viewport->WorkPos.y);
  ImGui::SetNextWindowPos(center_pos, ImGuiCond_Always);

  ImGui::SetNextWindowSize(ImVec2(window_width, window_height), ImGuiCond_Always);

  // Make window always on top and modal-like
  ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 5.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20.0f, 15.0f));

  ImGui::Begin(localization::request_permissions[localization_language_index_].c_str(), nullptr,
               ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove |
                   ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_Modal);

  ImGui::SetWindowFontScale(0.3f);

  // use system Chinese font
  if (system_chinese_font_ != nullptr) {
    ImGui::PushFont(system_chinese_font_);
  }

  // Message
  ImGui::SetCursorPosX(10.0f);
  ImGui::TextWrapped(
      "%s", localization::permission_required_message[localization_language_index_].c_str());

  ImGui::Spacing();
  ImGui::Spacing();
  ImGui::Spacing();

  // Accessibility Permission
  ImGui::SetCursorPosX(10.0f);
  ImGui::AlignTextToFramePadding();
  ImGui::Text("1. %s:",
              localization::accessibility_permission[localization_language_index_].c_str());
  ImGui::SameLine();
  ImGui::AlignTextToFramePadding();
  if (accessibility_granted) {
    ImGui::Text("%s", localization::permission_granted[localization_language_index_].c_str());
  } else {
    ImGui::Text("%s", localization::permission_denied[localization_language_index_].c_str());
    ImGui::SameLine();
    if (ImGui::Button(
            localization::open_keyboard_mouse_settings[localization_language_index_].c_str())) {
      OpenAccessibilityPreferences();
    }
  }

  ImGui::Spacing();

  // Screen Recording Permission
  ImGui::SetCursorPosX(10.0f);
  ImGui::AlignTextToFramePadding();
  ImGui::Text("2. %s:",
              localization::screen_recording_permission[localization_language_index_].c_str());
  ImGui::SameLine();
  ImGui::AlignTextToFramePadding();
  if (screen_recording_granted) {
    ImGui::Text("%s", localization::permission_granted[localization_language_index_].c_str());
  } else {
    ImGui::Text("%s", localization::permission_denied[localization_language_index_].c_str());
    ImGui::SameLine();
    if (ImGui::Button(
            localization::open_screen_recording_settings[localization_language_index_].c_str())) {
      OpenScreenRecordingPreferences();
    }
  }

  ImGui::SetWindowFontScale(1.0f);
  ImGui::SetWindowFontScale(0.45f);

  // pop system font
  if (system_chinese_font_ != nullptr) {
    ImGui::PopFont();
  }

  ImGui::End();
  ImGui::SetWindowFontScale(1.0f);
  ImGui::PopStyleVar(4);
  ImGui::PopStyleColor();

  return 0;
#else
  return 0;
#endif
}
}  // namespace crossdesk