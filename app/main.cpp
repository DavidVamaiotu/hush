#include <iostream>
#define DR_WAV_IMPLEMENTATION
#include <dr_wav.h>
#include <pocketfft_hdronly.h>


int main() {
    unsigned int channels;
    unsigned int sampleRate;
    drwav_uint64 totalPCMFrameCount;

    float* pSampleData = drwav_open_file_and_read_pcm_frames_f32("assets/melodii/test.wav", &channels, &sampleRate, &totalPCMFrameCount, NULL);


    if (pSampleData == NULL) {
        std::cerr<<"Eroare\n";
        return 1;
    }

    std::cout<<"Canale: "<<channels<<" Sample rate: "<<sampleRate<<" frame-uri: "<<totalPCMFrameCount;


    drwav_free(pSampleData, NULL);
}
