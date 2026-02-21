// SolfeggioIPlug.h
#pragma once
#include "IPlug/IPlug_include_in_plug_hdr.h"
#include "IPlug/Extras/Synth/Synth.h"

const int kNumPresets = 1;
const int kNumFreqs = 10;

enum EParams {
    kGain = 0,
    kFreqEnabled = kGain + kNumFreqs,
    kNumParams = kFreqEnabled + kNumFreqs
};

class SolfeggioIPlug : public iplug::Plugin {
public:
    SolfeggioIPlug(const iplug::InstanceInfo& info);
    
    void ProcessBlock(iplug::sample** inputs, iplug::sample** outputs, int nFrames) override;
    void OnReset() override;
    
private:
    std::array<iplug::FastSinOscillator<double>, kNumFreqs> mOscs;
    std::array<double, kNumFreqs> mFreqs = {
        174.0, 285.0, 396.0, 417.0, 432.0, 
        528.0, 639.0, 741.0, 852.0, 963.0
    };
};