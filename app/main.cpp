#include <iostream>
#include <string>
#include <algorithm>
#include <vector>
#define DR_WAV_IMPLEMENTATION
#include <dr_wav.h>
#include <pocketfft_hdronly.h>
#include <complex> 
#include <stdexcept>
#include <map>


// Clasa pentru cele mai puternice frecvente, pentru ca nu trebuie sa stim toate frecventele, doar pe cele mai puternice
class Varf {
    long windowFrame;
    unsigned int freq;
    float power;

    public:
        explicit Varf(const long windowFrame, const unsigned int freq, const float power)
            : windowFrame(windowFrame), freq(freq), power(power)
        {}

        friend std::ostream& operator<<(std::ostream& os, const Varf& s)
        {
            os << "Fereastra: "<<s.windowFrame<<" "<<"Frecventa: "<<s.freq<<" "<<"Putere semnal: "<<s.power;
            return os;
        }

        bool egal(const Varf& vf2) const { return this->freq == vf2.freq; }

        long diferenta(const Varf& vf2) const { return this->windowFrame - vf2.windowFrame; }
    };


// Spectrograma care e calculata prin fast fourier transform
class Spectrograma {
    std::vector<std::vector<float>> values;
    unsigned int sampleRate;
    std::vector<Varf> peaks;

    public:

        //Fereastra pe care o citim
        static const unsigned int windowFrame = 1024;
        // Pasul dintre ferestre, ne mutam cu jumatate de fereastra, ca ferestrele sa se suprapuna
        static const unsigned int steps = 512;


        //Constructor prin care aplicam FFT sa obtinem puterea frecventelor din fiecare fereastra
        explicit Spectrograma(const std::vector<float>& SampleData, const unsigned int& sampleRate)
            : sampleRate(sampleRate)
        {
            if (SampleData.size() < windowFrame)
                throw std::runtime_error("Numarul de sample-uri trebuie sa fie mai mare");
 

            //Vedem cate ferestre incap
            std::size_t windowFit = (SampleData.size() - windowFrame) / steps + 1;

            //Calculam FFT pentru fiecare fereastra si apoi pastram rezultatele intr-un vector de vectori de float (randul = timpul si coloana = puterea frecventelor)
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

        //Cautam max si pos pentru varfuri
        static void searchForMaxAndPosInInterval(const std::size_t start, const std::size_t end, const std::vector<float>& v, float& maxv, std::size_t& pos)
        {
            for (std::size_t i = start; i < end; i++)
            {
                if (v[i] > maxv)
                {
                    maxv = v[i], pos = i;
                }
            }
        }

        const static std::size_t nrPeaks = 3; 

        // Cautam varfurile in fiecare fereastra, cate 3, deoarece ne intereseaza doar cele mai puternice frecvente
        void findPeaks() {
        
            for (std::size_t i = 0; i < this->values.size(); i++)
            {
                std::size_t bandWitdth = this->values[i].size() / nrPeaks;

                for (std::size_t j = 0; j < nrPeaks; j++)
                {
                    float maxv;
                    std::size_t pos;

                    std::size_t start = bandWitdth*j;
                    std::size_t end = bandWitdth*(j+1);
                    if (j == nrPeaks-1) end = this->values[i].size(); 

                    maxv = this->values[i][bandWitdth * j];
                    pos = bandWitdth*j;
                    searchForMaxAndPosInInterval(start, end, this->values[i], maxv, pos);
                    Varf vf(static_cast<long>(i), static_cast<unsigned int>(pos * sampleRate / windowFrame), maxv);
                    this->peaks.push_back(vf);
                }
            }
        }


        //Cea mai puternica frecventa din fereastra data
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

        long compara(const Spectrograma& spec) const
        {
            std::map<long, long> voturi;
            for (std::size_t i = 0; i < spec.peaks.size(); i++)
            {
                for (std::size_t j = 0; j < this->peaks.size(); j++)
                {
                    
                    const Varf& vf1 = this->peaks[j];
                    const Varf& vf2 = spec.peaks[i];

                    if (vf1.egal(vf2))
                    {   
                        long decalaj = vf1.diferenta(vf2);
                        voturi[decalaj]++;
                    }
                }
            }
            long maxv = 0;
            for (const auto& [dec, vot] : voturi)
            {
                  if (vot > maxv)
                  {
                     maxv = vot;
                  }
            }
            return maxv;
        }

        friend std::ostream& operator<<(std::ostream& os, const Spectrograma& s)
        {
            for (std::size_t i = 0; i < s.peaks.size(); i++)
                os << s.peaks[i] << '\n';
            
            return os;
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

    
        //Constructor pentru b = a (copiere)
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


class Melodie {
    std::string adresaFisier;
    Sunet sound;
    Spectrograma spec;

    public:

        explicit Melodie(const std::string& adresa)
            : adresaFisier(adresa), sound(adresa), spec(sound.calculeazaSpectrograma()) 
        {
            spec.findPeaks();
        }

        long scorComparatie(const Melodie& m2) const
        {
            return this->spec.compara(m2.spec);
        }


         friend std::ostream& operator<<(std::ostream& os, const Melodie& s)
        {
            os << s.adresaFisier << "\n" << s.sound << "\n" << s.spec;
            return os;
        }
};


int main() {

   try { 
        std::string adresa_fisier1;
        std::string adresa_fisier2;

        std::cout<<"Adresa fisier 1: ";
        std::getline(std::cin, adresa_fisier1);
        std::cout<<"Adresa fisier 2: ";
        std::getline(std::cin, adresa_fisier2);
        
        Sunet test(adresa_fisier1);
        Sunet test3(adresa_fisier2);
        Sunet test2 = test;

        test = test3;

        Spectrograma spec = test.calculeazaSpectrograma();
        std::cout << spec.biggestValuePosFromWindow(0) << '\n';

        Melodie ml1(adresa_fisier1);
        Melodie ml2(adresa_fisier2);

        std::cout<<ml2;
        std::cout<<ml1;

        std::cout<<ml2.scorComparatie(ml1);
        
    } catch(const std::exception& e) {
        std::cerr << "Eroare: " << e.what() << "\n";
        return 1;
    }
}
