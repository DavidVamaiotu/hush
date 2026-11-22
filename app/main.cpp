#include <iostream>
#include <string>
#include <algorithm>
#include <vector>
#define DR_WAV_IMPLEMENTATION
#include <dr_wav.h>
#include <pocketfft_hdronly.h>


class Spectrograma {
    std::vector<std::vector<float>> values;
    unsigned int sampleRate;

    public:

        static const unsigned int windowFrame = 1024;
        static const unsigned int steps = 512;

        explicit Spectrograma(const std::vector<float>& SampleData, const unsigned int& sampleRate)
            : sampleRate(sampleRate)
        {
            if (SampleData.size() < windowFrame)
                throw std::runtime_error("Numarul de sample-uri trebuie sa fie mai mare");
 
            std::size_t windowFit = (SampleData.size() - windowFrame) / steps + 1;

            for (std::size_t i = 0; i < windowFit; i++)
            {
                std::vector<std::complex<float>> temp(windowFrame/2+1);
                pocketfft::r2c({windowFrame}, {sizeof(float)}, {sizeof(std::complex<float>)}, 0, pocketfft::FORWARD, SampleData.data() + i * steps, temp.data(), 1.0f);

                std::vector<float> valuesOfValues(windowFrame/2+1);

                for (std::size_t k = 0; k < temp.size(); k++)
                {
                    valuesOfValues[k] = std::abs(temp[k]);
                }

                values.push_back(valuesOfValues);
            }



        }

        std::size_t biggestValuePosFromWindow(std::size_t window) const
        {
            if (window >= this->values.size()) 
                throw std::runtime_error("Nu exista aceasta fereastra");


            float maxv = this->values[window][0];
            std::size_t pos = 0;
            for (std::size_t i = 1; i < this->values[window].size(); i++)
            {
                float candidate = this->values[window][i];
                if (candidate > maxv)
                {
                    maxv = candidate;
                    pos = i;
                }
            }
        
            return pos;
        }
};

class Sunet {
    unsigned int channels;
    unsigned int sampleRate;
    drwav_uint64 totalPCMFrameCount;
    std::vector<float> SampleData;


    public:


        // Deschidem fisierul, facem verificari si dupa initializam tabloul cu informatiile despre fisier
        explicit Sunet(const std::string& filename) {
            float* pSampleData = drwav_open_file_and_read_pcm_frames_f32(filename.c_str(), &this->channels, &this->sampleRate, &this->totalPCMFrameCount, nullptr);

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

        friend std::ostream& operator<<(std::ostream& os, const Sunet& s)
        {
            os << "Canale: "<<s.channels<<" "<<"Sample Rate: "<<s.sampleRate<<" "<<"Durata: "<<s.totalPCMFrameCount/s.sampleRate;
            return os;
        }


        Spectrograma calculeazaSpectrograma() const {
            return Spectrograma(this->SampleData, this->sampleRate);
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

   std::cout << test << "\n";

   Spectrograma spec = test.calculeazaSpectrograma();
   std::cout<<spec.biggestValuePosFromWindow(0);
}
