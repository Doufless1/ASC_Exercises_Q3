#ifndef AUDIO_CONTROLLER_H
#define AUDIO_CONTROLLER_H

#include <Arduino.h>
#include <driver/i2s.h>

class AudioController {
public:
    // Constructor with default pins
    AudioController(int bclk_pin = 26, int lrc_pin = 27, int data_pin = 25);
    
    // Destructor
    ~AudioController();
    
    // Initialize the I2S audio system
    bool begin(uint32_t sample_rate = 44100);
        
    // Tone generation
    void playTone(int frequency_hz, float volume = 0.1);
    void stopTone();
    
    // Get frequency or volume from latest sound
    int getFrequency() const;
    float getVolume() const;
    
    // Audio processing - call this regularly in the main loop
    void update();
    
    // Utility functions
    void playTestSequence();
    void playBeep(int frequency_hz = 1000, uint32_t duration_ms = 200, float volume = 0.1);
    
    // Status
    bool isInitialized() const;
    
private:
    // Pin configuration
    int _bclk_pin;
    int _lrc_pin;
    int _data_pin;
    
    // Audio configuration
    uint32_t _sample_rate;
    static const int BUFFER_SIZE = 1024;
    int16_t _audio_buffer[BUFFER_SIZE];
    
    // State variables
    bool _initialized;
    bool _tone_playing;
    float _volume;
    int _frequency_hz;
    float _phase;
    
    // Beep functionality
    bool _beep_playing;
    uint32_t _beep_start_time;
    uint32_t _beep_duration;
    int _beep_frequency;
    float _beep_volume;
    
    // I2S port
    static const i2s_port_t I2S_PORT = I2S_NUM_0;
    
    // Private methods
    void generateToneBuffer();
    void playAudioBuffer();
    bool setupI2S();
};

#endif // AUDIO_CONTROLLER_H