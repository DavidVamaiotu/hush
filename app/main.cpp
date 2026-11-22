#include <iostream>
#include <string>
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
        explicit Sunet(std::string filename) {
            float* pSampleData = drwav_open_file_and_read_pcm_frames_f32(filename.c_str(), &this->channels, &this->sampleRate, &this->totalPCMFrameCount, NULL);

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
   std::string adresa_fisier1;
   std::string adresa_fisier2;

   std::cout<<"Adresa fisier 1: ";
   std::cin >> adresa_fisier1;
   std::cout<<"Adresa fisier 2: ";
   std::cin >> adresa_fisier2;
   
   Sunet test(adresa_fisier1);
   Sunet test3(adresa_fisier2);
   Sunet test2 = test;

   test = test3;
}
