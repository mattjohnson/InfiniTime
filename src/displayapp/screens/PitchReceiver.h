#pragma once

#include "displayapp/screens/Screen.h"
#include "displayapp/apps/Apps.h"
#include "displayapp/Controllers.h"
#include "displayapp/screens/Symbols.h"
#include "components/motor/MotorController.h"
#include "components/ble/PitchCallService.h"
#include "components/ble/NimbleController.h"
#include "systemtask/SystemTask.h"
#include <lvgl/lvgl.h>

namespace Pinetime {
  namespace Applications {
    namespace Screens {

      class PitchReceiver : public Screen {
      public:
        PitchReceiver(Controllers::PitchCallService& pitchCallService,
                      Controllers::MotorController& motorController);
        ~PitchReceiver() override;

        void Refresh() override;
        bool OnTouchEvent(TouchEvents event) override;
        bool OnButtonPushed() override;

      private:
        Controllers::PitchCallService& pitchCallService;
        Controllers::MotorController& motorController;

        // UI elements
        lv_obj_t* mainLabel;      // Large text showing pitch/play
        lv_obj_t* subLabel;       // Smaller text with zone info
        lv_obj_t* waitingLabel;   // "Waiting for signal..." text
        lv_obj_t* timerArc;       // Countdown arc indicator
        lv_obj_t* zoneGrid;       // Visual zone grid
        lv_obj_t* zoneHighlight;  // Highlighted zone
        lv_obj_t* idLabel;        // Device short ID in top-right corner

        lv_task_t* refreshTask;

        // State
        bool showingSignal = false;
        Controllers::ParsedSignal currentSignal;
        uint32_t signalTimestamp = 0;
        static constexpr uint32_t DISMISS_TIMEOUT_MS = 15000;

        void ShowWaitingState();
        void ShowSignal(const Controllers::ParsedSignal& signal);
        void DismissSignal();
        void UpdateTimerArc();
        void CreateZoneGrid();
        void HighlightZone(uint8_t zone);
        void Vibrate();

        static void RefreshTaskCallback(lv_task_t* task);
      };

    }

    template <>
    struct AppTraits<Apps::PitchReceiver> {
      static constexpr Apps app = Apps::PitchReceiver;
      static constexpr const char* icon = Screens::Symbols::eye;

      static Screens::Screen* Create(AppControllers& controllers) {
        return new Screens::PitchReceiver(controllers.systemTask->nimble().GetPitchCallService(), controllers.motorController);
      };

      static bool IsAvailable(Pinetime::Controllers::FS& /*filesystem*/) {
        return true;
      };
    };
  }
}
