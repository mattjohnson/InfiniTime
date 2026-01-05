#pragma once

#include <cstdint>
#include <string>
#include <functional>

#define NimBLELog ""
#undef min
#undef max

#include <host/ble_gatt.h>

namespace Pinetime {
  namespace System {
    class SystemTask;
  }

  namespace Controllers {

    class PitchCallService {
    public:
      // Service UUID: 00060000-78fc-48fe-8e23-433b3a1942d0
      // Signal Characteristic UUID: 00060001-78fc-48fe-8e23-433b3a1942d0

      PitchCallService(Pinetime::System::SystemTask& systemTask);

      void Init();
      
      // Callback for when a signal is received
      using SignalCallback = std::function<void(const std::string&)>;
      void SetSignalCallback(SignalCallback callback);
      
      // Get the last received signal
      const std::string& GetLastSignal() const { return lastSignal; }
      
      // Check if there's an unread signal
      bool HasUnreadSignal() const { return hasUnreadSignal; }
      void MarkSignalRead() { hasUnreadSignal = false; }
      
      // BLE GATT callback
      int OnSignalWrite(struct ble_gatt_access_ctxt* ctxt);
      
    private:
      Pinetime::System::SystemTask& systemTask;

      // 00060000-78fc-48fe-8e23-433b3a1942d0
      static constexpr ble_uuid128_t serviceUuid {
        .u = {.type = BLE_UUID_TYPE_128},
        .value = {0xd0, 0x42, 0x19, 0x3a, 0x3b, 0x43, 0x23, 0x8e,
                  0xfe, 0x48, 0xfc, 0x78, 0x00, 0x00, 0x06, 0x00}
      };

      // 00060001-78fc-48fe-8e23-433b3a1942d0
      static constexpr ble_uuid128_t signalCharUuid {
        .u = {.type = BLE_UUID_TYPE_128},
        .value = {0xd0, 0x42, 0x19, 0x3a, 0x3b, 0x43, 0x23, 0x8e,
                  0xfe, 0x48, 0xfc, 0x78, 0x01, 0x00, 0x06, 0x00}
      };

      struct ble_gatt_chr_def characteristicDefinition[2];
      struct ble_gatt_svc_def serviceDefinition[2];
      
      std::string lastSignal;
      bool hasUnreadSignal = false;
      SignalCallback signalCallback;
      
      uint16_t signalHandle;
    };
    
    // Signal parsing utilities
    struct ParsedSignal {
      enum class Type { Pitch, Play, Unknown };
      
      Type type = Type::Unknown;
      std::string pitchCode;
      uint8_t zone = 0;
      std::string playCode;
      
      // Display text for the watch screen
      std::string GetDisplayText() const;
      std::string GetSubText() const;
    };
    
    ParsedSignal ParseSignal(const std::string& signal);
  }
}
