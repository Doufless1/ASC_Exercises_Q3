#include "AudioController.h"
#include <math.h>

AudioController::AudioController(int bclk_pin, int lrc_pin, int data_pin)
    : _bclk_pin(bclk_pin)
    , _lrc_pin(lrc_pin)
    , _data_pin(data_pin)
    , _sample_rate(44100)
    , _initialized(false)
    , _tone_playing(false)
    , _volume(0.1f)
    , _frequency_hz(440)
    , _phase(0.0f)
    , _beep_playing(false)
    , _beep_start_time(0)
    , _beep_duration(0)
    , _beep_frequency(1000)
    , _beep_volume(0.2f)
{
    memset(_audio_buffer, 0, sizeof(_audio_buffer));
}

AudioController::~AudioController() {
    if (_initialized) {
        stopTone();
        i2s_driver_uninstall(I2S_PORT);
        _initialized = false;
        Serial.println("AudioController stopped");
    }
}

bool AudioController::begin(uint32_t sample_rate) {
    if (_initialized) {
        return true;
    }
    
    _sample_rate = sample_rate;
    
    if (setupI2S()) {
        _initialized = true;
        Serial.println("AudioController initialized successfully");
        return true;
    }
    
    Serial.println("AudioController initialization failed");
    return false;
}

bool AudioController::setupI2S() {
    // I2S configuration
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = _sample_rate,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = BUFFER_SIZE,
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0
    };

    // I2S pin configuration
    i2s_pin_config_t pin_config = {
        .bck_io_num = _bclk_pin,
        .ws_io_num = _lrc_pin,
        .data_out_num = _data_pin,
        .data_in_num = I2S_PIN_NO_CHANGE
    };

    // Install I2S driver
    esp_err_t result = i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
    if (result != ESP_OK) {
        Serial.printf("Error installing I2S driver: %d\n", result);
        return false;
    }

    // Set I2S pins
    result = i2s_set_pin(I2S_PORT, &pin_config);
    if (result != ESP_OK) {
        Serial.printf("Error setting I2S pins: %d\n", result);
        i2s_driver_uninstall(I2S_PORT);
        return false;
    }

    return true;
}

void AudioController::playTone(int frequency_hz, float volume) {
    if (!_initialized) return;
    
    _frequency_hz = frequency_hz;
    _volume = constrain(volume, 0.0f, 1.0f);
    _tone_playing = true;
    _phase = 0.0f;
}

void AudioController::stopTone() {
    _tone_playing = false;
    _beep_playing = false;
    memset(_audio_buffer, 0, sizeof(_audio_buffer)); // clear audio buffer
    
    if (_initialized) {
        size_t bytes_written;
        i2s_write(I2S_PORT, _audio_buffer, sizeof(_audio_buffer), &bytes_written, 100);
    }
}

int AudioController::getFrequency() const {
    return _frequency_hz;
}

float AudioController::getVolume() const {
    return _volume;
}

bool AudioController::isInitialized() const {
    return _initialized;
}

void AudioController::update() {
    if (!_initialized) return;
    
    // Handle beep timing
    if (_beep_playing) {
        if (millis() - _beep_start_time >= _beep_duration) {
            stopTone();
            return;
        }
    }
    
    // Generate and play audio if needed
    if (_tone_playing || _beep_playing) {
        generateToneBuffer();
        playAudioBuffer();
    }
}

void AudioController::generateToneBuffer() {
    int current_frequency = _beep_playing ? _beep_frequency : _frequency_hz;
    float current_volume = _beep_playing ? _beep_volume : _volume;
    
    float phase_increment = 2.0f * M_PI * current_frequency / _sample_rate;;
    
    for (int i = 0; i < BUFFER_SIZE; i++) {
        float sample = sin(_phase) * current_volume * 32767.0f;
        _audio_buffer[i] = (int16_t)sample;
        
        _phase += phase_increment;
        if (_phase > 2.0f * M_PI) {
            _phase -= 2.0f * M_PI;
        }
    }
}

void AudioController::playAudioBuffer() {
    size_t bytes_written;
    esp_err_t result = i2s_write(I2S_PORT, _audio_buffer, sizeof(_audio_buffer), &bytes_written, portMAX_DELAY);
    
    if (result != ESP_OK) {
        Serial.printf("Error writing to I2S: %d\n", result);
    }
}

void AudioController::playBeep(int frequency_hz, uint32_t duration_ms, float volume) {
    if (!_initialized) return;
    
    _beep_frequency = constrain(frequency_hz, 20, 20000);
    _beep_duration = duration_ms;
    _beep_volume = constrain(volume, 0.0f, 1.0f);
    _beep_start_time = millis();
    _beep_playing = true;
    _tone_playing = true;  // Enable audio generation
    _phase = 0.0f;
}