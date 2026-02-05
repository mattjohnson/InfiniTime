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
    class Ble;

    class PitchCallService {
    public:
      // Service UUID: 00060000-78fc-48fe-8e23-433b3a1942d0
      // Signal Characteristic UUID: 00060001-78fc-48fe-8e23-433b3a1942d0
      // Device ID Characteristic UUID: 00060002-78fc-48fe-8e23-433b3a1942d0

      PitchCallService(Pinetime::System::SystemTask& systemTask, Pinetime::Controllers::Ble& bleController);

      void Init();

      // Callback for when a signal is received
      using SignalCallback = std::function<void(const std::string&)>;
      void SetSignalCallback(SignalCallback callback);

      // Get the last received signal
      const std::string& GetLastSignal() const { return lastSignal; }

      // Check if there's an unread signal
      bool HasUnreadSignal() const { return hasUnreadSignal; }
      void MarkSignalRead() { hasUnreadSignal = false; }

      // Get the short device ID (last 4 hex chars of BLE address)
      const char* GetShortId();

      // BLE GATT callbacks
      int OnSignalWrite(struct ble_gatt_access_ctxt* ctxt);
      int OnDeviceIdRead(struct ble_gatt_access_ctxt* ctxt);

    private:
      Pinetime::System::SystemTask& systemTask;
      Pinetime::Controllers::Ble& bleController;

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

      // 00060002-78fc-48fe-8e23-433b3a1942d0
      static constexpr ble_uuid128_t deviceIdCharUuid {
        .u = {.type = BLE_UUID_TYPE_128},
        .value = {0xd0, 0x42, 0x19, 0x3a, 0x3b, 0x43, 0x23, 0x8e,
                  0xfe, 0x48, 0xfc, 0x78, 0x02, 0x00, 0x06, 0x00}
      };

      struct ble_gatt_chr_def characteristicDefinition[3];
      struct ble_gatt_svc_def serviceDefinition[2];

      std::string lastSignal;
      bool hasUnreadSignal = false;
      SignalCallback signalCallback;

      uint16_t signalHandle;

      char shortId[5] = {0};  // 4 hex chars + null terminator
      bool shortIdInitialized = false;
      void InitShortId();
    };

    // Signal parsing utilities
    struct ParsedSignal {
      enum class Type { Pitch, Play, Connect, Unknown };

      Type type = Type::Unknown;
      std::string pitchCode;
      uint8_t zone = 0;
      int8_t signNumber = -1;  // -1 = no sign, 0-5 = valid sign
      std::string playCode;

      bool hasSign() const { return signNumber >= 0 && signNumber <= 5; }

      // Display text for the watch screen
      std::string GetDisplayText() const;
      std::string GetSubText() const;
    };

    ParsedSignal ParseSignal(const std::string& signal);
  }
}
