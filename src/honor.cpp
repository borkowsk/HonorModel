// Prototypowy program "ewolucji" kultury honoru w kultur� policyjn�
//                                                             wersja 21-08-2012
////////////////////////////////////////////////////////////////////////////////
#define _USE_MATH_DEFINES //bo M_PI
#include <cmath>

#include <iostream>
#include <fstream>
#include <iomanip>

#define HIDE_WB_PTR_IO 1
#include "INCLUDE/wb_ptr.hpp"
#include "INCLUDE/OptParam.hpp"


#include "SYMSHELL/symshell.h"
#include "SYMSHELL/sshutils.hpp"
#include "MISCCLASSES/Wieloboki.hpp"

//#include "INCLUDE/wb_bitarray.h" //Mo�e si� przyda p�niej
#define USES_STDC_RAND
#include "INCLUDE/Random.h"

using namespace wbrtm;
using namespace std;

const char* MODELNAME="Culture of honor";
const char* VERSIONNUM="0.3a (05-04-2013)";
typedef double FLOAT;


unsigned population_growth=1;//SPOSOBY ROZMNA�ANIA
							 // 0 - wg. inicjalnych proporcji
							 // 1 - lokalne rozmazanie losowe s�siad
							 // 2 - lokalne rozmazanie proporcjonalne do sily  //NIE ZAIMPLEMENTOWANE!
							 // 3 - globalne, losowy ziutek z ca�o�ci
bool  batch_mode=true;       //Czy tryb pracy przeszukiwania przestrzeni parametr�w?
enum  {NO_BAT=0,BAT_SELECTION=1,BAT_HONORvsCPOLL=2,BAT_HONORvsAGRR=3} batch_sele=BAT_SELECTION;		  //Czy tryb przeszukiwania szuka po proporcjach czy po sile selekcji?
char*  batch_names[]={"NO_BAT","BAT_SELECTION","BAT_HONORvsCPOLL","BAT_HONORvsAGRR"}; //batch_names[batch_sele]

// Przykladowa lista parametr�w:
//POLICEEF=0 BULLYPR=.25 HONORPR=.11 CALLPRP=.22 MAXSTEP=10000 VISSTEP=10 GROWMODE=1 REPETITIONS=10
//
//ZMIANY:
//0.30a - r�ne zmiany w sposobie wyprowadzania danych tekstowych i trzeci rodzaj eksploracji
//0.28aa- pocz�tek tworzenia trzeciego rodzaju przekroju przestrzeni parametr�w, oraz poprawki
//0.27  - wykres przestrzeni selekcja x polic.effic.
//0.2631 - Drobne poprawki???
//0.263 - Wi�kszo�� parametr�w obs�ugiwana przez szablon klasy z "OptParam"
//0.262 - WA�NE - WIMPowie patrz� teraz nie na swoj� realn� si��, ale na w�asn� reputacje,
//		  wi�c de facto poza dzieci�stwem prawdopodobnie nigdy si� nie broni�
//		  Techniczne - wprowadzenie u�ycia klasy parametr�w do obs�ugi parametr�w modelu
//0.26 -  Wykres przestrzeni - w proporcji honorowych i policyjnych do siebie dziel�cych 1/2 lub 2/3 ca�o�ci.
//		  Inny spos�b kolorowania "kultur" na obrazie �wiata symulacji (czerwony-agresja, zielony-honor, niebieski-callPolice)
//		  Pr�ba zmiany rozk�adu �omotu dla GIVEUP - �eby mogli zgin��, ale bardziej niszczy bullys!?!?
//		  Do ewolucji potrzebny szumowe �mierci - parametr NOISE_KILL
//		  ewolucja:
//		  - lokalne rozmazanie losowe s�siad
//		  TODO  - losowy ziutek z ca�o�ci
//		  TODO  - lokalne rozmazanie proporcjonalne do sily
//		  Statystyki: ile razy agent jest atakowany i ile razy wygra� ,
//						klastering index dla kultur
//        Czesto�ci atak�w na poszczeg�lne grupy w czasie - bez selekcji, zysk indywidualny
//        Du�a mapa z gradientem policji
//
//		  ewentualnie r�na aktywnosc honorowych. Nie zawsze musz� stawa�, ale z prawdopodobienstwem
//		 NA POZNIEJ: ilo�� policji zale�y od ilo�ci atak�w? Inny model - podatkowy.
//
//0.251 - poprawienie b��du w liczeniu "always give up" na metryczce pliku log
//
//0.25 - wprowadzenie statystyk z si�y r�nych grup (najwy�szy decyl tylko)
//		 i ich drukowania do pliku i na konsol�
//		 Uelastycznienie wydruku metryczki, �eby nadawa�a si� i do pionu i do poziomu ...
//		 WPROWADZENIE TRYBU PRZESZUKIWANIA PRZESTRZENI PARAMETR�W
//		 W tym ZAPISU BITMAP Z WYNIKAMI
//0.24 - wprowadzenie parametru RATIONALITY, bo u�ycie w pe�ni realistycznej oceny si�y psuje selekcje
//		 Zmiany kosmetyczne w nag��wku pliku wyj�ciowego
//0.23 - znaj� swoj� realn� si�� i j� por�wnuj� z reputacj� drugiego gdy maj� atakowa� lub si� broni�
//		 Ale to nie dzia�a�o dobrze...
//0.20 - pierwsza wersja w pe�ni dzia�aj�ca

//MACIE� I LINKOWANIE  AGENT�W
const unsigned SIDE=100;//SIDE*SIDE to rozmiar �wiata symulacji
const int	   MOORE_RAD=3; //Nie zmienia� na unsigned bo si� psuje losowanie!
const FLOAT    OUTFAR_LINKS_PER_AGENT=0.5; //Ile jest dodatkowych link�w jako u�amek liczby agent�w
const unsigned MOORE_SIZE=(4*MOORE_RAD*MOORE_RAD)+4*MOORE_RAD;//S�siedzi Moora - cztery kwadranty + 4 osie
const unsigned FAR_LINKS=unsigned(SIDE*SIDE*OUTFAR_LINKS_PER_AGENT*2);
const unsigned MAX_LINKS=MOORE_SIZE + 2/*ZAPAS*/ + (FAR_LINKS/(SIDE*SIDE)? FAR_LINKS/(SIDE*SIDE):2); //Ile maksymalnie mo�e miec agent link�w
bool 	       TORUS=false; //Czy geometria torusa czy wyspy z brzegami

//INNE GLOBALNE WLASCIWOSCI SWIATA
FLOAT    USED_SELECTION=0.05;//0.10; //Jak bardzo przegrani umieraj� (0 - brak selekcji w og�le)
FLOAT    NOISE_KILL=0.001; //Jakie jest prawdopodobienstwo przypadkowej �mierci
FLOAT    RANDOM_AGRESSION=0.05;//0.015;//Bazowy poziom agresji zale�ny od honoru

const FLOAT    RECOVERY_POWER=0.005;//Jak� cz�� si�y odzyskuje w kroku
const FLOAT    RATIONALITY=0.0; //Jak realistycznie ocenia w�asn� si�� (vs. wg. w�asnej reputacji)

//INDYWIDUALNE CECHY AGENT�W
const FLOAT    BULLISM_LIMIT=-1;//0.2;//0.66;//0.10;//Maksymalny mo�liwy bulizm.
								//Jak ujemne to rozk�ad Pareto lub brak rozk�adu, jak dodatnie to dzwonowy

FLOAT    BULLI_POPUL=-0.25;//0.2;//0.100;//Albo zero-jedynkowo. Jak 1 to decyduje rozk�ad sterowany BULLISM_LIMIT
						   //("-" jest sygna�em zafiksowania w trybie batch
FLOAT	 HONOR_POPUL=0.18;//0.3333;//Jaka cz�� agent�w populacji jest �ci�le honorowa
FLOAT    CALLER_POPU=0.25;//Jaka cz�� wzywa policje zamiast si� poddawa�

FLOAT    POLICE_EFFIC=0.50;//0.650;//0.950; //Z jakim prawdopodobie�stwem wezwana policja obroni agenta

//const unsigned MAX_INIT_REPUTATION=100; //Maksymalna mo�liwa pocz�tkowa reputacja
//const double   SOCIAL_IMPACT_INTENSITY_PERCENT=0; //Jaka jest wzgl�dna intensywno�� wp�ywu spo�ecznego. W procentach kroku MC g��wnej dynamiki. 0 oznacza brak

//Eksploracja przestrzeni parametr�w
bool  Compensation_mode=true;//Czy przestrze� gdzie Honorowi i CallPolice, albo Agresywni i Honorowi uzupe�niaj� si� do PROP_MAX
							 //Ma sens tylko dla zafiksowanych pozosta�ych
FLOAT POLICE_EFFIC_STEP=0.25;
FLOAT POLICE_EFFIC_MAX=1;
FLOAT POLICE_EFFIC_MIN=0;

FLOAT SELECTION_STEP=0.05;
FLOAT SELECTION_MAX=0.5;
FLOAT SELECTION_MIN=0;

FLOAT PROPORTION_STEP=0.05;
FLOAT PROPORTION_MAX=1.0/3.0;
FLOAT PROPORTION_MIN=0.0;

//Sterowanie statystykami i powt�rzeniami
unsigned REPETITION_LIMIT=1;//10//Ile ma zrobi� powt�rze� tego samego eksperymentu
unsigned RepetNum=1; //Kt�ra to kolejna repetycja?  - NIE ZMIENIA� R�CZNIE!
unsigned STOP_AFTER=10000;//Po jakim czasie staje automatycznie
unsigned STAT_AFTER=0; //Po jakim czasie zacz�� zlicza� koncow� statystyk�
unsigned EveryStep=100;//Cz�stotliwo�� wizualizacji i zapisu do logu
					 //Ujemne oznacza tryb automatycznej inkrementacji
unsigned DumpStep=1000;//Cz�sto�� zrzut�w stan�w agent�w

//Parametry techniczne steruj�ce wizualizacj� i wydrukami
unsigned VSIZ=5; //Maksymalny rozmiar boku agenta w wizualizacji kompozytowej
unsigned SSIZ=1; //Bok agenta w wizualizacji uzupe�niaj�cej (ma�ej)
bool  ConsoleLog=true;//Czy u�ywa logowania zdarze� ma konsoli. Wa�ne dla startu, potem si� da prze��cza�
bool  VisShorLinks=false; //Wizualizacja bliskich link�w
bool  VisFarLinks=false;  //Wizualizacja dalekich
bool  VisAgents=true;     //Wizualizacja w�a�ciwo�ci agent�w
bool  dump_screens=false;
bool  BatchPlotPower=false;   //Czy w trakcie wy�wietla� MnPower czy jednak MnProportions
bool  Batch_true_color=false;//Czy skale kolor�w true-color czy 256 kolor�w t�czy

//Nazwy plik�w
char* LogName="Honor";
char* DumpNam="Honor_dump";


OptionalParameterBase* Parameters[]={ //sizeof(Parameters)/sizeof(Parameters[])
new ParameterLabel("PARAMETERS FOR SINGLE SIMULATION"),
new OptionalParameter<unsigned>(population_growth,0,3,"GROWMODE","How population growth?\n\t  0-as initial distribution, 1-local distribution, 3-global distribution\n\t "),
new OptionalParameter<bool>(TORUS,false,true,"TORUS","Is the world topology toroidal or not"), //Czy geometria torusa czy wyspy z brzegami
new OptionalParameter<FLOAT>(USED_SELECTION,0,0.75,"SELECTION","Minimal level of strenght required to survive"),//0.10; //Jak bardzo przegrani umieraj� (0 - brak selekcji w og�le)
new OptionalParameter<FLOAT>(NOISE_KILL,0,0.05,    "NOISEKILL","Ratio of agents randomly killed in each step"), //Jakie jest prawdopodobienstwo przypadkowej �mierci
new OptionalParameter<FLOAT>(RANDOM_AGRESSION,0,0.15,"HONAGRES","Probability of random agresion of honor agents"),//0.015;//Bazowy poziom agresji zale�ny od honoru
new OptionalParameter<FLOAT>(BULLI_POPUL,0,1,"BULLYPR","Initial probability to born as bully agent"),//=-0.25;//0.2;//0.100;//Albo zero-jedynkowo. Jak 1 to decyduje rozk�ad sterowany BULLISM_LIMIT ("-" jest sygna�em zafiksowania w trybie batch
new OptionalParameter<FLOAT>(HONOR_POPUL,0,1,"HONORPR","Initial probability to born as honor agent"),//=0.18;//0.3333;//Jaka cz�� agent�w populacji jest �ci�le honorowa
new OptionalParameter<FLOAT>(CALLER_POPU,0,1,"CALLPRP","Initial probability to born as police caller"),//=0.25;//Jaka cz�� wzywa policje zamiast si� poddawa�
new OptionalParameter<FLOAT>(POLICE_EFFIC,0,1,"POLICEEF","Probability of efficient police intervention"),//=0.50;//0.650;//0.950; //Z jakim prawdopodobie�stwem wezwana policja obroni agenta
new ParameterLabel("PARAMETERS FOR MULTIPLE SIMULATIONS (EXPLORATION/BATCH MODE)"),
new OptionalParameter<bool>(batch_mode,false,true,"BATCH","To switch into parametr space batch mode"),
//Nie ma jeszcze szablonu dla enumeracji wi�c chamski rzut na "unsigned int"
new OptionalParameter<unsigned>(*((unsigned*)&batch_sele),1,3,"BSELE","To switch batches bettwen SELECTION=1,HONORvsCPOLL=2,HONORvsAGRR=3"),
new OptionalParameter<FLOAT>(POLICE_EFFIC_STEP,0,1.0,"PEFFSTEP","Pol. efficiency exploration step"),//=0.1;
new OptionalParameter<FLOAT>(POLICE_EFFIC_MAX,0,1.0,"PEFFMAX","Pol. efficiency exploration maximum"),//=1;
new OptionalParameter<FLOAT>(POLICE_EFFIC_MIN,0,1.0,"PEFFMIN","Pol. efficiency exploration minimum"),//=0;
new OptionalParameter<FLOAT>(PROPORTION_STEP,0,1.0,"PROPSTEP","Proportion exploration step"),//=0.03;
new OptionalParameter<FLOAT>(PROPORTION_MAX,0,1.0,"PROPMAX","Proportion exploration maximum"),//=1.0/3.0;
new OptionalParameter<FLOAT>(PROPORTION_MIN,0,1.0,"PROPMIN","Proportion exploration minimum"),//=0.0;
new OptionalParameter<FLOAT>(SELECTION_STEP,0,1.0,"SELESTEP","Selection exploration step"),//=0.1;
new OptionalParameter<FLOAT>(SELECTION_MAX,0,1.0,"SELEMAX","Selection exploration maximum"),//=1;
new OptionalParameter<FLOAT>(SELECTION_MIN,0,1.0,"SELEMIN","Selection exploration minimum"),//=0;
new ParameterLabel("STATISTIC & SO... HOW MANY, HOW OFFTEN, WHERE"),
new OptionalParameter<unsigned>(REPETITION_LIMIT,1,1000,"REPETITIONS","How many repetitions for each settings we expect?"),
new OptionalParameter<unsigned>(STAT_AFTER,1,1000000,"STATSTART","When start to calculate statistics in batch mode?"),
new OptionalParameter<unsigned>(STOP_AFTER,1,1000000,"MAXSTEP","To stop each simulation after this number of steps"),
new OptionalParameter<unsigned>(EveryStep,1,10000,   "VISSTEP","For set how offten visualisation and statistics occur?"),
new OptionalParameter<unsigned>(DumpStep,1,1000000,  "DMPSTEP","For set how offten agents attributes and graphix view is dumped?"),
new OptionalParameter<const char*>(LogName,"honor","HONOR","LOGNAME","Name for main log file"),
new OptionalParameter<const char*>(DumpNam,"honor_dump","DUMP","DUMPNAME","Name for main log file"),
new ParameterLabel("VISUALISATION OPTIONS"),
new OptionalParameter<bool>(ConsoleLog,  false,true, "CONSOLOG","To log events on console window"),
new OptionalParameter<bool>(dump_screens,false,true, "DUMPSCR","Dump graphix screen after every visualisation frame"),
new OptionalParameter<bool>(VisShorLinks,false,true, "VISSHORT","To visualise short links"),
new OptionalParameter<bool>(VisFarLinks, false,true, "VISFARLN","To visualise far connections"),
new OptionalParameter<bool>(VisAgents,   false,true, "VISAGENT","To visualise composed view of agents"),
new OptionalParameter<bool>(BatchPlotPower,false,true,"BATPOWER","To plot mean power during batch simulations"),
new OptionalParameter<bool>(Batch_true_color,false,true,"BATTRCOL","To plot in true colors during batch sim."),
new OptionalParameter<unsigned>(VSIZ,3,20,"AGENTSIZ1","Side of an agent on the composed visualisation"), //Maksymalny rozmiar boku agenta w wizualizacji kompozytowej
new OptionalParameter<unsigned>(SSIZ,1,5,"AGENTSIZ2","Side of an agent on the small visualisations") //Bok agenta w wizualizacji uzupe�niaj�cej (ma�ej)
};

void Parameters_dump(ostream& o,const char* SEP="\t",const char* ENDL="\n",bool FL=true)
{
	o<<MODELNAME<<"\tv.:"<<SEP<<VERSIONNUM<<ENDL;
	o<<"uint"<<SEP<<"SIDE"<<SEP<<SIDE<<ENDL;//SIDE*SIDE to rozmiar �wiata symulacji
	o<<"bool"<<SEP<<"TORUS"<<SEP<<(TORUS?"true":"false")<<ENDL;

	o<<"uint "<<SEP<<"MOORE_RAD"<<SEP<<MOORE_RAD<<ENDL;
	o<<"FLOAT"<<SEP<<"OUT_FAR_LINKS_PER_AGENT"<<SEP<<OUTFAR_LINKS_PER_AGENT<<ENDL;//Ile jest dodatkowych link�w jako u�amek liczby agent�w
	o<<"FLOAT"<<SEP<<"RECOVERY_POWER"<<SEP<<RECOVERY_POWER<<ENDL;//Jak� cz�� si�y odzyskuje w kroku
	o<<"uint "<<SEP<<"POP_GROWTH_MODE"<<SEP<<population_growth<<ENDL;//Rodzaj wzrostu populacji (z prop. bazowych,globalnych lub lokalnych)
	o<<"FLOAT"<<SEP<<"RANDOM_AGRESSION"<<SEP<<RANDOM_AGRESSION<<ENDL;//0.950;
	o<<"FLOAT"<<SEP<<"NOISE_KILL"<<SEP<<NOISE_KILL<<ENDL;

	if(batch_mode)
	{
		o<<"FLOAT"<<SEP<<"POLICE_EFFIC_STEP"<<SEP<<POLICE_EFFIC_STEP
				  <<SEP<<"POLICE_EFFIC_MIN "<<SEP<<POLICE_EFFIC_MIN
				  <<SEP<<"POLICE_EFFIC_MAX "<<SEP<<POLICE_EFFIC_MAX<<ENDL;
		o<<"FLOAT"<<SEP<<"PROPORTION_STEP"<<SEP<<PROPORTION_STEP
				  <<SEP<<"PROPORTION_MIN "<<SEP<<PROPORTION_MIN
				  <<SEP<<"PROPORTION_MAX "<<SEP<<PROPORTION_MAX<<SEP<<"Batch in ratio mode"<<SEP<<(Compensation_mode?"true":"false")<<ENDL;
		o<<"FLOAT"<<SEP<<"SELECTION_STEP"<<SEP<<SELECTION_STEP//Selection exploration step=0.1;
				  <<SEP<<"SELECTION_MIN "<<SEP<<SELECTION_MIN//Selection exploration minimum=0;
				  <<SEP<<"SELECTION_MAX "<<SEP<<SELECTION_MAX<<ENDL;//Selection exploration maximum=1;
		o<<"uint "<<SEP<<"TYPE OF BATCH JOB:"<<SEP<<batch_sele<<SEP<<batch_names[batch_sele]<<ENDL;
		o<<"FLOAT"<<SEP<<"Default POLICE_EFFIC "<<SEP<<POLICE_EFFIC<<ENDL;	//Z jakim prawdopodobie�stwem wezwana policja obroni agenta
		o<<"FLOAT"<<SEP<<"Default USE_SELECTION"<<SEP<<USED_SELECTION<<ENDL; //Jak bardzo przegrani umieraj� (0 - brak selekcji w og�le)
		o<<"FLOAT"<<SEP<<"Default BULLY_POPUL  "<<SEP<<BULLI_POPUL<<ENDL;
		o<<"FLOAT"<<SEP<<"Default HONOR_POPUL  "<<SEP<<HONOR_POPUL<<ENDL;
		o<<"FLOAT"<<SEP<<"Default CALLP_POPUL  "<<SEP<<CALLER_POPU<<ENDL;
	}
	else
	{
	 o<<"FLOAT"<<SEP<<"POLICE_EFFIC"<<SEP<<POLICE_EFFIC<<ENDL;	//Z jakim prawdopodobie�stwem wezwana policja obroni agenta
	 o<<"FLOAT"<<SEP<<"USE_SELECTION"<<SEP<<USED_SELECTION<<ENDL; //Jak bardzo przegrani umieraj� (0 - brak selekcji w og�le)
	 if(BULLI_POPUL>=1) //...
	  o<<"FLOAT"<<SEP<<"BULLISM_LIMIT"<<SEP<<BULLISM_LIMIT<<ENDL;//Maksymalny mo�liwy bulizm.
	 else
	 {
	  o<<"FLOAT"<<SEP<<"BULLY_POPUL"<<SEP<<BULLI_POPUL<<ENDL;//Albo zero-jedynkowo
	  o<<"FLOAT"<<SEP<<"HONOR_POPUL"<<SEP<<HONOR_POPUL<<ENDL;//Jaka cz�� agent�w populacji jest �ci�le honorowa
	  o<<"FLOAT"<<SEP<<"CALLP_POPUL"<<SEP<<CALLER_POPU<<ENDL;//Jaka cz�� wzywa policje zamiast si� poddawa�
	 }
	 o<<"FLOAT"<<SEP<<"always give up"<<SEP<<(1.0-CALLER_POPU-CALLER_POPU-HONOR_POPUL)<<ENDL;//"loosers"
	 o<<"FLOAT"<<SEP<<"RATIONALITY"<<SEP<<RATIONALITY<<ENDL<<ENDL; //Jak realistycznie ocenia w�asn� si�� (vs. wg. w�asnej reputacji)
	}

	o<<"REPET"<<SEP<<"STOPAFER"<<SEP<<"VISFREQ"<<SEP<<"STATSTART"<<ENDL;
	o<<REPETITION_LIMIT<<SEP<<STOP_AFTER<<SEP<<EveryStep<<SEP<<STAT_AFTER<<ENDL<<ENDL;
	if(FL) o.flush();
}

class HonorAgent// agent tego modelu
{
 public: //WEWN�TRZNE TYPYT POMOCNICZE
	struct LinkTo {unsigned X,Y;};
	enum Decision {NOTHING=-1,WITHDRAW=0,GIVEUP=1,HOOK=2,FIGHT=3,CALLAUTH=4};

 public: //Na razie g��wne w�a�ciwo�ci dost�pne zewn�trznie, potem mo�na pomysle� czy je schowa�
	unsigned ID; //Kolejni agenci
	static unsigned licznik_zyc;//Do tworzeni unikalnych identyfikator�w agent�w
	double Power;//	Si�a (0..1)
	double PowLimit;// Jak� si�� mo�e osi�gn�� maksymalnie, gdy nie traci

	double Agres;// Bulizm (0..1) sk�onno�� do atakowania
	double Honor;// Bezwarunkowa honorowo�� (0..1) sk�onno�� podj�cia obrony

	double CallPolice;//Odium wzywacza policji - prawdopodobie�stwo wzywania policji (0..1) jako �w�drowna �rednia� wezwa�
	//double Resources;// Zasoby (0..inf)

	HonorAgent();  //KONSTRUKTOR

	void RandomReset(); //Losowanie warto�ci atrubut�w

	//Obsluga po��cze�
	bool addNeigh(unsigned x,unsigned y);//Dodaje sasiada o okreslonych wsp�rz�dnych w �wiecie, o ile zmie�ci
	bool getNeigh(unsigned i,unsigned& x,unsigned& y) const;//Czyta wsp�rzedne s�siada, o ile jest
	unsigned NeighSize() const; //Ile ma zarejestrowanych s�siad�w
	void forgetAllNeigh(); //Zapomina wszystkich dodanych s�siad�w, co nie znaczy �e oni zapominaj� jego

	//Funkcje decyzyjne
	Decision  check_partner(wb_dynmatrix<HonorAgent>& World,unsigned& x,unsigned& y);//Wyb�r partnera interakcji
	Decision  answer_if_hooked(wb_dynmatrix<HonorAgent>& World,unsigned x,unsigned y);//Odpowied� na zaczepk�
	void      change_reputation(double delta);//Wzrost lub spadek reputacji z zabezpieczeniem zakresu
	void      lost_power(double delta);  //Spadek, zu�ycie si�y z zabezpieczeniem zera
	static
	bool      firstWin(HonorAgent& In,HonorAgent& Ho);//Ustala czy pierwszy czy drugi agent zwyci�y� w konfrontacji

	//Inne
	double    GetFeiReputation() const { return HonorFeiRep;}
	Decision  LastDecision(bool clean=false); //Ostatnia decyzja z kroku MC do cel�w wizualizacyjnych i statystycznych
	ssh_rgb   GetColor() const {return Color;} //Nie modyfikowalny z zewn�trz indywidualny kolor w�z�a

 private:
	ssh_rgb   Color;	//Indywidualny i niezmienny lub obliczony kt�r�� z funkcji koduj�cych kolor
	Decision  MemOfLastDecision;
	wb_dynarray<LinkTo> Neighbourhood;//Lista wsp�rz�dnych s�siad�w
	unsigned HowManyNeigh; //Liczba posiadanych s�siad�w

	double HonorFeiRep;//Reputacja wojownika jako �w�drowna �rednia� z konfrontacji (0..1)


	const LinkTo* Neigh(unsigned i); //Dost�p do rekordu linku do kolejnego s�siada
};
unsigned HonorAgent::licznik_zyc=0;//Do tworzenia unikalnych identyfikator�w agent�w

ofstream OutLog;
ofstream Dumps;

unsigned long  step_counter=0;
unsigned long  LastStep=-1;//Ostatnie wypisany krok

HonorAgent::HonorAgent():
	   Neighbourhood(MAX_LINKS),HowManyNeigh(0)
	   ,Power(0),PowLimit(0)
	   ,Agres(0),HonorFeiRep(0.5),CallPolice(0.25)
	   ,MemOfLastDecision(HonorAgent::NOTHING),ID(0)
	 //  ,Resources(0),
{
	Color.r=25+RANDOM(220);Color.g=25+RANDOM(220);Color.b=25+RANDOM(220);
}

void HonorAgent::RandomReset()
//Losowanie warto�ci atrubut�w
{
        ID=++licznik_zyc;
		PowLimit=(DRAND()+DRAND()+DRAND()+DRAND()+DRAND()+DRAND())/6;  // Jak� si�� mo�e osi�gn�� maksymalnie, gdy nie traci
		Power=(0.5+DRAND()*0.5)*PowLimit; //	Si�a (0..inf)
		HonorFeiRep=Power;//DRAND()*0.5;  //Reputacja obrony jako �w�drowna �rednia� z konfrontacji
													assert(0<=HonorFeiRep);
													assert(HonorFeiRep<=1);
/*
		if(BULLI_POPUL<1)
		{
		  Agres=(DRAND()<BULLI_POPUL?1:0); //Albo jest albo nie jest
		}
		else
		{
		 if(BULLISM_LIMIT>0)
			Agres=(DRAND()+DRAND()+DRAND()+DRAND()+DRAND()+DRAND())/6*BULLISM_LIMIT; // Bulizm (0..1) sk�onno�� do atakowania,  const FLOAT    BULLISM_LIMIT=1;//Maksymalny mo�liwy bulizm.
			else
			Agres=(DRAND()*DRAND()*DRAND()*DRAND())*fabs(BULLISM_LIMIT);
		}

		Honor=(DRAND()<HONOR_POPUL?1:0); //Albo jest albo nie jest -  Bezwarunkowa honorowo�� (0..1) sk�onno�� podj�cia obrony,  const FLOAT	   HONOR_POPUL=0.9;//Jaka cz�� agent�w populacji jest �ci�le honorowa


		if(Honor<0.99 && Agres<0.99 && DRAND()<CALLER_POPU) //??? Agres XOR HighHonor XOR CallPolice
			CallPolice=1;//Prawdopodobie�stwo wzywania policji (0..1) Mo�e te� jako �w�drowna �rednia� wezwa�, ale na razie nie
			else
			CallPolice=0;
*/
		if(fabs(BULLI_POPUL)<1) //
		{
		   Honor=0;CallPolice=0;//Warto�ci domy�lne dla agresywnych i wzywaj�cych policje
		   Agres=(DRAND()<fabs(BULLI_POPUL)?1:0); //Albo jest albo nie jest
		   if(Agres!=1)
		   {
			 Honor=(DRAND()*(1-fabs(BULLI_POPUL)) < fabs(HONOR_POPUL)?1:0);
			 if(Honor!=1)
			 {
			   CallPolice=(DRAND()*(1-fabs(BULLI_POPUL)-fabs(HONOR_POPUL)) < fabs(CALLER_POPU)?1:0);
			 }
			 else
			 {
			   /* LOOSER */    //cerr<<"*";
			 }
		   }
		}
}

bool      HonorAgent::firstWin(HonorAgent& In,HonorAgent& Ho)
//Ustala czy pierwszy czy drugi agent zwyci�y� w konfrontacji
//Zawsze kt�ry� musi zwyci�y�, co znowu nie jest realistyczne
{                                                           ;
	if( In.Power*(1+DRAND())  >  Ho.Power*(1+DRAND()) )
			return true;
			else
			return false;
}

void      HonorAgent::change_reputation(double delta)
//Wzrost lub spadek reputacji z zabezpieczeniem zakresu
{
													assert(0<=HonorFeiRep);
													assert(HonorFeiRep<=1);
	if(delta>0) //Wzrost
	{
		HonorFeiRep+=(1-HonorFeiRep)*delta;       	assert(HonorFeiRep<=1);
	}
	else
	{
		HonorFeiRep-=HonorFeiRep*fabs(delta);       assert(0<=HonorFeiRep);
													assert(HonorFeiRep<=1);
	}
}

void      HonorAgent::lost_power(double delta)
//Spadek si�y z zabezpieczeniem zakresu
{                                  					assert(Power<=1);
	 delta=fabs(delta);
	 Power*=(1-delta);             					assert(Power>0);
	 //if(Power<0) Power=0;
}

HonorAgent::Decision  HonorAgent::check_partner(wb_dynmatrix<HonorAgent>& World,unsigned& x,unsigned& y)
//Wyb�r partnera interakcji przez agenta, kt�ry dosta� losow� inicjatyw�
{
	this->MemOfLastDecision=WITHDRAW; //DOMY�LNA DECYZJA
	unsigned L=RANDOM(HowManyNeigh);//Ustalenie kt�ry s�siad
	if(getNeigh(L,x,y) ) //Pobranie wsp�rz�dnych s�siada
	{
		HonorAgent& Ag=World[y][x];	//Zapami�tanie referencji do s�siada

		//BARDZO PROSTA REGU�A DECYZYJNA - wygl�da, �e warto atakowa� i jest ch��
		if(this->Agres>0.0 && DRAND()<this->Agres           //Agresywno�� jako
		&& Ag.HonorFeiRep<(RATIONALITY*this->Power+(1-RATIONALITY)*HonorFeiRep)
		) //jako wyrachowanie
		{
			this->MemOfLastDecision=HOOK;//Nieprzypadkowa zaczepka bulliego
		}
		else if(
		//DRAND()<RANDOM_AGRESSION)					//+losowe przypadki burd po pijaku - ZACIEMNIA
		(Honor>0 && DRAND()<Honor*RANDOM_AGRESSION) //A mo�e tylko honorowi maj� niezer� agresj� z powodu nieporozumie�?
		)
		{
			this->MemOfLastDecision=HOOK;//Ewentualna zaczepka losowa lub honorowa
		}
	}
	return this->MemOfLastDecision;
}

HonorAgent::Decision  HonorAgent::answer_if_hooked(wb_dynmatrix<HonorAgent>& World,unsigned x,unsigned y)
//Odpowied� na zaczepk�
{
	 this->MemOfLastDecision=GIVEUP;//DOMY�LNA DECYZJA

	 HonorAgent& Ag=World[y][x];	//Zapami�tanie referencji do s�siada

	 //REGU�A ZALE�NA WYCHOWANIA POLICYJNEGO
	 //LUB OD HONORU i W�ASNEGO POCZUCIA SI�Y
	 if(this->CallPolice>0.999999
	 || (this->CallPolice>0 && DRAND()<this->CallPolice)
	 )
	 {
		this->MemOfLastDecision=CALLAUTH;
	 }
	 else
	 if(this->Honor>0.999999
	 || (this->Honor>0 && DRAND()<this->Honor)
	 || Ag.HonorFeiRep<this->HonorFeiRep
//	 || Ag.HonorFeiRep<this->Power /*HonorFeiRep*/)   //Wersja 2.60
	 )
	 {
		this->MemOfLastDecision=FIGHT;
	 }

	 return this->MemOfLastDecision;
}

void one_step(wb_dynmatrix<HonorAgent>& World)
//G��wna dynamika kontakt�w
{
	unsigned N=(SIDE*SIDE)/2;//Ile losowa� w kroku MC? Po�owa, bo w ka�dej interakcji dwaj
	for(unsigned i=0;i<N;i++)
	{
		unsigned x1=RANDOM(SIDE);
		unsigned y1=RANDOM(SIDE);
		HonorAgent& AgI=World[y1][x1];  //Zapami�tanie referencji do agenta inicjuj�cego
		unsigned x2,y2;
		HonorAgent::Decision Dec1=AgI.check_partner(World,x2,y2);
		if(Dec1==HonorAgent::HOOK) //Je�li zaczepi�
		{                                					assert(AgI.Agres>0 || AgI.Honor>0);
		   AgI.change_reputation(+0.05); // Od razu dostaje powzy�szenie reputacji za samo wyzwanie
		   HonorAgent& AgH=World[y2][x2];//Zapami�tanie referencji do agenta zaczepionego
		   HonorAgent::Decision Dec2=AgH.answer_if_hooked(World,x1,y1);
		   switch(Dec2){
		   case HonorAgent::FIGHT:
					   if(HonorAgent::firstWin(AgI,AgH))
					   {
						  AgI.change_reputation(+0.35);//Zyska� bo wygra�
						  AgI.lost_power(-0.75*DRAND());//Straci� bo walczy�

						  AgH.change_reputation(+0.1); //Zyska� bo stan��
						  AgH.lost_power(-0.95*DRAND());//Straci� bo przegra� walk�
					   }
					   else
					   {
						  AgI.change_reputation(-0.35);//Straci� bo zaczepi� i dosta� b�cki
						  AgI.lost_power(-0.95*DRAND()); //Straci� bo przegra� walk�

						  AgH.change_reputation(+0.75);//Zyska� bo wygra� cho� by� zaczepiony
						  AgH.lost_power(-0.75*DRAND());	//Straci� bo walczy�
					   }
					break;
		   case HonorAgent::GIVEUP:
					   {
						  AgI.change_reputation(+0.5); //Zyska� bo wygra� bez walki. Na pewno wi�cej ni� w walce???

						  AgH.change_reputation(-0.5); //Straci� bo si� podda�
						  AgH.lost_power(-0.5*DRAND()/*DRAND()*/);//I troch� dosta� �omot dla przyk�adu   (!!!)
					   }
					break;
		   case HonorAgent::CALLAUTH:
					  if(DRAND()<POLICE_EFFIC) //Czy przyby�a
					  {
						  AgI.change_reputation(-0.35);//Straci� bo zaczepi� i dosta� b�cki od policji
						  AgI.lost_power(-0.99*DRAND()); //Straci� bo walczy� i przegra� z przewa�aj�c� si��

						  AgH.change_reputation(-0.75);//A zaczepiony straci� bo wezwa� policje zamiast walczy�
					  }
					  else //A mo�e nie
					  {
						  AgI.change_reputation(+0.5);//Zyska� bo wygra� bez walki. A� tyle?

						  AgH.change_reputation(-0.75);//Zaczepiony straci� bo wezwa� policje
						  AgH.lost_power(-0.99*DRAND()); //Straci� bo si� nie broni�, a wkurzy� wzywaj�c
					  }
					break;
		   //Odpowiedzi na zaczepk�, kt�re si� nie powinny zdarza�
		   case HonorAgent::WITHDRAW:/* TU NIE MO�E BY� */
		   case HonorAgent::HOOK:	 /* TU NIE MO�E BY� */
		   default:                  /* TU JU� NIE MO�E BY�*/
				  cout<<"?";  //Podejrzane - nie powinno si� zdarza�
		   break;
		   }
		}
		else  //if WITHDRAW  //Jak si� wycofa� z zaczepiania?
		{
		  	AgI.change_reputation(-0.0001);//Minimalnie traci w swoich oczach
		}
	}

	step_counter++;
}

//typedef ssh_rgb CalculateColorFunction(bitarray&); //Typ funkcji obliczaj�cej kolor dla wirusa i hosta
//CalculateColorFunction* CalculateColorDefault=NULL; //Wska�nik do domy�lnej funkcji koloruj�cej
	//Po��czenia z najbli�szymi s�siadami
void DeleteAllConnections(wb_dynmatrix<HonorAgent>& World)
{
	for(int x=0;x<SIDE;x++)
		for(int y=0;y<SIDE;y++)
		{
		   HonorAgent& Ag=World[y][x];	//Zapami�tanie referencji do agenta, �eby ci�gle nie liczy� indeks�w
		   Ag.forgetAllNeigh(); //Bezwarunkowe zapomnienie
		}
}

void InitConnections(wb_dynmatrix<HonorAgent>& World,FLOAT HowManyFar)
{
	//Po��czenia z najbli�szymi s�siadami
	for(int x=0;x<SIDE;x++)
		for(int y=0;y<SIDE;y++)
		{
		   HonorAgent& Ag=World[y][x];	//Zapami�tanie referencji do agenta, �eby ci�gle nie liczy� indeks�w
		   for(int xx=x-MOORE_RAD;xx<=x+MOORE_RAD;xx++)
			  for(int yy=y-MOORE_RAD;yy<=y+MOORE_RAD;yy++)
			  if(!(xx==x && yy==y))//Wyci�cie samego siebie
			  {
				 if(TORUS)
				   Ag.addNeigh((xx+SIDE)%SIDE,(yy+SIDE)%SIDE);//Zamkni�te w torus
				   else
				   if(0<=xx && xx<SIDE && 0<=yy && yy<SIDE)
					 Ag.addNeigh(xx,yy);//bez bok�w
			  }
		}

	//Dalekie po��czenia
	for(int f=0;f<HowManyFar;f++)
	{
		unsigned x1,y1,x2,y2;
		//POszukanie agent�w z wolnymi slotami
		do{
		x1=RANDOM(SIDE);
		y1=RANDOM(SIDE);
		}while(World[y1][x1].NeighSize()>=MAX_LINKS);
		do{
		x2=RANDOM(SIDE);
		y2=RANDOM(SIDE);
		}while(World[y2][x2].NeighSize()>=MAX_LINKS);
		//Po��czenie ich linkami w obie strony
		World[y1][x1].addNeigh(x2,y2);
		World[y2][x2].addNeigh(x1,y1);
	}
}

void InitAtributes(wb_dynmatrix<HonorAgent>& World,FLOAT HowMany)
{
	for(int y=0;y<SIDE;y++)
	  for(int x=0;x<SIDE;x++)
		World[y][x].RandomReset();
}

/*
void social_impact_step(wb_dynmatrix<HonorAgent>& World,double percent_of_MC=100)
//Powolna dynamika wp�ywu spo�ecznego - w losowej pr�bce s�siedztwa Moora
{

	unsigned N=(SIDE*SIDE*percent_of_MC)/100;//Ile losowa� w kroku MC
	for(unsigned i=0;i<N;i++)
	{
		int v1=RANDOM(SIDE);
		int h1=RANDOM(SIDE);
		HonorAgent& Ag=World[v1][h1];	//Zapami�tanie referencji do agenta, �eby ci�gle nie liczy� indeks�w
		//...
	}
}
*/

unsigned LiczbaTrupow=0;
unsigned LiczbaTrupowDzis=0;
void power_recovery_step(wb_dynmatrix<HonorAgent>& World)
{
  LiczbaTrupowDzis=0;
  for(int x=0;x<SIDE;x++)
	for(int y=0;y<SIDE;y++)
	{
		HonorAgent& Ag=World[y][x];	//Zapami�tanie referencji do agenta, �eby ci�gle nie liczy� indeks�w

		if(USED_SELECTION>0 && Ag.Power<USED_SELECTION)  //Chyba nie przezy�
		{
		   if(population_growth==0) //Tryb z prawdopodobienstwami inicjalnymi
		   {
			Ag.RandomReset();
		   }else
		   if(population_growth==1) //Tryb z losowym s�siadem
		   {
			unsigned ktory=RANDOM(Ag.NeighSize()),xx,yy;
			bool pom=Ag.getNeigh(ktory,xx,yy);
			HonorAgent& Drugi=World[yy][xx];
			Ag.RandomReset();
			Ag.Agres=Drugi.Agres;
			Ag.Honor=Drugi.Honor;
			Ag.CallPolice=Drugi.CallPolice;
		   }
		   else
		   if(population_growth==3) //Tryb z losowym cz�onkiem populacji
		   {
			unsigned xx=RANDOM(SIDE),yy=RANDOM(SIDE);
			HonorAgent& Drugi=World[yy][xx];
			Ag.RandomReset();
			Ag.Agres=Drugi.Agres;
			Ag.Honor=Drugi.Honor;
			Ag.CallPolice=Drugi.CallPolice;
		   }
		   //	Ag.Power=0.1*Ag.PowLimit;
		   LiczbaTrupow++;
		   LiczbaTrupowDzis++;
		}
		else
		if(Ag.Power<Ag.PowLimit)//Mo�e si� leczy� lub "poprawia�"
		{
			Ag.Power+=(Ag.PowLimit-Ag.Power)*RECOVERY_POWER;
		}
	}

  //Losowe �mierci i przypadkowe narodzenia
  if(NOISE_KILL>0)
  {
	unsigned WypadkiLosow=SIDE*NOISE_KILL*SIDE;
	for(;WypadkiLosow>0;WypadkiLosow--)
	{
	  unsigned x=RANDOM(SIDE),y=RANDOM(SIDE);
	  HonorAgent& Ag=World[y][x];	//Zapami�tanie referencji
	  Ag.RandomReset();  //Wg. inicjalnej dystrybucji - mo�e przywracac do istnienia ju� wymar�e strategie
	}
  }
}

//  Liczenie statystyk
///////////////////////////////////////////////////////////////////////////////
double MeanFeiReputation=0;
double MeanCallPolice=0;
double MeanPower=0;
double MeanAgres=0;
double MeanHonor=0;

struct zliczacz
{
  double summ;
  unsigned N;
  void Reset(){summ=0;N=0;}
  zliczacz():summ(0),N(0){}
  static void Reset(zliczacz t[],unsigned N)
  {
	  for(unsigned i=0;i<N;i++)
		t[i].Reset();
  }
};

const int NumOfCounters=4; //I RACZEJ TYLE POWINNO ZOPSTA� ZE WZGL�DU NA WIZUALIZACJE ITP
zliczacz  MnStrenght[NumOfCounters];//Liczniki sily skrajnych typ�w agent�w
const char* MnStrNam[NumOfCounters]={"MnAgresPw","MnHonorPw","MnPolicPw","MnOthrPwr"};

union {
struct { unsigned NOTHING,WITHDRAW,GIVEUP,HOOK,FIGHT,CALLAUTH;};
		 unsigned Tab[6];
void Reset(){NOTHING=WITHDRAW=GIVEUP=HOOK=FIGHT=CALLAUTH=0;}
} Actions;

void CalculateStatistics(wb_dynmatrix<HonorAgent>& World)
{
	unsigned N=SIDE*SIDE;
	double ForFeiRep=0;
	double ForCallRe=0;
	double ForPower=0;
	double ForAgres=0;
	double ForHonor=0;
	Actions.Reset();
	zliczacz::Reset(MnStrenght,NumOfCounters);

	for(unsigned v=0;v<SIDE;v++)
	{
		for(unsigned h=0;h<SIDE;h++)
		{
			HonorAgent& Ag=World[v][h];			//Zapami�tanie referencji do agenta, �eby ci�gle nie liczy� indeks�w

			ForFeiRep+=Ag.GetFeiReputation();
			ForCallRe+=Ag.CallPolice;
			ForPower+=Ag.Power;
			ForAgres+=Ag.Agres;
			ForHonor+=Ag.Honor;

			if(0.9<Ag.Agres) //AGRESYWNI (BULLY) - Tylko najwy�sze warto�ci
			{
			   MnStrenght[0].summ+=Ag.Power;MnStrenght[0].N++;
			}
			else
			if(0.9<Ag.Honor) //Jak nie, to jako HONORowi
			{
			   MnStrenght[1].summ+=Ag.Power;MnStrenght[1].N++;
			}
			else
			if(0.9<Ag.CallPolice) //Jak nie to mo�e zawsze wzywaj�cy POLICEje
			{
				MnStrenght[2].summ+=Ag.Power;MnStrenght[2].N++;
			}
			else //W ostateczno�ci zwykli luserzy
			{
             	MnStrenght[3].summ+=Ag.Power;MnStrenght[3].N++;
			}

			switch(Ag.LastDecision(false)){
			case HonorAgent::WITHDRAW: Actions.WITHDRAW++;  break;
			case HonorAgent::GIVEUP:   Actions.GIVEUP++;  break;
			case HonorAgent::HOOK:     Actions.HOOK++; break;
			case HonorAgent::FIGHT:    Actions.FIGHT++; break;
			case HonorAgent::CALLAUTH: Actions.CALLAUTH++;break;
			default:
								Actions.NOTHING++;	 break;
			}
		}
	}

	MeanFeiReputation=ForFeiRep/N;
	MeanCallPolice=ForCallRe/N;
	MeanPower=ForPower/N;
	MeanAgres=ForAgres/N;
	MeanHonor=ForHonor/N;
}

void save_stat()
{
	if(step_counter==0 && LastStep!=0 && RepetNum==1 )//Tylko za pierwszym razem
	{
		LastStep=0;
		Parameters_dump(OutLog);
		if(REPETITION_LIMIT>1)//Gdy ma zrobi� wi�cej powt�rze� tego samego eksperymentu
			OutLog<<"REPET N#"<<'\t'; //Kt�ra to kolejna repetycja?

		OutLog<<"MC_STEP"   <<'\t'<<"MeanAgres"<<'\t'<<"MeanHonor"<<'\t'<<"MeanCallPolice"<<'\t'
			  <<"MeanFeiReputation"<<'\t'<<"MeanPower"<<'\t'<<"All killed"
			  <<'\t'<<"NOTHING"<<'\t'<<"WITHDRAW"<<'\t'<<"GIVEUP"<<'\t'<<"HOOK"<<'\t'<<"FIGHT"<<'\t'<<"CALLAUTH";
		//MnStrenght&MnStrNam[NumOfCounters]
		for(unsigned i=0;i<NumOfCounters;i++)
			OutLog<<'\t'<<MnStrNam[i];
		for(unsigned i=0;i<NumOfCounters;i++)
			OutLog<<"\tNof"<<MnStrNam[i];
		OutLog<<endl;

		if(REPETITION_LIMIT>1)//Gdy ma zrobi� wi�cej powt�rze� tego samego eksperymentu
			OutLog<<RepetNum<<'\t'; //Kt�ra to kolejna repetycja?

		OutLog<<"0.9"<<'\t'<<MeanAgres<<'\t'<<MeanHonor<<'\t'<<MeanCallPolice<<'\t'
			  <<MeanFeiReputation<<'\t'<<MeanPower<<'\t'<<LiczbaTrupow<<'\t'
			  <<Actions.NOTHING<<'\t'<<Actions.WITHDRAW<<'\t'<<Actions.GIVEUP<<'\t'<<Actions.HOOK<<'\t'<<Actions.FIGHT<<'\t'<<Actions.CALLAUTH;
		//MnStrenght&MnStrNam[NumOfCounters]
		for(unsigned i=0;i<NumOfCounters;i++)
			OutLog<<'\t'<<(MnStrenght[i].N>0?MnStrenght[i].summ/MnStrenght[i].N:-9999);
		for(unsigned i=0;i<NumOfCounters;i++)
			OutLog<<'\t'<<MnStrenght[i].N;
		OutLog<<endl; //KONIEC LINII LOGU

		if(ConsoleLog)
		{
		  cout<<"STEP"<<setw(4)<<'\t'<<"MnAgres"<<'\t'<<"MnHonor"<<'\t'<<"MnFeiRep"<<'\t'<<"MnCallRep"<<'\t'<<"MnPower";
		  for(unsigned i=0;i<NumOfCounters;i++) cout<<'\t'<<MnStrNam[i];
		  cout<<endl;
		}
		//MnStrenght&MnStrNam[NumOfCounters]
	}
	else
	if(LastStep!=step_counter)
	{
		if(REPETITION_LIMIT>1)//Gdy ma zrobi� wi�cej powt�rze� tego samego eksperymentu
			OutLog<<RepetNum<<'\t'; //Kt�ra to kolejna repetycja?

		OutLog<<(step_counter>0?step_counter:0.1)<<'\t'<<MeanAgres<<'\t'<<MeanHonor<<'\t'<<MeanCallPolice<<'\t'<<MeanFeiReputation<<'\t'<<MeanPower<<'\t'<<LiczbaTrupow<<'\t'<<Actions.NOTHING<<'\t'<<Actions.WITHDRAW<<'\t'<<Actions.GIVEUP<<'\t'<<Actions.HOOK<<'\t'<<Actions.FIGHT<<'\t'<<Actions.CALLAUTH;
		//MnStrenght&MnStrNam[NumOfCounters]
		for(unsigned i=0;i<NumOfCounters;i++)
			OutLog<<'\t'<<(MnStrenght[i].N>0?MnStrenght[i].summ/MnStrenght[i].N:-9999);
		for(unsigned i=0;i<NumOfCounters;i++)
			OutLog<<'\t'<<MnStrenght[i].N;
		OutLog<<endl;

		if(ConsoleLog)
		{
		  cout<<step_counter<<setw(4)<<setprecision(3)<<'\t'<<MeanAgres<<'\t'<<MeanHonor<<'\t'<<MeanFeiReputation<<'\t'<<MeanCallPolice<<'\t'<<MeanPower;
		  for(unsigned i=0;i<NumOfCounters;i++)
			cout<<'\t'<<setw(4)<<(MnStrenght[i].N>0?MnStrenght[i].summ/MnStrenght[i].N:-1);
		  cout<<endl;
		}

		LastStep=step_counter;//Zapisuje �eby nie wypisywa� podw�jnie do logu
	}
}

void Reset_action_memories(wb_dynmatrix<HonorAgent>& World)
{
	for(unsigned v=0;v<SIDE;v++)
	{
		for(unsigned h=0;h<SIDE;h++)
		{
			World[v][h].LastDecision(true);			//Zapami�tanie referencji do agenta, �eby ci�gle nie liczy� indeks�w
		}
	}
}

void dump_step(wb_dynmatrix<HonorAgent>& World,unsigned step)
{
	const char TAB='\t';
	if(RepetNum==1 && step==0) //W kroku zerowym metryczka i nag��wek
	{
	   Parameters_dump(Dumps);
	   Dumps<<"Repet"<<TAB<<"step"<<TAB<<"v"<<TAB<<"h"<<TAB;
	   Dumps<<"Ag_ID"<<TAB<<"Power"<<TAB<<"PowLimit"<<TAB
				 <<"FeiReput"<<TAB
				 <<"Agresion"<<TAB
				 <<"Honor"<<TAB
				 <<"CallPolice"<<TAB
				 <<"LastDec"<<TAB
				 <<"NeighSize"<<endl;
	}
	for(unsigned v=0;v<SIDE;v++)
	{
		for(unsigned h=0;h<SIDE;h++)
		{
			Dumps<<RepetNum<<TAB<<step<<TAB<<v<<TAB<<h<<TAB;
			HonorAgent& Ag=World[v][h];	//Zapami�tanie referencji
			Dumps<<Ag.ID<<TAB<<Ag.Power<<TAB<<Ag.PowLimit<<TAB
				 <<Ag.GetFeiReputation()<<TAB
				 <<Ag.Agres<<TAB
				 <<Ag.Honor<<TAB
				 <<Ag.CallPolice<<TAB
				 <<Ag.LastDecision(false)<<TAB
				 <<Ag.NeighSize()<<endl;
		}
	}
	Dumps<<endl;//Po ca�o�ci
}

// WIZUALIZACJA
/////////////////////////////////////////////////////////////////////////////

void replot(wb_dynmatrix<HonorAgent>& World)
{
	int old=mouse_activity(0);
	//clear_screen();
	unsigned spw=screen_width()-SIDE*SSIZ;
	unsigned StartDyn=(SIDE+1)*SSIZ+char_height('X')+1;//Gdzie si� zaczyna wizualizacja pomocnicza
	unsigned StartPow=StartDyn+(SIDE+1)*SSIZ+char_height('X')+1;//Gdzie si� zaczyna wizualizacja aktywno�ci
	unsigned StartPro=StartPow+(SIDE+1)*SSIZ+char_height('X')+1;

	//double RealMaxReputation=0;
	//double SummReputation=0;
	//bool  VisShorLinks=false; //Wizualizacja bliskich link�w
	//bool  VisFarLinks=true;	//Wizualizacja dalekich
	//bool  VisAgents=true;     //Wizualizacja w�a�ciwo�ci agent�w

	//DRUKOWANIE POWI�ZA�
	int VSIZ2=VSIZ/2;

	if(VisFarLinks || VisShorLinks)
	for(unsigned n=0;n<SIDE*SIDE;n++)
	{
		unsigned v=RANDOM(SIDE);
		unsigned h=RANDOM(SIDE);

		HonorAgent& Ag=World[v][h];			//Zapami�tanie referencji do agenta, �eby ci�gle nie liczy� indeks�w
		unsigned NofL=Ag.NeighSize();
		ssh_rgb c=Ag.GetColor();

		for(unsigned x,y,l=0;l<NofL;l++)
		 if(Ag.getNeigh(l,x,y))
		 {
			if(l<MOORE_SIZE && VisShorLinks)
			{
			set_pen_rgb(c.r,c.g,c.b,0,SSH_LINE_SOLID);// Ustala aktualny kolor linii za pomoca skladowych RGB
			line_d(h*VSIZ+VSIZ2,v*VSIZ+VSIZ2,x*VSIZ+VSIZ2,y*VSIZ+VSIZ2);
			}
			if(l>=MOORE_SIZE && VisFarLinks)
			{
			set_pen_rgb(c.r,c.g,c.b,1,SSH_LINE_SOLID);// Dla dalekich nieco grubsze
			line_d(h*VSIZ+VSIZ2,v*VSIZ+VSIZ2,x*VSIZ+VSIZ2,y*VSIZ+VSIZ2);
			}
		 }
	}

	set_pen_rgb(75,75,75,0,SSH_LINE_SOLID); // Ustala aktualny kolor linii za pomoca skladowych RGB
	//DRUKOWANIE DANYCH DLA W�Z��W
	for(unsigned v=0;v<SIDE;v++)
	{
		for(unsigned h=0;h<SIDE;h++)
		{

			HonorAgent& Ag=World[v][h];			//Zapami�tanie referencji do agenta, �eby ci�gle nie liczy� indeks�w
			ssh_rgb Color=Ag.GetColor();

			if(VisAgents)
			{
				if(!(VisFarLinks || VisShorLinks))
					fill_rect(h*VSIZ,v*VSIZ,h*VSIZ+VSIZ,v*VSIZ+VSIZ,255+128);

				//set_brush_rgb(Ag.GetFeiReputation()*255,0,Ag.CallPolice*255);
				//set_pen_rgb(  Ag.GetFeiReputation()*255,0,Ag.CallPolice*255,1,SSH_LINE_SOLID);
				set_brush_rgb(Ag.Agres*255,Ag.Honor*255,Ag.CallPolice*255);
				set_pen_rgb(Ag.GetFeiReputation()*255,Ag.GetFeiReputation()*255,0,1,SSH_LINE_SOLID);
				unsigned ASiz=1+/*sqrt*/(Ag.Power)*VSIZ;
				fill_rect_d(h*VSIZ,v*VSIZ,h*VSIZ+ASiz,v*VSIZ+ASiz);
				fill_rect_d(spw+h*SSIZ,StartPro+v*SSIZ,spw+(h+1)*SSIZ,StartPro+(v+1)*SSIZ);

				line_d(h*VSIZ,v*VSIZ+ASiz,h*VSIZ+ASiz,v*VSIZ+ASiz);
				line_d(h*VSIZ+ASiz,v*VSIZ,h*VSIZ+ASiz,v*VSIZ+ASiz);
				//if(Ag.Agres>0 || Ag.Honor>0)
				//	plot_rgb(h*VSIZ+ASiz/2,v*VSIZ+ASiz/2,Ag.Agres*255,Ag.Agres*255,Ag.Honor*255);
				//set_brush_rgb(Ag.Power*Ag.Agres*255,Ag.Power*Ag.Honor*255,0);//Wzpenienie jako kombinacje si�y i sk�onno�ci
				//fill_circle_d(h*VSIZ+VSIZ2,v*VSIZ+VSIZ2,);   // G��wny rysunek HonorAgenta ...

			}

			//Wizualizacja "Reputacji" w odcieniach szaro�ci?
			set_pen_rgb(  Ag.GetFeiReputation()*255,0,Ag.CallPolice*255,1,SSH_LINE_SOLID);
			set_brush_rgb(Ag.GetFeiReputation()*255,0,Ag.CallPolice*255);
			fill_rect_d(spw+h*SSIZ,v*SSIZ,spw+(h+1)*SSIZ,(v+1)*SSIZ);

			//Wizualizacja si�y w odcieniach szaro�ci
			ssh_color ColorPow=257+unsigned(Ag.Power*254); //
			fill_rect(spw+h*SSIZ,StartPow+v*SSIZ,spw+(h+1)*SSIZ,StartPow+(v+1)*SSIZ,ColorPow);

			//Dynamika proces�w - punkty w r�nych kolorach co si� dzie�o w ostatnim kroku
			ssh_color ColorDyn=0;
			switch(Ag.LastDecision(false)){
			case HonorAgent::WITHDRAW:
								ColorDyn=220;  break;
			case HonorAgent::GIVEUP:
								ColorDyn=190;  break;
			case HonorAgent::HOOK:
								ColorDyn=254;  break;
			case HonorAgent::FIGHT:
								ColorDyn=50;  break;
			case HonorAgent::CALLAUTH:
								ColorDyn=128;  break;
			default:
								ColorDyn=0;	 break;
			}
			fill_rect(spw+h*SSIZ,StartDyn+v*SSIZ,spw+(h+1)*SSIZ,StartDyn+(v+1)*SSIZ,ColorDyn);
			unsigned ASiz=1+/*sqrt*/(Ag.Power)*VSIZ;
			plot(h*VSIZ+ASiz/2,v*VSIZ+ASiz/2,ColorDyn);
		}
	}

	printc(1,screen_height()-char_height('X'),100,255,"%s MnAgres=%f  MnHonor=%f  MnPoli=%f Killed=%u  AllKilled=%u  ",
										"COMPONENT VIEW",
									double(MeanAgres),double(MeanHonor),double(MeanCallPolice),LiczbaTrupowDzis,LiczbaTrupow);
	/*double MeanFeiReputation=0;
double MeanCallPolice=0;
double MeanPower=0;*/
	printc(spw,(SIDE+1)*SSIZ,150,255,        "%s Mn.Fe=%g Mn.Cp=%g    ","Reput.",double(MeanFeiReputation),double(MeanCallPolice));   // HonorAgent::MaxReputation
	printc(spw,StartPow+(SIDE+1)*SSIZ,50,255,"%s mn=%f                ","Power",double( MeanPower ));
	printc(spw,StartDyn+(SIDE+1)*SSIZ,50,255,"%s H:%u F:%u C:%u       ","Local interactions",Actions.HOOK,Actions.FIGHT,Actions.CALLAUTH);
	printc(spw,StartPro+(SIDE+1)*SSIZ,50,255,"%s A:%u H:%u P:%u O:%u      ","Counters",MnStrenght[0].N,MnStrenght[1].N,MnStrenght[2].N,MnStrenght[3].N);
	printc(spw,StartPro+(SIDE+1)*SSIZ+char_height('X'),128,255,"%u MC ",step_counter);
	//HonorAgent::Max...=Real...;//Aktualizacja max-�w do policzonych przed chwil� realnych
	flush_plot();

	save_stat();

	mouse_activity(old);
}

//Pomocnicze metody agent�w itp
//////////////////////////////////
HonorAgent::Decision HonorAgent::LastDecision(bool clean)
//Ostatnia decyzja z kroku MC do cel�w wizualizacyjnych i statystycznych
{
	Decision Tmp=MemOfLastDecision;
	if(clean) MemOfLastDecision=NOTHING;
	return Tmp;
}

unsigned HonorAgent::NeighSize()const
//Ile ma zarejestrowanych s�siad�w
{
	return HowManyNeigh;
}

void HonorAgent::forgetAllNeigh()
//Zapomina wszystkich dodanych s�siad�w, co nie znaczy �e oni zapominaj� jego
{
   HowManyNeigh=0;
}

bool HonorAgent::getNeigh(unsigned i,unsigned& x,unsigned& y) const
//Czyta wsp�rzedne s�siada, o ile jest
{
  if(i<HowManyNeigh) //Jest taki
   {
	x=Neighbourhood[i].X;
	y=Neighbourhood[i].Y;
	return true;
   }
   else return false;
}

const HonorAgent::LinkTo* HonorAgent::Neigh(unsigned i)//Wsp�rz�dne kolejnego s�siada
{
  if(i<HowManyNeigh)
		return &Neighbourhood[i];
		else return NULL;
}

bool HonorAgent::addNeigh(unsigned x,unsigned y)
//Dodaje sasiada o okreslonych wsp�rz�dnych w �wiecie
{
   if(HowManyNeigh<Neighbourhood.get_size()) //Jest jeszcze miejsce
   {
	Neighbourhood[HowManyNeigh].X=x;
	Neighbourhood[HowManyNeigh].Y=y;
	HowManyNeigh++;
	return true;
   }
   else return false;
}


/*  OGOLNA FUNKCJA MAIN I TO CO JEJ POTRZEBNE */
/**********************************************/
void Help();
void SaveScreen(unsigned step);
void mouse_check(wb_dynmatrix<HonorAgent>& World);
void fixed_params_mode();//Tryb interakcyjny z pe�n� wizualizacj�
//Tryby analizy przestrzeni parametr�w
void walk_params_sele(); //--> selekcja vs efektywno�� policji
void walk_params_prop(); //--> honorowi/policyjni vs efektywno�� policji
void walk_honor_vs_agrr();//--> honorowi/agresywni vs selekcja

int main(int argc,const char* argv[])
{
	cout<<MODELNAME<<" v.:"<<VERSIONNUM<<endl<<
		"==============================="<<endl<<
  //		"(programmed by Wojciech Borkowski from University of Warsaw)\n"
		"        "<<endl
		<<endl;
	cout<<"Use -help for graphics setup information,\nor HELP for information about available parameters."<<endl;
	if(OptionalParameterBase::parse_options(argc,argv,Parameters,sizeof(Parameters)/sizeof(Parameters[0])))
			return 1;

	mouse_activity(1);
	set_background(255);
	buffering_setup(1);// Czy wlaczona animacja
	if(batch_mode) fix_size(0);
		else fix_size(1);
	shell_setup(MODELNAME,argc,argv);

	cout<<"\n MODEL CONFIGURATION: "<<endl;

	Parameters_dump(cout);

	if(!init_plot(SIDE*VSIZ+20+SIDE*SSIZ,SIDE*VSIZ,0,1)) //Na g��wn� wizualizacj� swiata i jakie� boki
	{
		cerr<<"Can't initialize graphics"<<endl;
		exit(-1);
	}

	string tmp=LogName; tmp+=".log";
	OutLog.open(tmp.c_str());//"Honor.log";
	tmp=DumpNam; tmp+=".txt";
	if(!OutLog.is_open()) { perror(tmp.c_str());exit(-2);}
	Dumps.open(tmp.c_str()); //"Honor_dump.txt";
	if(!Dumps.is_open()) { perror(tmp.c_str());exit(-3);}

	RANDOMIZE();//Musi by� chyba �e chce si� powtarzalnie debugowa�!

	if(batch_mode)//Tryb analizy przestrzeni parametr�w
	{
		switch(batch_sele){ //Czy tryb przeszukiwania szuka po proporcjach czy po sile selekcji?
		case BAT_SELECTION:walk_params_sele();break;//=1,
		case BAT_HONORvsCPOLL:walk_params_prop();break;//=2,
		case BAT_HONORvsAGRR:walk_honor_vs_agrr();break;//=3
		case NO_BAT://=0
		default:
			cerr<<"\n\aInvalid batch mode!\n"<<endl;
			exit(-111);
		}
	}
	else
		fixed_params_mode();//Tryb interakcyjny z pe�n� wizualizacj�

	cout<<endl<<"Bye, bye!"<<endl;
	close_plot();
	return 0;
}

void PlotTables(const char* Name1,wb_dynmatrix<FLOAT>& Tab1,
				const char* Name2,wb_dynmatrix<FLOAT>& Tab2,
				const char* Name3,wb_dynmatrix<FLOAT>& Tab3,
				const char* Name4,wb_dynmatrix<FLOAT>& Tab4,bool true_color=false) //enum  {NO_BAT=0,BAT_SELECTION=1,BAT_HONORvsCPOLL=2,BAT_HONORvsAGRR=3} batch_sele;//Czy tryb przeszukiwania szuka po proporcjach czy po sile selekcji?
{
	 unsigned W=screen_width()/2;
	 unsigned Ws=(W-2)/Tab1[0].get_size();
	 unsigned H=(screen_height()-char_height('X'))/2;
	 unsigned Hs=(H-1-char_height('X'))/Tab1.get_size();
	 unsigned H2=Tab1.get_size()*Hs+char_height('X')+1;

	 if(true_color)
	 {
		for(unsigned i=0;i<256;i++)
		{
			set_pen_rgb(i,i,i,2,SSH_LINE_SOLID);
			line_d(screen_width()-10,screen_height()-i,screen_width(),screen_height()-i);
		}
	 }
	 else
	 {
		 for(unsigned i=0;i<256;i++)
			line(screen_width()-10,screen_height()-i,screen_width(),screen_height()-i,i);
     }

	 for(unsigned Y=0;Y<Tab1.get_size();Y++)
	  for(unsigned X=0;X<Tab1[Y].get_size();X++)
	  {
		  int col1=(Tab1[Y][X]>=0?Tab1[Y][X]*255:-128);
		  int col2=(Tab2[Y][X]>=0?Tab2[Y][X]*255:-128);
		  int col3=(Tab3[Y][X]>=0?Tab3[Y][X]*255:-128);
		  int col4=(Tab4[Y][X]>=0?Tab4[Y][X]*255:-128);

		  if(true_color)
		  {
			if(col1>=0){set_brush_rgb(col1,col1,0);
			fill_rect_d(X*Ws,Y*Hs,(X+1)*Ws,(Y+1)*Hs); }

			if(col2>=0){set_brush_rgb(col2,0,col2);
			fill_rect_d(W+X*Ws,Y*Hs,W+(X+1)*Ws,(Y+1)*Hs);}

			if(col3>=0){set_brush_rgb(0,col3,col3);
			fill_rect_d(X*Ws,H2+Y*Hs,(X+1)*Ws,H2+(Y+1)*Hs);}

			if(col4>=0){set_brush_rgb(0,col4,0);
			fill_rect_d(W+X*Ws,H2+Y*Hs,W+(X+1)*Ws,H2+(Y+1)*Hs);}
		  }
		  else
		  {
			if(col1>=0)fill_rect(X*Ws,Y*Hs,(X+1)*Ws,(Y+1)*Hs,col1);
			if(col2>=0)fill_rect(W+X*Ws,Y*Hs,W+(X+1)*Ws,(Y+1)*Hs,col2);
			if(col3>=0)fill_rect(X*Ws,H2+Y*Hs,(X+1)*Ws,H2+(Y+1)*Hs,col3);
			if(col4>=0)fill_rect(W+X*Ws,H2+Y*Hs,W+(X+1)*Ws,H2+(Y+1)*Hs,col4);
          }
	  }

	 print_transparently(1);
	 printbw(0,Tab1.get_size()*Hs,"%s",Name1);
	 printbw(W,Tab1.get_size()*Hs,"%s",Name2);
	 printbw(0,2*H2-char_height('X'),"%s",Name3);
	 printbw(W,2*H2-char_height('X'),"%s",Name4);

	 print_transparently(0);
	 switch(batch_sele){ //Czy tryb przeszukiwania szuka po proporcjach czy po sile selekcji?
		case BAT_SELECTION:printc(0,screen_height()-char_height('X'),55,255,
			"POL.EFF:%g-%g:%g SEL.:%g-%g:%g (last: B=%g H=%g C=%g) STEPS:%u NREP:%u STATSTART:%u  ",
						POLICE_EFFIC_MIN,POLICE_EFFIC_MAX,POLICE_EFFIC_STEP,
						SELECTION_MIN,SELECTION_MAX,SELECTION_STEP,
						BULLI_POPUL,HONOR_POPUL,CALLER_POPU,
						STOP_AFTER,
						REPETITION_LIMIT,
						STAT_AFTER
						);
				break;//=1,        walk_params_sele();
		case BAT_HONORvsCPOLL:printc(0,screen_height()-char_height('X'),55,255,
			"POL.EFF:%g-%g:%g PROP:%g-%g:%g (last: B=%g H=%g C=%g Sel=%g) STEPS:%u NREP:%u STATSTART:%u  ",
						POLICE_EFFIC_MIN,POLICE_EFFIC_MAX,POLICE_EFFIC_STEP,
						PROPORTION_MIN,PROPORTION_MAX,PROPORTION_STEP,
						BULLI_POPUL,HONOR_POPUL,CALLER_POPU,
						USED_SELECTION,
						STOP_AFTER,
						REPETITION_LIMIT,
						STAT_AFTER
						);
				break;//=2,    walk_params_prop();
		case BAT_HONORvsAGRR:printc(0,screen_height()-char_height('X'),55,255,
			"PROP:%g-%g:%g SEL.:%g-%g:%g (last: B=%g H=%g C=%g Sel=%g) STEPS:%u NREP:%u STATSTART:%u  ",
						PROPORTION_MIN,PROPORTION_MAX,PROPORTION_STEP,
						SELECTION_MIN,SELECTION_MAX,SELECTION_STEP,
						BULLI_POPUL,HONOR_POPUL,CALLER_POPU,
						USED_SELECTION,
						STOP_AFTER,
						REPETITION_LIMIT,
						STAT_AFTER
						);
				break;//=3    walk_honor_vs_agrr();
		case NO_BAT://=0
		default:
			cerr<<"\n\aInvalid batch mode!\n"<<endl;
			exit(-111);
		}
	 flush_plot();
}

void Write_tables(ostream o,const char* Name1,wb_dynmatrix<FLOAT>& Tab1,
							const char* Name2,wb_dynmatrix<FLOAT>& Tab2,const char* TAB="\t")//enum  {NO_BAT=0,BAT_SELECTION=1,BAT_HONORvsCPOLL=2,BAT_HONORvsAGRR=3} batch_sele;//Czy tryb przeszukiwania szuka po proporcjach czy po sile selekcji?
{
	unsigned X=0,Y=0;
	wb_pchar pom(128);
	o<<endl;
	o<<TAB;

	//NAG��WKI TABEL
	switch(batch_sele){ //Czy tryb przeszukiwania szuka po proporcjach czy po sile selekcji?
	case BAT_HONORvsCPOLL:
	case BAT_SELECTION:X=0;
	for(FLOAT effic=POLICE_EFFIC_MIN;effic<=POLICE_EFFIC_MAX;effic+=POLICE_EFFIC_STEP,X++)
	{
		pom.prn("P.Ef%g",effic);
		o<<pom.get()<<TAB;
	}
	o<<TAB<<TAB;X=0;
	for(FLOAT effic=POLICE_EFFIC_MIN;effic<=POLICE_EFFIC_MAX;effic+=POLICE_EFFIC_STEP,X++)
	{
		pom.prn("P.Ef%g",effic);
		o<<pom.get()<<TAB;
	}
	break;
	case BAT_HONORvsAGRR:X=0;
	for(FLOAT prop=PROPORTION_MIN;prop<=PROPORTION_MAX;prop+=PROPORTION_STEP,X++)
	{
		pom.prn("Prop%g",prop);
		o<<pom.get()<<TAB;
	}
	o<<TAB<<TAB;X=0;
	for(FLOAT prop=PROPORTION_MIN;prop<=PROPORTION_MAX;prop+=PROPORTION_STEP,X++)
	{
		pom.prn("Prop%g",prop);
		o<<pom.get()<<TAB;
	}
	break;//=3    walk_honor_vs_agrr();
	}
	o<<X<<TAB<<"!!!"<<endl;

	//NAG��WKI WIERSZY i ZAWARTO�� TABEL
	Y=0;
	switch(batch_sele){ //Czy tryb przeszukiwania szuka po proporcjach czy po sile selekcji?
		case BAT_HONORvsCPOLL:
			for(FLOAT prop=PROPORTION_MIN;prop<=PROPORTION_MAX;prop+=PROPORTION_STEP,Y++)
			{
			   pom.prn("Prop%g",prop);
			   o<<pom.get()<<TAB;
			   X=0;
			   for(FLOAT effic=POLICE_EFFIC_MIN;effic<=POLICE_EFFIC_MAX;effic+=POLICE_EFFIC_STEP,X++)
			   {
				  o<<Tab1[Y][X]<<TAB;
			   }
			   o<<TAB;
			   pom.prn("Prop%g",prop);
			   o<<pom.get()<<TAB;
			   X=0;
			   for(FLOAT effic=POLICE_EFFIC_MIN;effic<=POLICE_EFFIC_MAX;effic+=POLICE_EFFIC_STEP,X++)
			   {
				  o<<Tab2[Y][X]<<TAB;
			   }
			   o<<endl;
			}
				break;//=1,        walk_params_sele();
		case BAT_SELECTION:  //Ma selekcj� w pionie
			for(FLOAT selec=SELECTION_MIN;selec<=SELECTION_MAX;selec+=SELECTION_STEP,Y++)
			{
				pom.prn("Sel_%g",selec);
				o<<pom.get()<<TAB;
				X=0;
				for(FLOAT effic=POLICE_EFFIC_MIN;effic<=POLICE_EFFIC_MAX;effic+=POLICE_EFFIC_STEP,X++)
					{ o<<Tab1[Y][X]<<TAB; }
				o<<TAB;
				pom.prn("Sel_%g",selec);
				o<<pom.get()<<TAB;
				X=0;
				for(FLOAT effic=POLICE_EFFIC_MIN;effic<=POLICE_EFFIC_MAX;effic+=POLICE_EFFIC_STEP,X++)
					{o<<Tab2[Y][X]<<TAB;}
				o<<endl;
			}
				break;//=2,    walk_params_prop();
		case BAT_HONORvsAGRR://Te� ma selekcj� w pionie
			for(FLOAT selec=SELECTION_MIN;selec<=SELECTION_MAX;selec+=SELECTION_STEP,Y++)
			{
				pom.prn("Sel_%g",selec);
				o<<pom.get()<<TAB;
				X=0;
				for(FLOAT prop=PROPORTION_MIN;prop<=PROPORTION_MAX;prop+=PROPORTION_STEP,X++)
				//for(FLOAT effic=POLICE_EFFIC_MIN;effic<=POLICE_EFFIC_MAX;effic+=POLICE_EFFIC_STEP,X++)
					{ o<<Tab1[Y][X]<<TAB; }
				o<<TAB;
				pom.prn("Sel_%g",selec);
				o<<pom.get()<<TAB;
				X=0;
				for(FLOAT prop=PROPORTION_MIN;prop<=PROPORTION_MAX;prop+=PROPORTION_STEP,X++)
				//for(FLOAT effic=POLICE_EFFIC_MIN;effic<=POLICE_EFFIC_MAX;effic+=POLICE_EFFIC_STEP,X++)
					{o<<Tab2[Y][X]<<TAB;}
				o<<endl;
			}
				break;//=3    walk_honor_vs_agrr();
		case NO_BAT: default: cerr<<"\n\aInvalid batch mode!\n"<< endl; exit(-111);
		}

		//Dwie ostatnie nazwy pod spodem
		o << TAB << Name1;
		for (unsigned i = 0; i < X; i++)
			o << TAB;
		o << TAB << TAB << Name2 << endl;
}

void walk_params_prop()
// Tryb analizy przestrzeni parametr�w - wydajno�� policji (POLL_EFF) i PROPORTION
{
   unsigned X = (POLICE_EFFIC_MAX - POLICE_EFFIC_MIN)/ POLICE_EFFIC_STEP + 1;

   if (!(POLICE_EFFIC_MIN + POLICE_EFFIC_STEP * X < POLICE_EFFIC_MAX))
		// Operacja == nie dzia�a poprawnie na float
			X++;
   unsigned Y = (PROPORTION_MAX - PROPORTION_MIN) / PROPORTION_STEP + 1;
   if(! (PROPORTION_MIN+PROPORTION_STEP*Y < PROPORTION_MAX) )  //Operacja == nie dzia�a poprawnie na float
										Y++;
   cout<<"Police efficiency vs proportion batch job: "<<Y<<" vs. "<<X<<" cells."<<endl;
   Parameters_dump(OutLog);
   OutLog<<"Police efficiency vs proportion batch job: "<<Y<<" vs. "<<X<<" cells."<<endl;

   wb_dynmatrix<FLOAT> MeanPowerOfAgres(Y,X);MeanPowerOfAgres.fill(-9999.0);
   wb_dynmatrix<FLOAT> MeanPowerOfHonor(Y,X);MeanPowerOfHonor.fill(-9999.0);
   wb_dynmatrix<FLOAT> MeanPowerOfPCall(Y,X);MeanPowerOfPCall.fill(-9999.0);
   wb_dynmatrix<FLOAT> MeanPowerOfOther(Y,X);MeanPowerOfOther.fill(-9999.0);
   wb_dynmatrix<FLOAT> MeanPropOfAgres(Y,X);MeanPropOfAgres.fill(-9999.0);
   wb_dynmatrix<FLOAT> MeanPropOfHonor(Y,X);MeanPropOfHonor.fill(-9999.0);
   wb_dynmatrix<FLOAT> MeanPropOfPCall(Y,X);MeanPropOfPCall.fill(-9999.0);
   wb_dynmatrix<FLOAT> MeanPropOfOther(Y,X);MeanPropOfOther.fill(-9999.0);

   for(FLOAT prop=PROPORTION_MIN,Y=0;prop<=PROPORTION_MAX;prop+=PROPORTION_STEP,Y++)
   {
	   //if(BULLI_POPUL>=0)  //????
	   //			BULLI_POPUL=prop;

	   if(Compensation_mode)
	   {                             assert(BULLI_POPUL>=0);
		  HONOR_POPUL=PROPORTION_MAX-prop;
		  CALLER_POPU=prop;
	   }
	   else
	   {
		if(HONOR_POPUL>=0)
				HONOR_POPUL=prop;
		if(CALLER_POPU>=0 )
				CALLER_POPU=prop;
	   }

	   for(FLOAT effic=POLICE_EFFIC_MIN,X=0;effic<=POLICE_EFFIC_MAX;effic+=POLICE_EFFIC_STEP,X++)
	   {
		   cout<<endl<<"SYM "<<Y<<' '<<X<<" Prop: "<<prop<<"\tPEffic: "<<effic<<endl;
		   POLICE_EFFIC=effic;

		   //P�TLA POWT�RZE� SYMULACJI
		   FLOAT MnPowOfAgres=0;
		   FLOAT MnPowOfHonor=0;
		   FLOAT MnPowOfPCall=0;
		   FLOAT MnPowOfOther=0;
		   FLOAT MnPropOfAgres=0;
		   FLOAT MnPropOfHonor=0;
		   FLOAT MnPropOfPCall=0;
		   FLOAT MnPropOfOther=0;
		   unsigned StatSteps=0;

		   for(unsigned rep=0;rep<REPETITION_LIMIT;rep++)
		   {
			 wb_dynmatrix<HonorAgent> World(SIDE,SIDE);//Pocz�tek - alokacja agent�w �wiata
			 double POPULATION=double(SIDE)*SIDE;   //Ile ich w og�le jest?
			 InitConnections(World,SIDE*SIDE*OUTFAR_LINKS_PER_AGENT);//Tworzenie sieci
			 InitAtributes(World,SIDE*SIDE); //Losowanie atrybut�w dla agent�w
			 //CalculateStatistics(World); //Po raz pierwszy dla tych parametr�w
			 for(step_counter=1;step_counter<=STOP_AFTER;step_counter++)
			 {
				Reset_action_memories(World);//Czyszczenie, mo�e niepotrzebne
				power_recovery_step(World); // Krok procesu regeneracji si�
				one_step(World); // Krok dynamiki interakcji agresywnych
				/*
				if (SOCIAL_IMPACT_INTENSITY_PERCENT > 0)
					// Opcjonalnie krok wp�ywu spo�ecznego
						social_impact_step(World,
						SOCIAL_IMPACT_INTENSITY_PERCENT);
				*/
				if (step_counter % max(EveryStep,100u) == 0)
				{
					CalculateStatistics(World);
					cout <<"\r["<<rep<<"] "<< step_counter<<"  ";

					if(step_counter>=STAT_AFTER) //Mo�e tylko koncowy stan r�wnowagi?
					{
						MnPowOfAgres += MnStrenght[0].N > 0 ? MnStrenght[0].summ / MnStrenght[0].N : 0;
						MnPowOfHonor += MnStrenght[1].N > 0 ? MnStrenght[1].summ / MnStrenght[1].N : 0;
						MnPowOfPCall += MnStrenght[2].N > 0 ? MnStrenght[2].summ / MnStrenght[2].N : 0;
						MnPowOfOther += MnStrenght[3].N > 0 ? MnStrenght[3].summ / MnStrenght[3].N : 0;
						MnPropOfAgres += MnStrenght[0].N / POPULATION;
						MnPropOfHonor += MnStrenght[1].N / POPULATION;
						MnPropOfPCall += MnStrenght[2].N / POPULATION;
						MnPropOfOther += MnStrenght[3].N / POPULATION;
						StatSteps++;
					}

					if (input_ready())
						{
							int znak = get_char();
							if (znak == 'v' || znak == 'c')
							{
								   BatchPlotPower=!BatchPlotPower;
								   clear_screen();
							}
							else
							if (znak == '\n')
								if(BatchPlotPower)
									PlotTables("MnPowOfAgres",MeanPowerOfAgres,"MnPowOfHonor",MeanPowerOfHonor,
									"MnPowOfPCall",MeanPowerOfPCall,"MnPowOfOther",MeanPowerOfOther,Batch_true_color);
								else
									PlotTables("MnPropOfAgres",MeanPropOfAgres,"MnPropOfHonor",MeanPropOfHonor,
									"MnPropOfPCall",MeanPropOfPCall,"MnPropOfOther",MeanPropOfOther,Batch_true_color);
						}
				}
			 } // KONIEC P�TLI SYMULACJI
			 DeleteAllConnections(World);  //Koniec tej symulacji
			} // KONIEC P�TLI POWTORZEN
																				assert(StatSteps>0);
			// PODLICZENIE WYNIK�W I ZAPAMI�TANIE W TABLICACH PRZESTRZENI PARAMETR�W
			MnPowOfAgres /= StatSteps;
			MnPowOfHonor /= StatSteps;
			MnPowOfPCall /= StatSteps;
			MnPowOfOther /= StatSteps;
			MnPropOfAgres /= StatSteps;
			MnPropOfHonor /= StatSteps;
			MnPropOfPCall /= StatSteps;
			MnPropOfOther /= StatSteps;
			cout << '\r' << setw(6) << setprecision(4)
				<< MnPowOfAgres << '\t' << MnPowOfHonor << '\t'
				<< MnPowOfPCall << '\t' << MnPowOfOther;
			MeanPowerOfAgres[Y][X] = MnPowOfAgres;
			MeanPowerOfHonor[Y][X] = MnPowOfHonor;
			MeanPowerOfPCall[Y][X] = MnPowOfPCall;
			MeanPowerOfOther[Y][X]=MnPowOfOther;
			MeanPropOfAgres[Y][X]=MnPropOfAgres;
			MeanPropOfHonor[Y][X]=MnPropOfHonor;
			MeanPropOfPCall[Y][X]=MnPropOfPCall;
			MeanPropOfOther[Y][X]=MnPropOfOther;

			if(BatchPlotPower)
				PlotTables("MnPowOfAgres",MeanPowerOfAgres,"MnPowOfHonor",MeanPowerOfHonor,
						"MnPowOfPCall",MeanPowerOfPCall,"MnPowOfOther",MeanPowerOfOther,Batch_true_color);
			else
				PlotTables("MnPropOfAgres",MeanPropOfAgres,"MnPropOfHonor",MeanPropOfHonor,
						"MnPropOfPCall",MeanPropOfPCall,"MnPropOfOther",MeanPropOfOther,Batch_true_color);
	   }
   }
   //Zrobienie u�ytku z wynik�w
   clear_screen();
   PlotTables("MnPowOfAgres",MeanPowerOfAgres,"MnPowOfHonor",MeanPowerOfHonor,
						"MnPowOfPCall",MeanPowerOfPCall,"MnPowOfOther",MeanPowerOfOther,Batch_true_color);
   SaveScreen(STOP_AFTER);
   clear_screen();
   PlotTables("MnPropOfAgres",MeanPropOfAgres,"MnPropOfHonor",MeanPropOfHonor,
						"MnPropOfPCall",MeanPropOfPCall,"MnPropOfOther",MeanPropOfOther,Batch_true_color);
   SaveScreen(STOP_AFTER+1);
   Write_tables(OutLog,"MnPowOfAgres",MeanPowerOfAgres,"MnPropOfAgres",MeanPropOfAgres);
   Write_tables(OutLog,"MnPowOfHonor",MeanPowerOfHonor,"MnPropOfHonor",MeanPropOfHonor);
   Write_tables(OutLog,"MnPowOfPCall",MeanPowerOfPCall,"MnPropOfPCall",MeanPropOfPCall);
   Write_tables(OutLog,"MnPowOfOther",MeanPowerOfOther,"MnPropOfOther",MeanPropOfOther);

   WB_error_enter_before_clean=0;//Juz sie bledow nie spodziewamy
   while(1)
   {
		int znak=get_char();
		if(znak==-1 || znak==27) break;
   }
}

void walk_params_sele()
//Tryb analizy przestrzeni parametr�w - wydajno�� policji (POLL_EFF) i SELECTION
{
   unsigned X=(POLICE_EFFIC_MAX-POLICE_EFFIC_MIN)/POLICE_EFFIC_STEP + 1;
   if(! (POLICE_EFFIC_MIN+POLICE_EFFIC_STEP*X < POLICE_EFFIC_MAX) ) //Operacja == nie dzia�a poprawnie na float
										X++;
   unsigned Y=(SELECTION_MAX-SELECTION_MIN)/SELECTION_STEP   + 1;
   if(! (SELECTION_MIN+SELECTION_STEP*Y < SELECTION_MAX) )  //Operacja == nie dzia�a poprawnie na float
										Y++;
   cout<<"Police efficiency vs selection batch job: "<<Y<<" vs. "<<X<<" cells."<<endl;
   Parameters_dump(OutLog);
   OutLog<<"Police efficiency vs selection batch job: "<<Y<<" vs. "<<X<<" cells."<<endl;

   wb_dynmatrix<FLOAT> MeanPowerOfAgres(Y,X);MeanPowerOfAgres.fill(-9999.0);
   wb_dynmatrix<FLOAT> MeanPowerOfHonor(Y,X);MeanPowerOfHonor.fill(-9999.0);
   wb_dynmatrix<FLOAT> MeanPowerOfPCall(Y,X);MeanPowerOfPCall.fill(-9999.0);
   wb_dynmatrix<FLOAT> MeanPowerOfOther(Y,X);MeanPowerOfOther.fill(-9999.0);
   wb_dynmatrix<FLOAT> MeanPropOfAgres(Y,X);MeanPropOfAgres.fill(-9999.0);
   wb_dynmatrix<FLOAT> MeanPropOfHonor(Y,X);MeanPropOfHonor.fill(-9999.0);
   wb_dynmatrix<FLOAT> MeanPropOfPCall(Y,X);MeanPropOfPCall.fill(-9999.0);
   wb_dynmatrix<FLOAT> MeanPropOfOther(Y,X);MeanPropOfOther.fill(-9999.0);

   for(FLOAT selec=SELECTION_MIN,Y=0;selec<=SELECTION_MAX;selec+=SELECTION_STEP,Y++)
   {
	   USED_SELECTION=selec;
	   for(FLOAT effic=POLICE_EFFIC_MIN,X=0;effic<=POLICE_EFFIC_MAX;effic+=POLICE_EFFIC_STEP,X++)
	   {
		   cout<<endl<<"SYM "<<Y<<' '<<X<<" Select: "<<selec<<"\tPEffic: "<<effic<<endl;
		   POLICE_EFFIC=effic;

		   //P�TLA POWT�RZE� SYMULACJI
		   FLOAT MnPowOfAgres=0;
		   FLOAT MnPowOfHonor=0;
		   FLOAT MnPowOfPCall=0;
		   FLOAT MnPowOfOther=0;
		   FLOAT MnPropOfAgres=0;
		   FLOAT MnPropOfHonor=0;
		   FLOAT MnPropOfPCall=0;
		   FLOAT MnPropOfOther=0;
		   unsigned StatSteps=0;

		   for(unsigned rep=0;rep<REPETITION_LIMIT;rep++)
		   {
			 wb_dynmatrix<HonorAgent> World(SIDE,SIDE);//Pocz�tek - alokacja agent�w �wiata
			 double POPULATION=double(SIDE)*SIDE;   //Ile ich w og�le jest?
			 InitConnections(World,SIDE*SIDE*OUTFAR_LINKS_PER_AGENT);//Tworzenie sieci
			 InitAtributes(World,SIDE*SIDE); //Losowanie atrybut�w dla agent�w
			 //CalculateStatistics(World); //Po raz pierwszy dla tych parametr�w
			 for(step_counter=1;step_counter<=STOP_AFTER;step_counter++)
			 {
				Reset_action_memories(World);//Czyszczenie, mo�e niepotrzebne
				power_recovery_step(World); // Krok procesu regeneracji si�
				one_step(World); // Krok dynamiki interakcji agresywnych
				/*
				if (SOCIAL_IMPACT_INTENSITY_PERCENT > 0)
					// Opcjonalnie krok wp�ywu spo�ecznego
						social_impact_step(World,
						SOCIAL_IMPACT_INTENSITY_PERCENT);
				*/
				if (step_counter % max(EveryStep,100u) == 0)
				{
					CalculateStatistics(World);
					cout <<"\r["<<rep<<"] "<< step_counter<<"  ";

					if(step_counter>=STAT_AFTER) //Mo�e tylko koncowy stan r�wnowagi?
					{
						MnPowOfAgres += MnStrenght[0].N > 0 ? MnStrenght[0].summ / MnStrenght[0].N : 0;
						MnPowOfHonor += MnStrenght[1].N > 0 ? MnStrenght[1].summ / MnStrenght[1].N : 0;
						MnPowOfPCall += MnStrenght[2].N > 0 ? MnStrenght[2].summ / MnStrenght[2].N : 0;
						MnPowOfOther += MnStrenght[3].N > 0 ? MnStrenght[3].summ / MnStrenght[3].N : 0;
						MnPropOfAgres += MnStrenght[0].N / POPULATION;
						MnPropOfHonor += MnStrenght[1].N / POPULATION;
						MnPropOfPCall += MnStrenght[2].N / POPULATION;
						MnPropOfOther += MnStrenght[3].N / POPULATION;
						StatSteps++;
					}

					if (input_ready())
						{
							int znak = get_char();
							if (znak == 'v' || znak == 'c')
							{
								   BatchPlotPower=!BatchPlotPower;
								   clear_screen();
							}
							else
							if (znak == '\n')
								if(BatchPlotPower)
									PlotTables("MnPowOfAgres",MeanPowerOfAgres,"MnPowOfHonor",MeanPowerOfHonor,
									"MnPowOfPCall",MeanPowerOfPCall,"MnPowOfOther",MeanPowerOfOther,Batch_true_color);
								else
									PlotTables("MnPropOfAgres",MeanPropOfAgres,"MnPropOfHonor",MeanPropOfHonor,
									"MnPropOfPCall",MeanPropOfPCall,"MnPropOfOther",MeanPropOfOther,Batch_true_color);
						}
				}
			 } // KONIEC P�TLI SYMULACJI
			 DeleteAllConnections(World);  //Koniec tej symulacji
			} // KONIEC P�TLI POWTORZEN
																				assert(StatSteps>0);
			// PODLICZENIE WYNIK�W I ZAPAMI�TANIE W TABLICACH PRZESTRZENI PARAMETR�W
			MnPowOfAgres /= StatSteps;
			MnPowOfHonor /= StatSteps;
			MnPowOfPCall /= StatSteps;
			MnPowOfOther /= StatSteps;
			MnPropOfAgres /= StatSteps;
			MnPropOfHonor /= StatSteps;
			MnPropOfPCall /= StatSteps;
			MnPropOfOther /= StatSteps;
			cout << '\r' << setw(6) << setprecision(4)
				<< MnPowOfAgres << '\t' << MnPowOfHonor << '\t'
				<< MnPowOfPCall << '\t' << MnPowOfOther;
			MeanPowerOfAgres[Y][X] = MnPowOfAgres;
			MeanPowerOfHonor[Y][X] = MnPowOfHonor;
			MeanPowerOfPCall[Y][X] = MnPowOfPCall;
			MeanPowerOfOther[Y][X]=MnPowOfOther;
			MeanPropOfAgres[Y][X]=MnPropOfAgres;
			MeanPropOfHonor[Y][X]=MnPropOfHonor;
			MeanPropOfPCall[Y][X]=MnPropOfPCall;
			MeanPropOfOther[Y][X]=MnPropOfOther;

			if(BatchPlotPower)
				PlotTables("MnPowOfAgres",MeanPowerOfAgres,"MnPowOfHonor",MeanPowerOfHonor,
						"MnPowOfPCall",MeanPowerOfPCall,"MnPowOfOther",MeanPowerOfOther,Batch_true_color);
			else
				PlotTables("MnPropOfAgres",MeanPropOfAgres,"MnPropOfHonor",MeanPropOfHonor,
						"MnPropOfPCall",MeanPropOfPCall,"MnPropOfOther",MeanPropOfOther,Batch_true_color);
	   }
   }
   //Zrobienie u�ytku z wynik�w
   clear_screen();
   PlotTables("MnPowOfAgres",MeanPowerOfAgres,"MnPowOfHonor",MeanPowerOfHonor,
						"MnPowOfPCall",MeanPowerOfPCall,"MnPowOfOther",MeanPowerOfOther,Batch_true_color);
   SaveScreen(STOP_AFTER);
   clear_screen();
   PlotTables("MnPropOfAgres",MeanPropOfAgres,"MnPropOfHonor",MeanPropOfHonor,
						"MnPropOfPCall",MeanPropOfPCall,"MnPropOfOther",MeanPropOfOther,Batch_true_color);
   SaveScreen(STOP_AFTER+1);
   Write_tables(OutLog,"MnPowOfAgres",MeanPowerOfAgres,"MnPropOfAgres",MeanPropOfAgres);
   Write_tables(OutLog,"MnPowOfHonor",MeanPowerOfHonor,"MnPropOfHonor",MeanPropOfHonor);
   Write_tables(OutLog,"MnPowOfPCall",MeanPowerOfPCall,"MnPropOfPCall",MeanPropOfPCall);
   Write_tables(OutLog,"MnPowOfOther",MeanPowerOfOther,"MnPropOfOther",MeanPropOfOther);

   WB_error_enter_before_clean=0;//Juz sie bledow nie spodziewamy
   while(1)
   {
		int znak=get_char();
		if(znak==-1 || znak==27) break;
   }
}

void walk_honor_vs_agrr()
//Tryb analizy przestrzeni parametr�w - SELECTION i RATIO Agresywnych do Honorowych
{
   unsigned X = (PROPORTION_MAX - PROPORTION_MIN) / PROPORTION_STEP + 1;
   if(! (PROPORTION_MIN+PROPORTION_STEP*X < PROPORTION_MAX) )  //Operacja == nie dzia�a poprawnie na float
										X++;
   unsigned Y=(SELECTION_MAX-SELECTION_MIN)/SELECTION_STEP   + 1;
   if(! (SELECTION_MIN+SELECTION_STEP*Y < SELECTION_MAX) )  //Operacja == nie dzia�a poprawnie na float
										Y++;
   cout<<"Agressive to honor proportion vs selection batch job: "<<Y<<" vs. "<<X<<" cells."<<endl;
   Parameters_dump(OutLog);
   OutLog<<"Agressive to honor proportion vs selection batch job: "<<Y<<" vs. "<<X<<" cells."<<endl;

   wb_dynmatrix<FLOAT> MeanPowerOfAgres(Y,X);MeanPowerOfAgres.fill(-9999.0);
   wb_dynmatrix<FLOAT> MeanPowerOfHonor(Y,X);MeanPowerOfHonor.fill(-9999.0);
   wb_dynmatrix<FLOAT> MeanPowerOfPCall(Y,X);MeanPowerOfPCall.fill(-9999.0);
   wb_dynmatrix<FLOAT> MeanPowerOfOther(Y,X);MeanPowerOfOther.fill(-9999.0);
   wb_dynmatrix<FLOAT> MeanPropOfAgres(Y,X);MeanPropOfAgres.fill(-9999.0);
   wb_dynmatrix<FLOAT> MeanPropOfHonor(Y,X);MeanPropOfHonor.fill(-9999.0);
   wb_dynmatrix<FLOAT> MeanPropOfPCall(Y,X);MeanPropOfPCall.fill(-9999.0);
   wb_dynmatrix<FLOAT> MeanPropOfOther(Y,X);MeanPropOfOther.fill(-9999.0);

   for(FLOAT selec=SELECTION_MIN,Y=0;selec<=SELECTION_MAX;selec+=SELECTION_STEP,Y++)
   {
	   USED_SELECTION=selec;

	   for(FLOAT prop=PROPORTION_MIN,X=0;prop<=PROPORTION_MAX;prop+=PROPORTION_STEP,X++)
	   {
			//if(BULLI_POPUL>=0)  //????
			//			BULLI_POPUL=prop;

		   if(Compensation_mode)
			{
			  BULLI_POPUL=PROPORTION_MAX-prop;
			  HONOR_POPUL=prop;
			}
			else
			{
			 if(HONOR_POPUL>=0)
					HONOR_POPUL=prop;
			 if(BULLI_POPUL>=0 )
					BULLI_POPUL=prop;
			}

		   cout<<endl<<"SYM "<<Y<<' '<<X<<" Prop: "<<prop<<"\tSelec: "<<selec<<endl;

		   //P�TLA POWT�RZE� SYMULACJI
		   FLOAT MnPowOfAgres=0;
		   FLOAT MnPowOfHonor=0;
		   FLOAT MnPowOfPCall=0;
		   FLOAT MnPowOfOther=0;
		   FLOAT MnPropOfAgres=0;
		   FLOAT MnPropOfHonor=0;
		   FLOAT MnPropOfPCall=0;
		   FLOAT MnPropOfOther=0;
		   unsigned StatSteps=0;

		   for(unsigned rep=0;rep<REPETITION_LIMIT;rep++)
		   {
			 wb_dynmatrix<HonorAgent> World(SIDE,SIDE);//Pocz�tek - alokacja agent�w �wiata
			 double POPULATION=double(SIDE)*SIDE;   //Ile ich w og�le jest?
			 InitConnections(World,SIDE*SIDE*OUTFAR_LINKS_PER_AGENT);//Tworzenie sieci
			 InitAtributes(World,SIDE*SIDE); //Losowanie atrybut�w dla agent�w
			 //CalculateStatistics(World); //Po raz pierwszy dla tych parametr�w
			 for(step_counter=1;step_counter<=STOP_AFTER;step_counter++)
			 {
				Reset_action_memories(World);//Czyszczenie, mo�e niepotrzebne
				power_recovery_step(World); // Krok procesu regeneracji si�
				one_step(World); // Krok dynamiki interakcji agresywnych
				/*
				if (SOCIAL_IMPACT_INTENSITY_PERCENT > 0)
					// Opcjonalnie krok wp�ywu spo�ecznego
						social_impact_step(World,
						SOCIAL_IMPACT_INTENSITY_PERCENT);
				*/
				if (step_counter % max(EveryStep,100u) == 0)
				{
					CalculateStatistics(World);
					cout <<"\r["<<rep<<"] "<< step_counter<<"  ";

					if(step_counter>=STAT_AFTER) //Mo�e tylko koncowy stan r�wnowagi?
					{
						MnPowOfAgres += MnStrenght[0].N > 0 ? MnStrenght[0].summ / MnStrenght[0].N : 0;
						MnPowOfHonor += MnStrenght[1].N > 0 ? MnStrenght[1].summ / MnStrenght[1].N : 0;
						MnPowOfPCall += MnStrenght[2].N > 0 ? MnStrenght[2].summ / MnStrenght[2].N : 0;
						MnPowOfOther += MnStrenght[3].N > 0 ? MnStrenght[3].summ / MnStrenght[3].N : 0;
						MnPropOfAgres += MnStrenght[0].N / POPULATION;
						MnPropOfHonor += MnStrenght[1].N / POPULATION;
						MnPropOfPCall += MnStrenght[2].N / POPULATION;
						MnPropOfOther += MnStrenght[3].N / POPULATION;
						StatSteps++;
					}

					if (input_ready())
						{
							int znak = get_char();
							if (znak == 'v' || znak == 'c')
							{
								   BatchPlotPower=!BatchPlotPower;
								   clear_screen();
							}
							else
							if (znak == '\n')
								if(BatchPlotPower)
									PlotTables("MnPowOfAgres",MeanPowerOfAgres,"MnPowOfHonor",MeanPowerOfHonor,
									"MnPowOfPCall",MeanPowerOfPCall,"MnPowOfOther",MeanPowerOfOther,Batch_true_color);
								else
									PlotTables("MnPropOfAgres",MeanPropOfAgres,"MnPropOfHonor",MeanPropOfHonor,
									"MnPropOfPCall",MeanPropOfPCall,"MnPropOfOther",MeanPropOfOther,Batch_true_color);
						}
				}
			 } // KONIEC P�TLI SYMULACJI
			 DeleteAllConnections(World);  //Koniec tej symulacji
			} // KONIEC P�TLI POWTORZEN
																				assert(StatSteps>0);
			// PODLICZENIE WYNIK�W I ZAPAMI�TANIE W TABLICACH PRZESTRZENI PARAMETR�W
			MnPowOfAgres /= StatSteps;
			MnPowOfHonor /= StatSteps;
			MnPowOfPCall /= StatSteps;
			MnPowOfOther /= StatSteps;
			MnPropOfAgres /= StatSteps;
			MnPropOfHonor /= StatSteps;
			MnPropOfPCall /= StatSteps;
			MnPropOfOther /= StatSteps;
			cout << '\r' << setw(6) << setprecision(4)
				<< MnPowOfAgres << '\t' << MnPowOfHonor << '\t'
				<< MnPowOfPCall << '\t' << MnPowOfOther;
			MeanPowerOfAgres[Y][X] = MnPowOfAgres;
			MeanPowerOfHonor[Y][X] = MnPowOfHonor;
			MeanPowerOfPCall[Y][X] = MnPowOfPCall;
			MeanPowerOfOther[Y][X]=MnPowOfOther;
			MeanPropOfAgres[Y][X]=MnPropOfAgres;
			MeanPropOfHonor[Y][X]=MnPropOfHonor;
			MeanPropOfPCall[Y][X]=MnPropOfPCall;
			MeanPropOfOther[Y][X]=MnPropOfOther;

			if(BatchPlotPower)
				PlotTables("MnPowOfAgres",MeanPowerOfAgres,"MnPowOfHonor",MeanPowerOfHonor,
						"MnPowOfPCall",MeanPowerOfPCall,"MnPowOfOther",MeanPowerOfOther,Batch_true_color);
			else
				PlotTables("MnPropOfAgres",MeanPropOfAgres,"MnPropOfHonor",MeanPropOfHonor,
						"MnPropOfPCall",MeanPropOfPCall,"MnPropOfOther",MeanPropOfOther,Batch_true_color);
	   }
   }
   //Zrobienie u�ytku z wynik�w
   clear_screen();
   PlotTables("MnPowOfAgres",MeanPowerOfAgres,"MnPowOfHonor",MeanPowerOfHonor,
						"MnPowOfPCall",MeanPowerOfPCall,"MnPowOfOther",MeanPowerOfOther,Batch_true_color);
   SaveScreen(STOP_AFTER);
   clear_screen();
   PlotTables("MnPropOfAgres",MeanPropOfAgres,"MnPropOfHonor",MeanPropOfHonor,
						"MnPropOfPCall",MeanPropOfPCall,"MnPropOfOther",MeanPropOfOther,Batch_true_color);
   SaveScreen(STOP_AFTER+1);
   Write_tables(OutLog,"MnPowOfAgres",MeanPowerOfAgres,"MnPropOfAgres",MeanPropOfAgres);
   Write_tables(OutLog,"MnPowOfHonor",MeanPowerOfHonor,"MnPropOfHonor",MeanPropOfHonor);
   Write_tables(OutLog,"MnPowOfPCall",MeanPowerOfPCall,"MnPropOfPCall",MeanPropOfPCall);
   Write_tables(OutLog,"MnPowOfOther",MeanPowerOfOther,"MnPropOfOther",MeanPropOfOther);

   WB_error_enter_before_clean=0;//Juz sie bledow nie spodziewamy
   while(1)
   {
		int znak=get_char();
		if(znak==-1 || znak==27) break;
   }
}

void fixed_params_mode()
//Tryb interakcyjny z pe�n� wizualizacj�
{
	wb_dynmatrix<HonorAgent> World(SIDE,SIDE);//Tablica agent�w �wiata

	InitConnections(World,SIDE*SIDE*OUTFAR_LINKS_PER_AGENT);
	InitAtributes(World,SIDE*SIDE);
	dump_step(World,0);//Po raz pierwszy
	CalculateStatistics(World); //Po raz pierwszy

	//Prowizoryczna p�tla g��wna
	Help();
	int cont=1;//flaga kontynuacji programu
	int runs=0;//Flaga wykonywania symulacji
	while(cont)
	{
		char tab[2];
		tab[1]=0;
		if(input_ready())
		{
			tab[0]=get_char();
			switch(tab[0])
			{
			case '1':EveryStep=1;cout<<"Visualisation at every "<<EveryStep<<" step."<<endl;break;
			case '2':EveryStep=2;cout<<"Visualisation at every "<<EveryStep<<" step."<<endl;break;
			case '3':EveryStep=3;cout<<"Visualisation at every "<<EveryStep<<" step."<<endl;break;
			case '4':EveryStep=4;cout<<"Visualisation at every "<<EveryStep<<" step."<<endl;break;
			case '5':EveryStep=5;cout<<"Visualisation at every "<<EveryStep<<" step."<<endl;break;
			case '6':EveryStep=10;cout<<"Visualisation at every "<<EveryStep<<" step."<<endl;break;
			case '7':EveryStep=20;cout<<"Visualisation at every "<<EveryStep<<" step."<<endl;break;
			case '8':EveryStep=25;cout<<"Visualisation at every "<<EveryStep<<" step."<<endl;break;
			case '9':EveryStep=50;cout<<"Visualisation at every "<<EveryStep<<" step."<<endl;break;
			case '0':EveryStep=100;cout<<"Visualisation at every "<<EveryStep<<" step."<<endl;break;

			case 'b':SaveScreen(step_counter);break;
			case 'd':dump_screens=!dump_screens;cout<<"\n From now screen will"<<(dump_screens?"be":"not be")<<" dumped..."<<endl;break;

			case '>'://Next step
			case 'n':runs=0;one_step(World);CalculateStatistics(World);replot(World);break;
			case 'p':runs=0;replot(World);break;
			case 'r':runs=1;break;

			case 'c':ConsoleLog=!ConsoleLog;break;
			case 's':VisShorLinks=!VisShorLinks;clear_screen();replot(World);break; //Wizualizacja bliskich link�w
			case 'f':VisFarLinks=!VisFarLinks;clear_screen();replot(World);break;	  //Wizualizacja dalekich
			case 'a':VisAgents=!VisAgents;clear_screen();replot(World);break;     //Wizualizacja w�a�ciwo�ci agent�w
			case 'v'://if(CalculateColorDefault==  //Wybranie typu wizualizacji
					replot(World);break;

			case '\b':mouse_check(World);break;
			case '@':
			case '\r':clear_screen();
			case '\n':replot(World);if(ConsoleLog)cout<<endl<<endl;break;

			case 'q':
			case EOF:  WB_error_enter_before_clean=0;//Juz sie bledow nie spodziewamy
					   cont=0;break;
			case 'h':
			default:
					Help();break;
			}
			flush_plot();
		}
		else
			if(runs)
			{
				Reset_action_memories(World);

				//Krok procesu regeneracji si�
				power_recovery_step(World);

				//Krok dynamiki interakcji agresywnych
				one_step(World);

				//Opcjonalnie krok wp�ywu spo�ecznego
				/*
				if(SOCIAL_IMPACT_INTENSITY_PERCENT>0)
					social_impact_step(World,SOCIAL_IMPACT_INTENSITY_PERCENT);
				*/
				if(step_counter%EveryStep==0)
				{
					CalculateStatistics(World);
					replot(World);

					if(dump_screens)
					{
						SaveScreen(step_counter);
						cout<<"\nScreen for step "<<step_counter<<" dumped."<<endl;
					}

					if(step_counter>=STOP_AFTER)
					{
						if(RepetNum<REPETITION_LIMIT )//Kolejna repetycja?
						{
							dump_step(World,step_counter);//Po raz ostatni
							RepetNum++;
							cout<<"\nStop after "<<STOP_AFTER<<" steps\a, and start iteration n# "<<RepetNum<<endl;
							Reset_action_memories(World);
							DeleteAllConnections(World);
							InitConnections(World,SIDE*SIDE*OUTFAR_LINKS_PER_AGENT);
							InitAtributes(World,SIDE*SIDE);
							step_counter=0;
							LiczbaTrupow=0; //Troch� to partyzantka
							LiczbaTrupowDzis=0;
							CalculateStatistics(World); //Po raz pierwszy dla tej iteracji
                            OutLog<<"next"<<endl;
							save_stat();
							dump_step(World,0);//Po raz pierwszy
						}
						else
						{
							dump_step(World,step_counter);//Po raz ostatni
							cout<<"\nStop because of limit "<<STOP_AFTER<<" steps\a\a"<<endl;
							runs=false;
						}
					}
					else
					{
					   if(step_counter%DumpStep==0)//10-100x rzadziej ni� statystyka
					   {
							dump_step(World,step_counter);//Co jaki� czas
                       }
					}
				}
			}
	}
}

void Help()
{
	cout<<"POSSIBLE COMMANDS FOR GRAPHIC WINDOW:"<<endl
		<<"q-quit or ESC\n"
		"\n"
		"n-next MC step\n"
		"p-pause simulation\n"
		"r-run simulation\n"
		"\n"
		"b-save screen to BMP\n"
		"d-dump on every visualisation\n"
		"\n"
		"1..0 visualisation freq.\n"
		"c-swich console on/off\n"
		"s - visualise short links\n"
		"f - visualise far links\n"
		"a - visualise agents\n"
		"\n"
		"mouse left or right - \n"
		"     inspection\n"
		"ENTER - replot screen\n";
}

void PrintHonorAgentInfo(ostream& o,const HonorAgent& H)
{
	//PrettyPrint(o,H.Pattern);o<<endl;	//Wzorzec bitowy - geny lub memy kultury - mo�e p�niej
	o<<"Individual color: rgb("<<unsigned(H.GetColor().r)<<','<<unsigned(H.GetColor().g)<<','<<unsigned(H.GetColor().b)<<')'<<endl;	//Obliczony, kt�r�� z funkcji koduj�cych kolor
	o<<"Curr. strenght: "<<H.Power<<" streght limit: "<<H.PowLimit<<endl;//	Si�a (0..1) i jak� si�� mo�e osi�gn�� maksymalnie, gdy nie traci
	o<<"Agression prob.: "<<H.Agres<<endl;// Bulizm (0..1) sk�onno�� do atakowania
	o<<"Feighter reputation: "<<H.GetFeiReputation()<<endl;//Reputacja wojownika jako �w�drowna �rednia� z konfrontacji (0..1)
	o<<"Honor: "<<H.Honor<<endl;// Bezwarunkowa honorowo�� (0..1) sk�onno�� podj�cia obrony
	o<<"Police call.: "<<H.CallPolice<<endl;//Odium wzywacza policji - prawdopodobie�stwo wzywania policji (0..1) jako �w�drowna �rednia� wezwa�
	o<<endl;
}

void mouse_check(wb_dynmatrix<HonorAgent>& World)
{
	int xpos=0;
	int ypos=0;
	int click=0;
	get_mouse_event(&xpos,&ypos,&click);
	xpos=xpos/VSIZ;
	ypos=ypos/VSIZ;
	set_pen_rgb(255,255,255,0,SSH_LINE_SOLID); // Ustala aktualny kolor linii za pomoca skladowych RGB
	if(xpos<SIDE && ypos<SIDE)//Trafiono agenta
	{
		HonorAgent& Ag=World[ypos][xpos];	//Zapami�tanie referencji do agenta, �eby ci�gle nie liczy� indeks�w

		cout<<"INSPECTION OF HonorAgent: "<<xpos<<' '<<ypos<<endl;
		PrintHonorAgentInfo(cout,Ag);
		if(click==1)
		{

		}
		else
			if(click==2)
			{


			}
	}
}

void SaveScreen(unsigned step)
{
	wb_pchar Filename;
	Filename.alloc(128);
	Filename.prn("D_%06u_%s",step,LogName);
	dump_screen(Filename.get_ptr_val());
}

int   WB_error_enter_before_clean=1; //Czy da� szanse operatorowi na poczytanie komunikat�w ko�cowych
