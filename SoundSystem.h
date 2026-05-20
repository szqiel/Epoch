#ifndef SoundSystemH
#define SoundSystemH

#include <atomic>

namespace SoundSystem {
  extern std::atomic<float> TargetFrequency;
  extern std::atomic<float> TargetVolume;
  extern std::atomic<int> Waveform;
  
  extern std::atomic<float> SfxAge;
  extern std::atomic<int> SfxType;

  void Start();
  void Stop();
}

void PlaySfx(int Type);

#endif
