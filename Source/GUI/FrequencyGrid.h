#pragma once
#include "FrequencyControl.h"
#include <JuceHeader.h>

class FrequencyGrid : public juce::Component {
public:
  FrequencyGrid(SolfeggioProcessor &p) {
    for (int i = 0; i < Solfeggio::NUM_FREQUENCIES; ++i) {
      controls.add(new FrequencyControl(p, i));
      addAndMakeVisible(controls.getLast());
    }
  }

  void resized() override {
    auto area = getLocalBounds();
    int rows = 2, cols = 5;
    int w = area.getWidth() / cols;
    int h = area.getHeight() / rows;

    for (int i = 0; i < controls.size(); ++i) {
      int r = i / cols;
      int c = i % cols;
      controls[i]->setBounds(c * w, r * h, w, h);
    }
  }

  void updateVisuals(bool autoMode) {
    float alpha = autoMode ? 0.35f : 1.0f;
    for (auto *ctrl : controls) {
      ctrl->setAlphaAndEnabled(alpha, !autoMode);
    }
  }

private:
  juce::OwnedArray<FrequencyControl> controls;
};
