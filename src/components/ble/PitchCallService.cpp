#include "PitchCallService.h"
#include <cstring>
#include <algorithm>
#include "systemtask/SystemTask.h"

using namespace Pinetime::Controllers;

constexpr ble_uuid128_t PitchCallService::serviceUuid;
constexpr ble_uuid128_t PitchCallService::signalCharUuid;

namespace {
  // Static instance pointer for C callback
  PitchCallService* instance = nullptr;

  int SignalWriteCallback(uint16_t /*conn_handle*/, uint16_t /*attr_handle*/,
                          struct ble_gatt_access_ctxt* ctxt, void* /*arg*/) {
    if (instance != nullptr) {
      return instance->OnSignalWrite(ctxt);
    }
    return BLE_ATT_ERR_UNLIKELY;
  }
}

PitchCallService::PitchCallService(Pinetime::System::SystemTask& systemTask)
  : systemTask {systemTask} {
  instance = this;

  // Define signal characteristic (write without response)
  characteristicDefinition[0] = {
    .uuid = reinterpret_cast<const ble_uuid_t*>(&signalCharUuid),
    .access_cb = SignalWriteCallback,
    .arg = this,
    .flags = BLE_GATT_CHR_F_WRITE_NO_RSP | BLE_GATT_CHR_F_WRITE,
    .val_handle = &signalHandle
  };
  characteristicDefinition[1] = {0}; // Terminator
  
  // Define service
  serviceDefinition[0] = {
    .type = BLE_GATT_SVC_TYPE_PRIMARY,
    .uuid = reinterpret_cast<const ble_uuid_t*>(&serviceUuid),
    .characteristics = characteristicDefinition
  };
  serviceDefinition[1] = {0}; // Terminator
}

void PitchCallService::Init() {
  int res = ble_gatts_count_cfg(serviceDefinition);
  if (res != 0) {
    return;
  }
  
  res = ble_gatts_add_svcs(serviceDefinition);
  if (res != 0) {
    return;
  }
}

void PitchCallService::SetSignalCallback(SignalCallback callback) {
  signalCallback = callback;
}

int PitchCallService::OnSignalWrite(struct ble_gatt_access_ctxt* ctxt) {
  if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
    // Read the incoming data
    uint16_t len = ctxt->om->om_len;
    if (len > 0 && len <= 32) {
      char buffer[33] = {0};
      memcpy(buffer, ctxt->om->om_data, len);

      lastSignal = std::string(buffer, len);
      hasUnreadSignal = true;

      // Invoke callback if set
      if (signalCallback) {
        signalCallback(lastSignal);
      }

      // Notify SystemTask to wake up and display PitchReceiver
      systemTask.PushMessage(Pinetime::System::Messages::OnPitchCall);
    }
    return 0;
  }
  return BLE_ATT_ERR_UNLIKELY;
}

// Signal parsing implementation
ParsedSignal Pinetime::Controllers::ParseSignal(const std::string& signal) {
  ParsedSignal result;
  
  // Find first delimiter
  size_t pos1 = signal.find('|');
  if (pos1 == std::string::npos) {
    return result; // Invalid format
  }
  
  std::string type = signal.substr(0, pos1);
  
  if (type == "PITCH") {
    result.type = ParsedSignal::Type::Pitch;

    // Find second delimiter (after pitch code)
    size_t pos2 = signal.find('|', pos1 + 1);
    if (pos2 != std::string::npos) {
      result.pitchCode = signal.substr(pos1 + 1, pos2 - pos1 - 1);

      // Find third delimiter (after zone, before optional sign)
      size_t pos3 = signal.find('|', pos2 + 1);

      // Parse zone number
      std::string zoneStr;
      if (pos3 != std::string::npos) {
        zoneStr = signal.substr(pos2 + 1, pos3 - pos2 - 1);
      } else {
        zoneStr = signal.substr(pos2 + 1);
      }

      if (!zoneStr.empty()) {
        int zoneVal = 0;
        for (char c : zoneStr) {
          if (c >= '0' && c <= '9') {
            zoneVal = zoneVal * 10 + (c - '0');
          } else {
            break;
          }
        }
        if (zoneVal >= 1 && zoneVal <= 13) {
          result.zone = static_cast<uint8_t>(zoneVal);
        }
      }

      // Parse optional sign number (0-5)
      if (pos3 != std::string::npos && pos3 + 1 < signal.length()) {
        char signChar = signal[pos3 + 1];
        if (signChar >= '0' && signChar <= '5') {
          result.signNumber = static_cast<int8_t>(signChar - '0');
        }
      }
    }
  } else if (type == "PLAY") {
    result.type = ParsedSignal::Type::Play;
    result.playCode = signal.substr(pos1 + 1);
  }
  
  return result;
}

std::string ParsedSignal::GetDisplayText() const {
  switch (type) {
    case Type::Pitch:
      if (hasSign()) {
        return pitchCode + " " + std::to_string(signNumber);
      }
      return pitchCode;
    case Type::Play:
      return playCode;
    default:
      return "???";
  }
}

std::string ParsedSignal::GetSubText() const {
  switch (type) {
    case Type::Pitch: {
      // Handle ball zones (outside strike zone)
      if (zone == 10) return "HIGH BALL";
      if (zone == 11) return "LOW BALL";
      if (zone == 12) return "INSIDE BALL";
      if (zone == 13) return "OUTSIDE BALL";

      // Strike zone positions (1-9)
      const char* vert = "";
      const char* horiz = "";

      switch ((zone - 1) / 3) {
        case 0: vert = "HIGH"; break;
        case 1: vert = "MID"; break;
        case 2: vert = "LOW"; break;
      }

      switch ((zone - 1) % 3) {
        case 0: horiz = "IN"; break;
        case 1: horiz = "MID"; break;
        case 2: horiz = "OUT"; break;
      }

      return std::string(vert) + " " + horiz;
    }
    case Type::Play:
      return "Play";
    default:
      return "";
  }
}
