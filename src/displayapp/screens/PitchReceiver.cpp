#include "PitchReceiver.h"
#include "components/motor/MotorController.h"

using namespace Pinetime::Applications::Screens;

namespace {
  constexpr lv_color_t COLOR_BG = LV_COLOR_MAKE(0x00, 0x00, 0x00);
  constexpr lv_color_t COLOR_FASTBALL = LV_COLOR_MAKE(0xFF, 0x44, 0x44);
  constexpr lv_color_t COLOR_BREAKING = LV_COLOR_MAKE(0x44, 0x88, 0xFF);
  constexpr lv_color_t COLOR_OFFSPEED = LV_COLOR_MAKE(0x44, 0xDD, 0x44);
  constexpr lv_color_t COLOR_PLAY = LV_COLOR_MAKE(0xFF, 0xAA, 0x00);
  constexpr lv_color_t COLOR_WHITE = LV_COLOR_MAKE(0xFF, 0xFF, 0xFF);
  constexpr lv_color_t COLOR_GRAY = LV_COLOR_MAKE(0x88, 0x88, 0x88);
  constexpr lv_color_t COLOR_DARK = LV_COLOR_MAKE(0x33, 0x33, 0x33);

  lv_color_t GetPitchColor(const std::string& pitchCode) {
    // Fastballs (red)
    if (pitchCode == "FB" || pitchCode == "2S" || pitchCode == "CUT" || pitchCode == "SNK") {
      return COLOR_FASTBALL;
    }
    // Breaking balls (blue)
    if (pitchCode == "CB" || pitchCode == "SL") {
      return COLOR_BREAKING;
    }
    // Offspeed (green)
    if (pitchCode == "CH" || pitchCode == "SPL") {
      return COLOR_OFFSPEED;
    }
    // Knuckleball/screwball - use white or add a new color
    if (pitchCode == "KN" || pitchCode == "SCR") {
      return COLOR_WHITE;
    }
    return COLOR_WHITE;
  }
}

PitchReceiver::PitchReceiver(Controllers::PitchCallService& pitchCallService,
                             Controllers::MotorController& motorController)
    : pitchCallService(pitchCallService),
      motorController(motorController) {

  // Set black background
  lv_obj_set_style_local_bg_color(lv_scr_act(), LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, COLOR_BG);

  // Create waiting label
  waitingLabel = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text(waitingLabel, "Waiting for\nsignal...");
  lv_obj_set_style_local_text_color(waitingLabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, COLOR_GRAY);
  lv_obj_set_style_local_text_font(waitingLabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &jetbrains_mono_bold_20);
  lv_label_set_align(waitingLabel, LV_LABEL_ALIGN_CENTER);
  lv_obj_align(waitingLabel, lv_scr_act(), LV_ALIGN_CENTER, 0, 0);

  // Create main label (hidden initially)
  mainLabel = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text(mainLabel, "");
  lv_obj_set_style_local_text_color(mainLabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, COLOR_WHITE);
  lv_obj_set_style_local_text_font(mainLabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &jetbrains_mono_32);
  lv_label_set_align(mainLabel, LV_LABEL_ALIGN_CENTER);
  lv_obj_align(mainLabel, lv_scr_act(), LV_ALIGN_CENTER, 0, -40);
  lv_obj_set_hidden(mainLabel, true);

  // Create sub label (hidden initially)
  subLabel = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text(subLabel, "");
  lv_obj_set_style_local_text_color(subLabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, COLOR_GRAY);
  lv_obj_set_style_local_text_font(subLabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &jetbrains_mono_bold_20);
  lv_label_set_align(subLabel, LV_LABEL_ALIGN_CENTER);
  lv_obj_align(subLabel, lv_scr_act(), LV_ALIGN_CENTER, 0, 20);
  lv_obj_set_hidden(subLabel, true);

  // Create simple zone grid
  zoneGrid = lv_obj_create(lv_scr_act(), nullptr);
  lv_obj_set_size(zoneGrid, 90, 90);
  lv_obj_align(zoneGrid, lv_scr_act(), LV_ALIGN_IN_BOTTOM_MID, 0, -20);
  lv_obj_set_style_local_bg_color(zoneGrid, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, COLOR_DARK);
  lv_obj_set_style_local_border_width(zoneGrid, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 1);
  lv_obj_set_style_local_border_color(zoneGrid, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, COLOR_GRAY);
  lv_obj_set_hidden(zoneGrid, true);

  // Create zone highlight
  zoneHighlight = lv_obj_create(zoneGrid, nullptr);
  lv_obj_set_size(zoneHighlight, 28, 28);
  lv_obj_set_hidden(zoneHighlight, true);

  // Create timer arc
  timerArc = lv_arc_create(lv_scr_act(), nullptr);
  lv_arc_set_bg_angles(timerArc, 0, 360);
  lv_arc_set_angles(timerArc, 0, 360);
  lv_obj_set_size(timerArc, 230, 230);
  lv_obj_align(timerArc, lv_scr_act(), LV_ALIGN_CENTER, 0, 0);
  lv_arc_set_rotation(timerArc, 270);
  lv_obj_set_style_local_line_width(timerArc, LV_ARC_PART_INDIC, LV_STATE_DEFAULT, 4);
  lv_obj_set_style_local_line_width(timerArc, LV_ARC_PART_BG, LV_STATE_DEFAULT, 4);
  lv_obj_set_style_local_line_color(timerArc, LV_ARC_PART_BG, LV_STATE_DEFAULT, COLOR_DARK);
  lv_obj_set_style_local_line_color(timerArc, LV_ARC_PART_INDIC, LV_STATE_DEFAULT, COLOR_WHITE);
  lv_obj_set_hidden(timerArc, true);

  // Set up refresh task
  refreshTask = lv_task_create(RefreshTaskCallback, 100, LV_TASK_PRIO_MID, this);

  // Just show waiting state - no callbacks or signal checking for now
  showingSignal = false;
}

PitchReceiver::~PitchReceiver() {
  if (refreshTask != nullptr) {
    lv_task_del(refreshTask);
  }
  lv_obj_clean(lv_scr_act());
}

void PitchReceiver::Refresh() {
  if (showingSignal) {
    UpdateTimerArc();
    uint32_t elapsed = xTaskGetTickCount() - signalTimestamp;
    if (elapsed >= pdMS_TO_TICKS(DISMISS_TIMEOUT_MS)) {
      DismissSignal();
    }
  }

  // Check for new signals
  if (pitchCallService.HasUnreadSignal()) {
    // Mark as read immediately to prevent re-processing
    pitchCallService.MarkSignalRead();

    const std::string& signalStr = pitchCallService.GetLastSignal();
    if (!signalStr.empty() && signalStr.length() < 32) {
      const char* data = signalStr.c_str();
      Controllers::ParsedSignal signal;

      // Check if it starts with "PITCH|"
      if (signalStr.length() > 6 &&
          data[0] == 'P' && data[1] == 'I' && data[2] == 'T' &&
          data[3] == 'C' && data[4] == 'H' && data[5] == '|') {
        signal.type = Controllers::ParsedSignal::Type::Pitch;

        // Find pitch code after "PITCH|"
        size_t codeStart = 6;
        size_t codeEnd = signalStr.find('|', codeStart);
        if (codeEnd != std::string::npos && codeEnd > codeStart) {
          signal.pitchCode = "FB";  // Default
          // Copy 2-3 char pitch code
          size_t codeLen = codeEnd - codeStart;
          if (codeLen >= 2 && codeLen <= 3) {
            signal.pitchCode = signalStr.substr(codeStart, codeLen);
          }

          // Parse zone (single digit after second |)
          if (codeEnd + 1 < signalStr.length()) {
            char zoneChar = data[codeEnd + 1];
            if (zoneChar >= '1' && zoneChar <= '9') {
              signal.zone = zoneChar - '0';
            }
          }
        }
        ShowSignal(signal);
      } else if (signalStr.length() > 5 &&
                 data[0] == 'P' && data[1] == 'L' && data[2] == 'A' &&
                 data[3] == 'Y' && data[4] == '|') {
        signal.type = Controllers::ParsedSignal::Type::Play;
        signal.playCode = "PLAY";  // Simple default
        ShowSignal(signal);
      }
    }
  }
}

void PitchReceiver::RefreshTaskCallback(lv_task_t* task) {
  auto* screen = static_cast<PitchReceiver*>(task->user_data);
  screen->Refresh();
}

bool PitchReceiver::OnTouchEvent(TouchEvents event) {
  if (event == TouchEvents::Tap && showingSignal) {
    DismissSignal();
    return true;
  }
  return false;
}

bool PitchReceiver::OnButtonPushed() {
  if (showingSignal) {
    DismissSignal();
    return true;
  }
  return false;
}

void PitchReceiver::ShowWaitingState() {
  showingSignal = false;

  lv_obj_set_hidden(waitingLabel, false);
  lv_obj_set_hidden(mainLabel, true);
  lv_obj_set_hidden(subLabel, true);
  lv_obj_set_hidden(timerArc, true);
  lv_obj_set_hidden(zoneGrid, true);
}

void PitchReceiver::ShowSignal(const Controllers::ParsedSignal& signal) {
  currentSignal = signal;
  showingSignal = true;
  signalTimestamp = xTaskGetTickCount();

  // Vibrate to alert (non-blocking)
  Vibrate();

  lv_obj_set_hidden(waitingLabel, true);

  lv_label_set_text(mainLabel, signal.GetDisplayText().c_str());
  lv_obj_set_hidden(mainLabel, false);

  lv_color_t textColor = COLOR_WHITE;
  if (signal.type == Controllers::ParsedSignal::Type::Pitch) {
    textColor = GetPitchColor(signal.pitchCode);
  } else if (signal.type == Controllers::ParsedSignal::Type::Play) {
    textColor = COLOR_PLAY;
  }
  lv_obj_set_style_local_text_color(mainLabel, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, textColor);
  lv_obj_set_style_local_line_color(timerArc, LV_ARC_PART_INDIC, LV_STATE_DEFAULT, textColor);

  if (signal.type == Controllers::ParsedSignal::Type::Pitch && signal.zone >= 1 && signal.zone <= 9) {
    // For pitches with zones: position label higher to make room for grid
    lv_obj_align(mainLabel, lv_scr_act(), LV_ALIGN_CENTER, 0, -40);
    lv_obj_set_hidden(subLabel, true);
    lv_obj_set_hidden(zoneGrid, false);
    HighlightZone(signal.zone);
  } else {
    // For plays: center the label, show sub text, hide grid
    lv_obj_align(mainLabel, lv_scr_act(), LV_ALIGN_CENTER, 0, -20);
    lv_label_set_text(subLabel, signal.GetSubText().c_str());
    lv_obj_set_hidden(subLabel, false);
    lv_obj_align(subLabel, lv_scr_act(), LV_ALIGN_CENTER, 0, 20);
    lv_obj_set_hidden(zoneGrid, true);
  }

  lv_arc_set_angles(timerArc, 0, 360);
  lv_obj_set_hidden(timerArc, false);
}

void PitchReceiver::DismissSignal() {
  ShowWaitingState();
}

void PitchReceiver::UpdateTimerArc() {
  uint32_t elapsed = xTaskGetTickCount() - signalTimestamp;
  uint32_t totalTicks = pdMS_TO_TICKS(DISMISS_TIMEOUT_MS);

  if (elapsed < totalTicks) {
    float remaining = 1.0f - (static_cast<float>(elapsed) / totalTicks);
    int16_t angle = static_cast<int16_t>(remaining * 360.0f);
    lv_arc_set_angles(timerArc, 0, angle);
  }
}

void PitchReceiver::CreateZoneGrid() {
  // Simplified - grid already created in constructor
}

void PitchReceiver::HighlightZone(uint8_t zone) {
  if (zone < 1 || zone > 9) return;

  int row = (zone - 1) / 3;
  int col = (zone - 1) % 3;

  int x = col * 30 + 1;
  int y = row * 30 + 1;

  lv_obj_set_pos(zoneHighlight, x, y);

  lv_color_t color = GetPitchColor(currentSignal.pitchCode);
  lv_obj_set_style_local_bg_color(zoneHighlight, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, color);

  lv_obj_set_hidden(zoneHighlight, false);
}

void PitchReceiver::Vibrate() {
  // Single short vibration - no vTaskDelay
  motorController.RunForDuration(50);
}
