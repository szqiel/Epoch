#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <cmath>
#include <cstdlib>
#include "SoundSystem.h"
#include "GameConfig.h"
#include "GameGlobals.h"

namespace SoundSystem {
  HWAVEOUT hWaveOut = NULL;
  HANDLE hAudioThread = NULL;
  std::atomic<bool> ThreadRunning(false);
  
  std::atomic<float> TargetFrequency(220.0f);
  std::atomic<float> TargetVolume(0.12f);
  std::atomic<int> Waveform(0);
  
  std::atomic<float> SfxAge(-1.0f);
  std::atomic<int> SfxType(0);
  
  const int SampleRate = 22050;
  const int BufferSize = 2048;
  short BufferA[BufferSize];
  short BufferB[BufferSize];
  WAVEHDR WaveHdrA;
  WAVEHDR WaveHdrB;
  
  void AudioLoop() {
    float Phase = 0.0f;
    float CurrentFreq = 220.0f;
    float CurrentVol = 0.05f;
    float WindFilterState = 0.0f;
    
    WAVEFORMATEX Format;
    Format.wFormatTag = WAVE_FORMAT_PCM;
    Format.nChannels = 1;
    Format.nSamplesPerSec = SampleRate;
    Format.nAvgBytesPerSec = SampleRate * 2;
    Format.nBlockAlign = 2;
    Format.wBitsPerSample = 16;
    Format.cbSize = 0;
    
    if (waveOutOpen(&hWaveOut, WAVE_MAPPER, &Format, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR) {
      return;
    }
    
    memset(&WaveHdrA, 0, sizeof(WAVEHDR));
    memset(&WaveHdrB, 0, sizeof(WAVEHDR));
    
    WaveHdrA.lpData = (LPSTR)BufferA;
    WaveHdrA.dwBufferLength = BufferSize * 2;
    WaveHdrB.lpData = (LPSTR)BufferB;
    WaveHdrB.dwBufferLength = BufferSize * 2;
    
    waveOutPrepareHeader(hWaveOut, &WaveHdrA, sizeof(WAVEHDR));
    waveOutPrepareHeader(hWaveOut, &WaveHdrB, sizeof(WAVEHDR));
    
    bool UseBufferA = true;
    
    while (ThreadRunning) {
      WAVEHDR &CurrentHdr = UseBufferA ? WaveHdrA : WaveHdrB;
      short *CurrentBuf = UseBufferA ? BufferA : BufferB;
      
      while (ThreadRunning && !(CurrentHdr.dwFlags & WHDR_DONE) && (CurrentHdr.dwFlags & WHDR_PREPARED)) {
        Sleep(2);
      }
      
      if (!ThreadRunning) break;
      
      if (CurrentHdr.dwFlags & WHDR_PREPARED) {
        waveOutUnprepareHeader(hWaveOut, &CurrentHdr, sizeof(WAVEHDR));
      }
      
      float TargetFreq = TargetFrequency.load();
      float TargetVol = TargetVolume.load();
      
      for (int I = 0; I < BufferSize; I++) {
        CurrentFreq += (TargetFreq - CurrentFreq) * 0.005f;
        CurrentVol += (TargetVol - CurrentVol) * 0.005f;
        
        float SampleVal = 0.0f;
        int WaveType = Waveform.load();
        if (WaveType == 0) {
          SampleVal = (float)sin(Phase);
        } else if (WaveType == 1) {
          float NormPhase = Phase / (Pi * 2.0f);
          NormPhase = NormPhase - (int)NormPhase;
          if (NormPhase < 0.5f) {
            SampleVal = -1.0f + 4.0f * NormPhase;
          } else {
            SampleVal = 3.0f - 4.0f * NormPhase;
          }
        } else {
          SampleVal = ((float)rand() / (float)RAND_MAX - 0.5f) * 2.0f;
        }
        
        // Sfx Mixing
        float SfxSample = 0.0f;
        float SfxTime = SfxAge.load();
        if (SfxTime >= 0.0f) {
          int Type = SfxType.load();
          if (Type == 1) {
            if (SfxTime < 0.15f) {
              float SfxFreq = 600.0f - SfxTime * 1500.0f;
              float Env = (0.15f - SfxTime) / 0.15f;
              SfxSample = (float)sin(SfxTime * SfxFreq * Pi * 2.0f) * Env * 0.35f;
            } else {
              SfxAge = -1.0f;
            }
          } else if (Type == 2) {
            if (SfxTime < 0.8f) {
              float SfxFreq = 120.0f * (1.0f - SfxTime * 0.9f);
              float Env = (0.8f - SfxTime) / 0.8f;
              float NoiseVal = ((float)rand() / (float)RAND_MAX - 0.5f) * 2.0f;
              SfxSample = ((float)sin(SfxTime * SfxFreq * Pi * 2.0f) * 0.6f + NoiseVal * 0.4f) * Env * 0.70f;
            } else {
              SfxAge = -1.0f;
            }
          } else if (Type == 3) {
            if (SfxTime < 0.4f) {
              float Env = (0.4f - SfxTime) / 0.4f;
              float NoiseVal = ((float)rand() / (float)RAND_MAX - 0.5f) * 2.0f;
              SfxSample = NoiseVal * Env * 0.30f;
            } else {
              SfxAge = -1.0f;
            }
          }
          if (SfxAge.load() >= 0.0f) {
            SfxAge = SfxTime + 1.0f / (float)SampleRate;
          }
        }
        
        CurrentBuf[I] = (short)((SampleVal * CurrentVol + SfxSample) * 32767.0f);
        
        // Wind Soundscape modulated by atmosphere thickness
        float RawNoise = ((float)rand() / (float)RAND_MAX - 0.5f);
        float GustMod = (float)sin(Phase * 0.04f) * 0.12f + 0.22f;
        WindFilterState += (RawNoise - WindFilterState) * GustMod;
        
        float AtmosScale = (VisualAtmosphereAxis + 1.15f) / 2.3f;
        if (AtmosScale < 0.0f) AtmosScale = 0.0f;
        if (AtmosScale > 1.0f) AtmosScale = 1.0f;
        
        float WindVolume = AtmosScale * (0.045f + (float)sin(Phase * 0.04f) * 0.015f);
        float WindSample = WindFilterState * WindVolume;
        
        CurrentBuf[I] = (short)((SampleVal * CurrentVol + SfxSample + WindSample) * 32767.0f);
        
        Phase += (CurrentFreq * Pi * 2.0f) / (float)SampleRate;
        if (Phase > Pi * 2.0f) {
          Phase -= Pi * 2.0f;
        }
      }
      
      CurrentHdr.dwFlags = 0;
      waveOutPrepareHeader(hWaveOut, &CurrentHdr, sizeof(WAVEHDR));
      waveOutWrite(hWaveOut, &CurrentHdr, sizeof(WAVEHDR));
      
      UseBufferA = !UseBufferA;
    }
    
    waveOutReset(hWaveOut);
    waveOutUnprepareHeader(hWaveOut, &WaveHdrA, sizeof(WAVEHDR));
    waveOutUnprepareHeader(hWaveOut, &WaveHdrB, sizeof(WAVEHDR));
    waveOutClose(hWaveOut);
  }
  
  DWORD WINAPI AudioThreadFunc(LPVOID lpParam) {
    AudioLoop();
    return 0;
  }
  
  void Start() {
    if (ThreadRunning) return;
    ThreadRunning = true;
    hAudioThread = CreateThread(NULL, 0, AudioThreadFunc, NULL, 0, NULL);
  }
  
  void Stop() {
    if (!ThreadRunning) return;
    ThreadRunning = false;
    if (hAudioThread != NULL) {
      WaitForSingleObject(hAudioThread, INFINITE);
      CloseHandle(hAudioThread);
      hAudioThread = NULL;
    }
  }
}

void PlaySfx(int Type) {
  SoundSystem::SfxType = Type;
  SoundSystem::SfxAge = 0.0f;
}
