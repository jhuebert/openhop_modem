#pragma once

namespace StationG3Power {

struct Snapshot {
    bool available;
    bool valid;
    float inputVoltageV;
    float currentMa;
    float powerW;
    float minimumInputVoltageV;
    float maximumCurrentMa;
};

void begin();
void loop();
Snapshot snapshot();

}  // namespace StationG3Power
