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
    long fereastra;
    unsigned int frecventa;
    float putere;

    public:
        explicit Varf(const long fereastra_, const unsigned int frecventa_, const float putere_)
            : fereastra(fereastra_), frecventa(frecventa_), putere(putere_)
        {}

        friend std::ostream& operator<<(std::ostream& os, const Varf& varf)
        {
            os << "Fereastra: "<<varf.fereastra<<" "<<"Frecventa: "<<varf.frecventa<<" "<<"Putere semnal: "<<varf.putere;
            return os;
        }

        bool aceeasiFrecventa(const Varf& altVarf) const { return this->frecventa == altVarf.frecventa; }

        long decalajFata(const Varf& altVarf) const { return this->fereastra - altVarf.fereastra; }
    };


// Structura pentru a returna secunda de cand incepe fragmentul relativ la melodie, scorul (cate voturi are un decalaj) si procentul care ne spune cat de mult se aseamana cu melodia
struct RezultatPotrivire {
    long scor;
    float secundaStart;
    float procent;
};

// Spectrograma care e calculata prin fast fourier transform
class Spectrograma {
    //Vector care contine ferestrele citite
    std::vector<std::vector<float>> magnitudini;
    //Cate esantioane sunt citite intr-o secunda
    unsigned int sampleRate;
    //Cele mai puternice frecvente
    std::vector<Varf> varfuri;


     //Cautam max valoare si pozitia acesteia pentru varfuri
    static void maximInInterval(const std::size_t start, const std::size_t end, const std::vector<float>& randMagnitudini, float& valoareMax, std::size_t& pozitieMax)
    {
        for (std::size_t i = start; i < end; i++)
        {
            if (randMagnitudini[i] > valoareMax)
            {
                valoareMax = randMagnitudini[i], pozitieMax = i;
            }
        }
    }


    float ferestreInSecunde(long nrFerestre) const {
        return static_cast<float>(this->pas) * nrFerestre / this->sampleRate;
    }

    // Cautam varfurile in fiecare fereastra, cate 3, deoarece ne intereseaza doar cele mai puternice frecvente
    void gasesteVarfuri() {
            for (std::size_t fereastra = 0; fereastra < this->magnitudini.size(); fereastra++)
            {
                std::size_t latimeBanda = this->magnitudini[fereastra].size() / nrBenzi;

                for (std::size_t banda = 0; banda < nrBenzi; banda++)
                {
                    float valoareMax;
                    std::size_t pozitieMax;

                    std::size_t start = latimeBanda*banda;
                    std::size_t end = latimeBanda*(banda+1);
                    if (banda == nrBenzi-1) end = this->magnitudini[fereastra].size();

                    valoareMax = this->magnitudini[fereastra][latimeBanda * banda];
                    pozitieMax = latimeBanda*banda;
                    maximInInterval(start, end, this->magnitudini[fereastra], valoareMax, pozitieMax);
                    Varf varf(static_cast<long>(fereastra), static_cast<unsigned int>(pozitieMax * sampleRate / marimeFereastra), valoareMax);
                    this->varfuri.push_back(varf);
                }
            }
        }

    public:

        //Marimea ferestrelor pe care le citim si pe care aplicam FFT
        static const unsigned int marimeFereastra = 1024;
        // Pasul dintre ferestre, ne mutam cu jumatate de fereastra, ca ferestrele sa se suprapuna (aprox 0.05s)
        static const unsigned int pas = 512;




        //Constructor prin care aplicam FFT sa obtinem puterea frecventelor din fiecare fereastra
        explicit Spectrograma(const std::vector<float>& esantioane, const unsigned int& sampleRate_)
            : sampleRate(sampleRate_)
        {
            if (esantioane.size() < marimeFereastra)
                throw std::runtime_error("Numarul de sample-uri trebuie sa fie mai mare");


            //Vedem cate ferestre incap
            std::size_t nrFerestre = (esantioane.size() - marimeFereastra) / pas + 1;

            //Calculam FFT pentru fiecare fereastra si apoi pastram rezultatele intr-un vector de vectori de float (randul = timpul si coloana = puterea frecventelor)
            for (std::size_t i = 0; i < nrFerestre; i++)
            {
                std::vector<std::complex<float>> spectruComplex(marimeFereastra/2+1);

                //Aplica FFT care transforma din real in complex, primeste ca input:
                //{marimeFereastra}: o fereastră de 1024 de eșantioane
                //{sizeof(float)}: cati bytes sunt intre 2 esantioane de intrare consecutive (logic float)
                //{sizeof(std::complex<float>)}: cati bytes sunt intre 2 esantioane de iesire consecutive (logic complex float)
                //0: axa pe care se face FFT, avem o singura dimensiune deci 0
                //pocketfft::FORWARD: Directia pe care se face FFT, din timp in frecventa
                //esantioane.data() + i * pas: de unde incepe fereastra i in vectorul de esantioane
                //spectruComplex.data(): unde sunt scrise cele 513 numere complexe
                //1.0f: factor de scalare
                pocketfft::r2c({marimeFereastra}, {sizeof(float)}, {sizeof(std::complex<float>)}, 0, pocketfft::FORWARD, esantioane.data() + i * pas, spectruComplex.data(), 1.0f);

            

                std::vector<float> magnitudiniFereastra(marimeFereastra/2+1);

                for (std::size_t k = 0; k < spectruComplex.size(); k++)
                {
                    magnitudiniFereastra[k] = std::abs(spectruComplex[k]);
                }

                magnitudini.push_back(magnitudiniFereastra);
            }

            this->gasesteVarfuri();
        }


        // In cate benzi de frecventa impartim fiecare fereastra; din fiecare banda
        // pastram cel mai puternic varf, deci avem nrBenzi varfuri pe fereastra
        const static std::size_t nrBenzi = 3;


        //Comparam doua spectograme sa vedem daca au peak-uri asemanatoare si calculam decalajul
        RezultatPotrivire compara(const Spectrograma& fragment) const
        {
            std::map<long, long> voturi;
            for (std::size_t i = 0; i < fragment.varfuri.size(); i++)
            {
                for (std::size_t j = 0; j < this->varfuri.size(); j++)
                {

                    const Varf& varfMelodie = this->varfuri[j];
                    const Varf& varfFragment = fragment.varfuri[i];

                    if (varfMelodie.aceeasiFrecventa(varfFragment))
                    {
                        long decalaj = varfMelodie.decalajFata(varfFragment);
                        voturi[decalaj]++;
                    }
                }
            }
            long voturiMax = 0, decalajCastigator = 0;
            for (const auto& [decalaj, nrVoturi] : voturi)
            {
                  if (nrVoturi > voturiMax)
                  {
                     voturiMax = nrVoturi;
                     decalajCastigator = decalaj;
                  }
            }

            return RezultatPotrivire {voturiMax, ferestreInSecunde(decalajCastigator), static_cast<float>(voturiMax) / fragment.varfuri.size() * 100};
        }

        friend std::ostream& operator<<(std::ostream& os, const Spectrograma& spectrograma)
        {
            os << "Ferestre: " << spectrograma.magnitudini.size() << '\n';
            os << "Varfuri: " << spectrograma.varfuri.size() << '\n';
            for (std::size_t i = 0; i < nrBenzi; i++)
                os << spectrograma.varfuri[i] << '\n';
            return os;
        }

};

//Clasa sunet prin care luam detalii dintr-un fisier despre un sunet si prelucram informatii cu libraria drwav
class Sunet {
    unsigned int canale;
    unsigned int sampleRate;
    drwav_uint64 nrEsantioane;
    std::vector<float> esantioane;

    public:

        // Deschidem fisierul, facem verificari si dupa initializam tabloul cu informatiile despre fisier folosind libraria drwav
        explicit Sunet (const std::string& caleFisier) {
            float* bufferDrWav = drwav_open_file_and_read_pcm_frames_f32(caleFisier.c_str(), &this->canale, &this->sampleRate, &this->nrEsantioane, nullptr);

            if (bufferDrWav == nullptr)
            {
               throw std::runtime_error("Eroare la initializarea fisierului");
            }

            if (this->canale != 1) {
                drwav_free(bufferDrWav, nullptr);
                throw std::runtime_error("Sunetul trebuie sa fie mono");
            }

            if (this->sampleRate != 11025) {
               drwav_free(bufferDrWav, nullptr);
               throw std::runtime_error("Sunetul trebuie sa aiba sample rate-ul de 11025");
            }

            esantioane.assign(bufferDrWav, bufferDrWav + nrEsantioane);

            drwav_free(bufferDrWav, nullptr);
        }


        Sunet(unsigned int canale_, unsigned int sampleRate_, const std::vector<float>& esantioane_)
            : canale(canale_), sampleRate(sampleRate_), nrEsantioane(esantioane_.size()), esantioane(esantioane_)
        {}



        Sunet taieFragment(float secundaStart, float durata) const {
            if (secundaStart < 0 || durata < 0)
                throw std::runtime_error("Secunda de start sau durata nu pot fi negative");

            std::size_t nrEsantioaneStart = static_cast<std::size_t>(secundaStart * this->sampleRate);
            std::size_t nrEsantioaneDurata = static_cast<std::size_t>(durata * this->sampleRate);

            std::size_t nrEsantioaneMelodie = this->esantioane.size();

            if (nrEsantioaneStart + nrEsantioaneDurata > nrEsantioaneMelodie)
                throw std::runtime_error("Fragmentul nu poate fi mai lung decat melodia");

            if (nrEsantioaneDurata < Spectrograma::marimeFereastra)
                throw std::runtime_error("Durata e prea scurta");

            std::vector<float> esantioaneFragment(this->esantioane.begin() + nrEsantioaneStart, this->esantioane.begin() + nrEsantioaneStart + nrEsantioaneDurata);

            return Sunet(this->canale, this->sampleRate, esantioaneFragment);

        }


        //Constructor de copiere: Sunet b = a;
        Sunet(const Sunet& other)
            : canale(other.canale), sampleRate(other.sampleRate), nrEsantioane(other.nrEsantioane), esantioane(other.esantioane)
        {}

        // operator=
        Sunet& operator=(const Sunet& other) {
            if (this == &other) {          // a = a;
                return *this;
            }
            this->canale = other.canale;
            this->sampleRate = other.sampleRate;
            this->nrEsantioane = other.nrEsantioane;
            this->esantioane = other.esantioane;
            return *this;
        }

        friend std::ostream& operator<<(std::ostream& os, const Sunet& sunet)
        {
            os << "Canale: "<<sunet.canale<<" "<<"Sample Rate: "<<sunet.sampleRate<<" "<<"Durata: "<<sunet.nrEsantioane/sunet.sampleRate;
            return os;
        }


        Spectrograma calculeazaSpectrograma() const {
            return Spectrograma(this->esantioane, this->sampleRate);
        }


        ~Sunet() { }
};


class Melodie {
    std::string nume;
    Sunet sunet;
    Spectrograma spectrograma;

    public:

        explicit Melodie(const std::string& caleFisier)
            : nume(caleFisier), sunet(caleFisier), spectrograma(sunet.calculeazaSpectrograma())
        { }


        Melodie(const std::string& nume_, const Sunet& sunet_)
            : nume(nume_), sunet(sunet_), spectrograma(sunet.calculeazaSpectrograma())
        { }


        //Cat de bine se potriveste fragmentul cu melodia
        long scorComparatie(const Melodie& fragment) const
        {
            return this->spectrograma.compara(fragment.spectrograma).scor;
        }

        //Pragul pentru care un fragment e recunoscut ca facand parte dintr-o melodie
        static constexpr float pragComparatie = 30.0f;

        // La ce secunda se afla fragmentul relativ la melodie
        float secundaFragment(const Melodie& fragment) const {
            return this->spectrograma.compara(fragment.spectrograma).secundaStart;
        }


        // % potrivire fragment cu melodie (verifica varfurile)
        float procentPotrivire(const Melodie& fragment) const
        {
            return this->spectrograma.compara(fragment.spectrograma).procent;
        }


         friend std::ostream& operator<<(std::ostream& os, const Melodie& melodie)
        {
            os << melodie.nume << "\n" << melodie.sunet << "\n" << melodie.spectrograma;
            return os;
        }
};


int main() {

   try {


        long scorMax = 0;

        std::string caleFragment;
        std::string caleMelodie;

        std::cout<<"Adresa fragment: ";

        if (!std::getline(std::cin, caleFragment) || caleFragment == "0") {
            return 0;
        }

        std::cout<<"Adresa melodie: ";

        if (!std::getline(std::cin, caleMelodie)) {
            return 0;
        }

        Melodie fragment(caleFragment);
        Melodie melodie(caleMelodie);

        Melodie celMaiBunFragment = fragment;


        while (true)
        {
            std::cout<<"--------"<<'\n';
            std::cout<<melodie;
            std::cout<<"--------"<<'\n';
            std::cout<<fragment;
            std::cout<<"--------"<<'\n';


            long scor = melodie.scorComparatie(fragment);

            if (scor == 0)
            {
                std::cout<<"Fragmentul nu a fost gasit in melodie"<<'\n';
            }
            else {
                 std::cout<<"Scor potrivire: "<< scor << '\n';

                if (melodie.procentPotrivire(fragment) < Melodie::pragComparatie)
                    std::cout<<"Fragmentul nu face parte din melodie" << '\n';
                else
                {
                    std::cout<<"Fragmentul incepe la secunda: "<< melodie.secundaFragment(fragment) << '\n';
                    std::cout<<"--------"<<'\n';

                    if (scor > scorMax)
                    {
                        scorMax = scor;
                        celMaiBunFragment = fragment;
                    }
                }

            }

            std::cout<<"Adresa fragment: ";

            if (!std::getline(std::cin, caleFragment) || caleFragment == "0") {
                std::cout << '\n';
                std::cout << "--------"<<'\n';
                std::cout << "Fragmentul care se potriveste cel mai bine: " << celMaiBunFragment;
                std::cout << "Cu scorul: " << scorMax << '\n';
                std::cout<<"--------"<<'\n';

                Sunet sunetTaiat(caleMelodie);

                float secundaStart = 0, durata = 0;

                std::cout << "Secunda la care sari: " << '\n';
                std::cin >> secundaStart;
                std::cout << "Durata: " << '\n';
                std::cin >> durata;
                std::cout<<"--------"<<'\n' << '\n';

                sunetTaiat = sunetTaiat.taieFragment(secundaStart, durata);

                Melodie melodieTaiata("fragment incepand cu secunda " + std::to_string(secundaStart), sunetTaiat);

                std::cout<<"Fragment taiat la " << secundaStart << ", gasit la secunda: " << melodie.secundaFragment(melodieTaiata) << '\n';


                return 0;
            }

            fragment = Melodie(caleFragment);

        }


    } catch(const std::exception& e) {
        std::cerr << "Eroare: " << e.what() << "\n";
        return 1;
    }
}
