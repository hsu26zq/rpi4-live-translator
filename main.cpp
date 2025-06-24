#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>

#include <alsa/asoundlib.h>

// Replace these paths with your own
const std::string AUDIO_FILE = "live.wav";
const std::string TRANSCRIPT_FILE = "transcript.txt";
const std::string TRANSLATED_FILE = "translated.txt";
const std::string WHISPER_EXE = "/home/tommy/live-translator/whisper.cpp/build/bin/whisper-cli";
const std::string WHISPER_MODEL = "/home/tommy/live-translator/whisper.cpp/models/ggml-tiny.en.bin";
const std::string PYTHON_TRANSLATE_SCRIPT = "./translate.py";

bool record_audio(int duration_seconds = 5) {
    snd_pcm_t *pcm_handle;
    snd_pcm_hw_params_t *params;
    unsigned int rate = 48000;
    int channels = 2;
    int dir;
    snd_pcm_uframes_t frames = 32;

    int rc = snd_pcm_open(&pcm_handle, "hw:1,0", SND_PCM_STREAM_CAPTURE, 0);
    if (rc < 0) {
        std::cerr << "Unable to open PCM device: " << snd_strerror(rc) << "\n";
        return false;
    }

    snd_pcm_hw_params_malloc(&params);
    snd_pcm_hw_params_any(pcm_handle, params);
    snd_pcm_hw_params_set_access(pcm_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(pcm_handle, params, SND_PCM_FORMAT_S32_LE);
    snd_pcm_hw_params_set_channels(pcm_handle, params, channels);
    snd_pcm_hw_params_set_rate_near(pcm_handle, params, &rate, 0);
    snd_pcm_hw_params_set_period_size_near(pcm_handle, params, &frames, &dir);
    rc = snd_pcm_hw_params(pcm_handle, params);
    if (rc < 0) {
        std::cerr << "Unable to set HW parameters: " << snd_strerror(rc) << "\n";
        return false;
    }

    int buffer_size = frames * channels * 4; // 4 bytes per sample for S32_LE
    char *buffer = new char[buffer_size];

    std::ofstream wav_file(AUDIO_FILE, std::ios::binary);
    if (!wav_file) {
        std::cerr << "Failed to open audio file for writing.\n";
        return false;
    }

    // Write WAV header (simple, 5 sec length)
    auto write_wav_header = [&](std::ofstream& f) {
        // For brevity, a fixed 5-sec header is written
        int sample_rate = rate;
        short bits_per_sample = 32;
        short num_channels = channels;
        int byte_rate = sample_rate * num_channels * bits_per_sample / 8;
        int block_align = num_channels * bits_per_sample / 8;
        int data_size = sample_rate * num_channels * bits_per_sample / 8 * duration_seconds;

        f.write("RIFF", 4);
        int chunk_size = 36 + data_size;
        f.write(reinterpret_cast<char*>(&chunk_size), 4);
        f.write("WAVE", 4);
        f.write("fmt ", 4);
        int subchunk1_size = 16;
        f.write(reinterpret_cast<char*>(&subchunk1_size), 4);
        short audio_format = 3; // IEEE float = 3, PCM = 1 (use 3 for 32-bit float)
        f.write(reinterpret_cast<char*>(&audio_format), 2);
        f.write(reinterpret_cast<char*>(&num_channels), 2);
        f.write(reinterpret_cast<char*>(&sample_rate), 4);
        f.write(reinterpret_cast<char*>(&byte_rate), 4);
        f.write(reinterpret_cast<char*>(&block_align), 2);
        f.write(reinterpret_cast<char*>(&bits_per_sample), 2);
        f.write("data", 4);
        f.write(reinterpret_cast<char*>(&data_size), 4);
    };

    write_wav_header(wav_file);

    int total_frames = rate * duration_seconds;
    int frames_recorded = 0;

    while (frames_recorded < total_frames) {
        rc = snd_pcm_readi(pcm_handle, buffer, frames);
        if (rc == -EPIPE) {
            snd_pcm_prepare(pcm_handle);
            continue;
        } else if (rc < 0) {
            std::cerr << "Error reading from PCM device: " << snd_strerror(rc) << "\n";
            break;
        } else if (rc != (int)frames) {
            std::cerr << "Short read, expected " << frames << " got " << rc << "\n";
        }

        wav_file.write(buffer, rc * channels * 4);
        frames_recorded += rc;
    }

    delete[] buffer;
    wav_file.close();
    snd_pcm_drain(pcm_handle);
    snd_pcm_close(pcm_handle);
    snd_pcm_hw_params_free(params);

    return true;
}

// Helper to run system commands and capture output
std::string exec_command(const std::string& cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

void run_whisper() {
    std::string cmd = WHISPER_EXE + " -m " + WHISPER_MODEL +
                      " -f " + AUDIO_FILE +
                      " -otxt -of transcript -l en --threads 4";
    std::cout << "Running whisper: " << cmd << "\n";
    system(cmd.c_str());
}

void run_translation() {
    std::string cmd = "python3 " + PYTHON_TRANSLATE_SCRIPT + " transcript.txt " + TRANSLATED_FILE;
    std::cout << "Running translation: " << cmd << "\n";
    system(cmd.c_str());
}

int main() {
    std::cout << "Starting live translator (press Ctrl+C to exit)...\n";

    while (true) {
        std::cout << "[🎧] Recording audio...\n";
        if (!record_audio(5)) {
            std::cerr << "Audio recording failed!\n";
            continue;
        }

        run_whisper();

        if (!std::ifstream(TRANSCRIPT_FILE)) {
            std::cerr << "Transcript file missing!\n";
            continue;
        }

        run_translation();

        std::cout << "[✅] Translated text updated.\n";

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return 0;
}
