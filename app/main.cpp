#include <iostream>
#include <algorithm>
#include <vector>
#define DR_WAV_IMPLEMENTATION
#include <dr_wav.h>
#include <pocketfft_hdronly.h>

class Sunet {
    unsigned int channels;
    unsigned int sampleRate;
    drwav_uint64 totalPCMFrameCount;
    std::vector<float> SampleData;


    public:


        // Deschidem fisierul, facem verificari si dupa initializam tabloul cu informatiile despre fisier
        explicit Sunet(const char* filename) {
            float* pSampleData = drwav_open_file_and_read_pcm_frames_f32(filename, &this->channels, &this->sampleRate, &this->totalPCMFrameCount, NULL);

            if (pSampleData == nullptr)
            {
               throw std::runtime_error("Eroare la initializarea fisierului");
            }
                
            if (this->channels != 1) {
                drwav_free(pSampleData, nullptr);
                throw std::runtime_error("Sunetul trebuie sa fie mono");
            }

            if (this->sampleRate != 11025) {
               drwav_free(pSampleData, nullptr);
               throw std::runtime_error("Sunetul trebuie sa aiba sample rate-ul de 11025");
            }

            SampleData.assign(pSampleData, pSampleData + totalPCMFrameCount);

            drwav_free(pSampleData, nullptr);
        }

    
        //Constructor pentru a = b (copiere)
        Sunet(const Sunet& other)
            : channels(other.channels), sampleRate(other.sampleRate), totalPCMFrameCount(other.totalPCMFrameCount), SampleData(other.SampleData)
        {}

        // operator=
        Sunet& operator=(const Sunet& other) {
            if (this == &other) {          // a = a;
                return *this;
            }        
            this->channels = other.channels;
            this->sampleRate = other.sampleRate;
            this->totalPCMFrameCount = other.totalPCMFrameCount;
            this->SampleData = other.SampleData;
            return *this;
        }


        ~Sunet() { }
};


int main() {
   Sunet test("assets/melodii/test.wav");
   Sunet test3("assets/melodii/test.wav");
   Sunet test2 = test;
  
   test = test3;

}
