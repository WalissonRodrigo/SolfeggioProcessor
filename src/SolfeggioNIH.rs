use nih_plug::prelude::*;
use std::sync::Arc;

const SOLFEGGIO_FREQS: [f32; 10] = [
    174.0, 285.0, 396.0, 417.0, 432.0, 528.0, 639.0, 741.0, 852.0, 963.0,
];

const FREQ_NAMES: [&str; 10] = [
    "174Hz", "285Hz", "396Hz", "417Hz", "432Hz", "528Hz", "639Hz", "741Hz", "852Hz", "963Hz",
];

struct SolfeggioPlugin {
    params: Arc<SolfeggioParams>,
    oscillators: Vec<SineOscillator>,
    sample_rate: f32,
}

impl Default for SolfeggioPlugin {
    fn default() -> Self {
        Self {
            params: Arc::new(SolfeggioParams::default()),
            oscillators: Vec::new(),
            sample_rate: 44100.0,
        }
    }
}

#[derive(Params)]
struct SolfeggioParams {
    #[id = "mix"]
    pub mix: FloatParam,

    #[id = "174hz"]
    pub freq_0: FloatParam,
    #[id = "285hz"]
    pub freq_1: FloatParam,
    #[id = "396hz"]
    pub freq_2: FloatParam,
    #[id = "417hz"]
    pub freq_3: FloatParam,
    #[id = "432hz"]
    pub freq_4: FloatParam,
    #[id = "528hz"]
    pub freq_5: FloatParam,
    #[id = "639hz"]
    pub freq_6: FloatParam,
    #[id = "741hz"]
    pub freq_7: FloatParam,
    #[id = "852hz"]
    pub freq_8: FloatParam,
    #[id = "963hz"]
    pub freq_9: FloatParam,
}

impl SolfeggioParams {
    fn freq_gain(&self, index: usize) -> f32 {
        match index {
            0 => self.freq_0.value(),
            1 => self.freq_1.value(),
            2 => self.freq_2.value(),
            3 => self.freq_3.value(),
            4 => self.freq_4.value(),
            5 => self.freq_5.value(),
            6 => self.freq_6.value(),
            7 => self.freq_7.value(),
            8 => self.freq_8.value(),
            9 => self.freq_9.value(),
            _ => 0.0,
        }
    }
}

impl Default for SolfeggioParams {
    fn default() -> Self {
        let make_param = |name: &str| {
            FloatParam::new(name, 0.0, FloatRange::Linear { min: 0.0, max: 1.0 })
                .with_unit(" dB")
                .with_smoother(SmoothingStyle::Linear(10.0))
        };

        Self {
            mix: FloatParam::new("Mix", 0.15, FloatRange::Linear { min: 0.0, max: 1.0 })
                .with_smoother(SmoothingStyle::Linear(10.0)),
            freq_0: make_param(FREQ_NAMES[0]),
            freq_1: make_param(FREQ_NAMES[1]),
            freq_2: make_param(FREQ_NAMES[2]),
            freq_3: make_param(FREQ_NAMES[3]),
            freq_4: make_param(FREQ_NAMES[4]),
            freq_5: make_param(FREQ_NAMES[5]),
            freq_6: make_param(FREQ_NAMES[6]),
            freq_7: make_param(FREQ_NAMES[7]),
            freq_8: make_param(FREQ_NAMES[8]),
            freq_9: make_param(FREQ_NAMES[9]),
        }
    }
}

impl Plugin for SolfeggioPlugin {
    const NAME: &'static str = "Solfeggio Frequencies";
    const VENDOR: &'static str = "Walisson Rodrigo";
    const URL: &'static str = "https://github.com/WalissonRodrigo";
    const EMAIL: &'static str = "walissonrodrigo@outlook.com";
    const VERSION: &'static str = env!("CARGO_PKG_VERSION");

    const AUDIO_IO_LAYOUTS: &'static [AudioIOLayout] = &[AudioIOLayout {
        main_input_channels: NonZeroU32::new(2),
        main_output_channels: NonZeroU32::new(2),
        ..AudioIOLayout::const_default()
    }];

    type SysExMessage = ();
    type BackgroundTask = ();

    fn params(&self) -> Arc<dyn Params> {
        self.params.clone()
    }

    fn initialize(
        &mut self,
        _audio_io_layout: &AudioIOLayout,
        buffer_config: &BufferConfig,
        _context: &mut impl InitContext<Self>,
    ) -> bool {
        self.sample_rate = buffer_config.sample_rate;
        self.oscillators = SOLFEGGIO_FREQS
            .iter()
            .map(|&freq| SineOscillator::new(freq, buffer_config.sample_rate))
            .collect();
        true
    }

    fn process(
        &mut self,
        buffer: &mut Buffer,
        _aux: &mut AuxiliaryBuffers,
        _context: &mut impl ProcessContext<Self>,
    ) -> ProcessStatus {
        let mix = self.params.mix.smoothed.next();

        for mut channel_samples in buffer.iter_samples() {
            let mut solfeggio_sum = 0.0;

            for (i, osc) in self.oscillators.iter_mut().enumerate() {
                let gain = self.params.freq_gain(i);
                if gain > 0.001 {
                    solfeggio_sum += osc.next() * gain * 0.1; // -20dB subliminal
                }
            }

            for sample in channel_samples.iter_mut() {
                // Sidechain: reduce solfeggio when music is loud
                let sidechain = 1.0 - (sample.abs() * 0.5).min(0.7);
                *sample = *sample * (1.0 - mix * 0.3) + solfeggio_sum * mix * sidechain;
            }
        }

        ProcessStatus::Normal
    }
}

impl ClapPlugin for SolfeggioPlugin {
    const CLAP_ID: &'static str = "com.walissonrodrigo.solfeggio";
    const CLAP_DESCRIPTION: Option<&'static str> =
        Some("Applies subliminal Solfeggio frequencies to audio");
    const CLAP_MANUAL_URL: Option<&'static str> = None;
    const CLAP_SUPPORT_URL: Option<&'static str> = None;
    const CLAP_FEATURES: &'static [ClapFeature] = &[ClapFeature::AudioEffect, ClapFeature::Utility];
}

impl Vst3Plugin for SolfeggioPlugin {
    const VST3_CLASS_ID: [u8; 16] = *b"SolfeggioFreqWR!";
    const VST3_SUBCATEGORIES: &'static [Vst3SubCategory] =
        &[Vst3SubCategory::Fx, Vst3SubCategory::Tools];
}

// --- Sine Oscillator ---

struct SineOscillator {
    phase: f32,
    phase_increment: f32,
}

impl SineOscillator {
    fn new(freq: f32, sample_rate: f32) -> Self {
        Self {
            phase: 0.0,
            phase_increment: freq * 2.0 * std::f32::consts::PI / sample_rate,
        }
    }

    fn next(&mut self) -> f32 {
        let sample = self.phase.sin();
        self.phase += self.phase_increment;
        if self.phase > 2.0 * std::f32::consts::PI {
            self.phase -= 2.0 * std::f32::consts::PI;
        }
        sample
    }
}

nih_export_clap!(SolfeggioPlugin);
nih_export_vst3!(SolfeggioPlugin);
