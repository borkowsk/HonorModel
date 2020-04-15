// Prototypowy program "ewolucji" kultury honory w kultur� policyjn�
//                                                             wersja 18-05-2012
////////////////////////////////////////////////////////////////////////////////
#include <iostream>
#include <fstream>
#define _USE_MATH_DEFINES //bo M_PI
#include <cmath>

#include "INCLUDE/wb_ptr.hpp"
using namespace wbrtm;
//#include "INCLUDE/wb_bitarray.h" //Mo�e si� przyda p�niej
#define USES_STDC_RAND
#include "INCLUDE/Random.h"
#include "SYMSHELL/symshell.h"
#include "SYMSHELL/sshutils.hpp"
#include "MISCCLASSES/Wieloboki.hpp"


const char* MODELNAME="Culture of honor";
const char* VERSIONNUM="0.24 (19-05-2012)";
//ZMIANY:
//0.24 - wprowadzenie parametru RATIONALITY, bo u�ycie w pe�ni realistycznej oceny si�y psuje selekcje
//		 Zmiany kosmetyczne w nag��wku pliku wyj�ciowego
//0.23 - znaj� swoj� realn� si�� i j� por�wnuj� z reputacj� drugiego gdy maj� atakowa� lub si� broni�


const unsigned SIDE=50;//SIDE*SIDE to rozmiar �wiata symulacji
const float    POLICE_EFFIC=0.00;//0.950; //Z jakim prawdopodobie�stwem wezwana policja obroni agenta
const float    USE_SELECTION=0.05;//0.05; //Czy bardzo przegrani umieraj�
const float    RECOVERY_POWER=0.01;//Jak� cz�� si�y odzyskuje w kroku
const float    RATIONALITY=0.0; //Jak realistycznie ocenia w�asn� si�� (vs. wg. w�asnej reputacji)
const float    RANDOM_AGRESSION=0.005;//Bazowy poziom agresji dla honorowych

//LINKOWANIE
const bool 	   TORUS=false; //Czy geometria torusa czy wyspy z brzegami
const int	   MOORE_RAD=3; //Nie zmienia� na unsigned bo si� psuje losowanie!
const float    OUTFAR_LINKS_PER_AGENT=0.5; //Ile jest dodatkowych link�w jako u�amek liczby agent�w
const unsigned MOORE_SIZE=(4*MOORE_RAD*MOORE_RAD)+4*MOORE_RAD;//S�siedzi Moora - cztery kwadranty + 4 osie
const unsigned FAR_LINKS=unsigned(SIDE*SIDE*OUTFAR_LINKS_PER_AGENT*2);
const unsigned MAX_LINKS=MOORE_SIZE + 2/*ZAPAS*/ + (FAR_LINKS/(SIDE*SIDE)? FAR_LINKS/(SIDE*SIDE):2); //Ile maksymalnie mo�e miec agent link�w


//CECHY WRODZONE
const float    BULLISM_LIMIT=-1;//0.2;//0.66;//0.10;//Maksymalny mo�liwy bulizm.
								//Jak ujemne to rozk�ad Pareto, jak dodatnie to dzwonowy

const float    BULLI_POPUL=0.3333;//0.2;//0.100;//Albo zero-jedynkowo. Jak 1 to decyduje rozk�ad sterowany BULLISM_LIMIT
const float	   HONOR_POPUL=0.3333;//Jaka cz�� agent�w populacji jest �ci�le honorowa
const float    CALLER_POPU=0.3333;//Jaka cz�� wzywa policje zamiast si� poddawa�
								 //- musi by� dwa razy wi�cej bo to udzia� reszcie tych co nie s� agresywni i honorowi
								 //Mo�e te� jako �w�drowna �rednia� wezwa�, ale na razie nie

//const unsigned MAX_INIT_REPUTATION=100; //Maksymalna mo�liwa pocz�tkowa reputacja
const double   SOCIAL_IMPACT_INTENSITY_PERCENT=0; //Jaka jest wzgl�dna intensywno�� wp�ywu spo�ecznego. W procentach kroku MC g��wnej dynamiki. 0 oznacza brak

//Sterowanie statystykami i powt�rzeniami
unsigned REPETITION_LIMIT=1;//10//Ile ma zrobi� powt�rze� tego samego eksperymentu
unsigned RepetNum=1; //Kt�ra to kolejna repetycja?  - NIE ZMIENIA� R�CZNIE!
unsigned STOP_AFTER=10000;//Po jakim czasie staje automatycznie
unsigned EveryStep=10;//Cz�stotliwo�� wizualizacji i zapisu do logu
					 //Ujemne oznacza tryb automatycznej inkrementacji
unsigned DumpStep=1000;//Cz�sto�� zrzut�w stan�w agent�w

//Parametry techniczne steruj�ce wizualizacj� i wydrukami
const unsigned VSIZ=5; //Maksymalny rozmiar boku agenta w wizualizacji kompozytowej
const unsigned SSIZ=1; //Bok agenta w wizualizacji uzupe�niaj�cej (ma�ej)

bool  ConsoleLog=1;//Czy u�ywa logowania zdarze� ma konsoli. Wa�ne dla startu, potem si� da prze��cza�
bool  VisShorLinks=false; //Wizualizacja bliskich link�w
bool  VisFarLinks=false;  //Wizualizacja dalekich
bool  VisAgents=true;     //Wizualizacja w�a�ciwo�ci agent�w
bool  dump_screens=false;

int   WB_error_enter_before_clean=0; //Czy da� szanse operatorowi na poczytanie komunikat�w ko�cowych

void Parameters_dump(ostream& o)
{
	o<<MODELNAME<<"\tv.:\t"<<VERSIONNUM<<endl;
	o<<"uint"<<'\t'<<"SIDE"<<'\t'<<SIDE<<endl;//SIDE*SIDE to rozmiar �wiata symulacji
	o<<"bool"<<'\t'<<"TORUS"<<'\t'<<TORUS<<endl;

	o<<"uint"<<'\t'<<"MOORE_RAD"<<'\t'<<MOORE_RAD<<endl;
	o<<"float"<<'\t'<<"OUT_FAR_LINKS_PER_AGENT"<<'\t'<<OUTFAR_LINKS_PER_AGENT<<endl; //Ile jest dodatkowych link�w jako u�amek liczby agent�w
	o<<"float"<<'\t'<<"USE_SELECTION"<<'\t'<<USE_SELECTION<<endl;
	o<<"float"<<'\t'<<"RECOVERY_POWER"<<'\t'<<RECOVERY_POWER<<endl;//Jak� cz�� si�y odzyskuje w kroku

	if(BULLI_POPUL>=1)
	o<<"float"<<'\t'<<"BULLISM_LIMIT"<<'\t'<<BULLISM_LIMIT<<endl;//Maksymalny mo�liwy bulizm.
	else
	{
	o<<"float"<<'\t'<<"BULLY_POPUL"<<'\t'<<BULLI_POPUL<<endl;//Albo zero-jedynkowo
	o<<"float"<<'\t'<<"HONOR_POPUL"<<'\t'<<HONOR_POPUL<<endl;//Jaka cz�� agent�w populacji jest �ci�le honorowa
	o<<"float"<<'\t'<<"CALLP_POPUL"<<'\t'<<CALLER_POPU<<endl;//Jaka cz�� wzywa policje zamiast si� poddawa�
	}
	o<<"float"<<'\t'<<"POLICE_EFFIC"<<'\t'<<POLICE_EFFIC<<endl;
	o<<"float"<<'\t'<<"always give up"<<'\t'<<(1-POLICE_EFFIC-CALLER_POPU-HONOR_POPUL)<<endl;//"loosers"
	o<<"float"<<'\t'<<"RANDOM_AGRESSION"<<RANDOM_AGRESSION<<endl;//0.950; //Z jakim prawdopodobie�stwem wezwana policja obroni agenta
	o<<"float"<<'\t'<<"RATIONALITY"<<RATIONALITY<<endl<<endl; //Jak realistycznie ocenia w�asn� si�� (vs. wg. w�asnej reputacji)

	o<<"REPET:"<<'\t'<<"STOP:"<<'\t'<<"VFREQ:"<<endl;
	o<<REPETITION_LIMIT<<'\t'<<STOP_AFTER<<'\t'<<EveryStep<<endl<<endl;
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
ofstream OutLog("Honor.log");
ofstream Dumps("Honor_dump.txt");
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
			Agres=(DRAND()+DRAND()+DRAND()+DRAND()+DRAND()+DRAND())/6*BULLISM_LIMIT; // Bulizm (0..1) sk�onno�� do atakowania,  const float    BULLISM_LIMIT=1;//Maksymalny mo�liwy bulizm.
			else
			Agres=(DRAND()*DRAND()*DRAND()*DRAND())*fabs(BULLISM_LIMIT);
		}

		Honor=(DRAND()<HONOR_POPUL?1:0); //Albo jest albo nie jest -  Bezwarunkowa honorowo�� (0..1) sk�onno�� podj�cia obrony,  const float	   HONOR_POPUL=0.9;//Jaka cz�� agent�w populacji jest �ci�le honorowa
	

		if(Honor<0.99 && Agres<0.99 && DRAND()<CALLER_POPU) //??? Agres XOR HighHonor XOR CallPolice
			CallPolice=1;//Prawdopodobie�stwo wzywania policji (0..1) Mo�e te� jako �w�drowna �rednia� wezwa�, ale na razie nie
			else
			CallPolice=0;
*/
		if(BULLI_POPUL<1)
		{
		   Honor=0;CallPolice=0;//Warto�ci domy�lne dla agresywnych i wzywaj�cych policje
		   Agres=(DRAND()<BULLI_POPUL?1:0); //Albo jest albo nie jest
		   if(Agres!=1)
		   {
			 Honor=(DRAND()*(1-BULLI_POPUL)<HONOR_POPUL?1:0);
			 if(Honor!=1)
			   CallPolice=(DRAND()*(1-BULLI_POPUL-HONOR_POPUL)<CALLER_POPU?1:0);
			   else
			   { /* LOOSER */ }
		   }
        }
	 //	Resources=(DRAND()*DRAND()*DRAND()*DRAND()*DRAND()*DRAND())*10000;  //Zasoby (0..inf)
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
	 if(DRAND()<this->CallPolice)  //TOCHECK CallPolice==0
	 {
		this->MemOfLastDecision=CALLAUTH;
	 }
	 else
	 if(DRAND()<this->Honor
	 || Ag.HonorFeiRep<this->Power /*HonorFeiRep*/)   //TOCHECK? Honor==1
	 {
		this->MemOfLastDecision=FIGHT;
	 }

	 return this->MemOfLastDecision;
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
		{
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
						  AgH.lost_power(-0.5*DRAND());//I troch� dosta� �omot dla przyk�adu
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

void InitConnections(wb_dynmatrix<HonorAgent>& World,float HowManyFar)
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

void InitAtributes(wb_dynmatrix<HonorAgent>& World,float HowMany)
{
	for(int y=0;y<SIDE;y++)
	  for(int x=0;x<SIDE;x++)
		World[y][x].RandomReset();
}


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

unsigned LiczbaTrupow=0;
unsigned LiczbaTrupowDzis=0;
void power_recovery_step(wb_dynmatrix<HonorAgent>& World)
{
  LiczbaTrupowDzis=0;
  for(int x=0;x<SIDE;x++)
	for(int y=0;y<SIDE;y++)
	{
		HonorAgent& Ag=World[y][x];	//Zapami�tanie referencji do agenta, �eby ci�gle nie liczy� indeks�w

		if(USE_SELECTION>0 && Ag.Power<USE_SELECTION)  //Chyba nie przezy�
		{
			Ag.RandomReset();
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
}

//  Liczenie statystyk
///////////////////////////////////////////////////////////////////////////////
double MeanFeiReputation=0;
double MeanCallPolice=0;
double MeanPower=0;
double MeanAgres=0;
double MeanHonor=0;

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
		OutLog<<"MC_STEP"   <<'\t'<<"MeanAgres"<<'\t'<<"MeanHonor"<<'\t'<<"MeanCallPolice"<<'\t'<<"MeanFeiReputation"<<'\t'<<"MeanPower"<<'\t'<<"All killed"<<'\t'<<"NOTHING"<<'\t'<<"WITHDRAW"<<'\t'<<"GIVEUP"<<'\t'<<"HOOK"<<'\t'<<"FIGHT"<<'\t'<<"CALLAUTH"<<endl;
		if(REPETITION_LIMIT>1)//Gdy ma zrobi� wi�cej powt�rze� tego samego eksperymentu
			OutLog<<RepetNum<<'\t'; //Kt�ra to kolejna repetycja?
		OutLog<<"0.1"<<'\t'<<MeanAgres<<'\t'<<MeanHonor<<'\t'<<MeanCallPolice<<'\t'<<MeanFeiReputation<<'\t'<<MeanPower<<'\t'<<LiczbaTrupow<<'\t'<<Actions.NOTHING<<'\t'<<Actions.WITHDRAW<<'\t'<<Actions.GIVEUP<<'\t'<<Actions.HOOK<<'\t'<<Actions.FIGHT<<'\t'<<Actions.CALLAUTH<<endl;
		if(ConsoleLog)
		  cout<<"STEP"<<'\t'<<"MnAgres"<<'\t'<<"MnHonor"<<'\t'<<"MnFeiRep"<<'\t'<<"MnCallRep"<<'\t'<<"MnPower"<<'\t'<<endl;
	}
	else
	if(LastStep!=step_counter)
	{
		if(REPETITION_LIMIT>1)//Gdy ma zrobi� wi�cej powt�rze� tego samego eksperymentu
			OutLog<<RepetNum<<'\t'; //Kt�ra to kolejna repetycja?
		OutLog<<(step_counter>0?step_counter:0.1)<<'\t'<<MeanAgres<<'\t'<<MeanHonor<<'\t'<<MeanCallPolice<<'\t'<<MeanFeiReputation<<'\t'<<MeanPower<<'\t'<<LiczbaTrupow<<'\t'<<Actions.NOTHING<<'\t'<<Actions.WITHDRAW<<'\t'<<Actions.GIVEUP<<'\t'<<Actions.HOOK<<'\t'<<Actions.FIGHT<<'\t'<<Actions.CALLAUTH<<endl;
		if(ConsoleLog)
		  cout<<step_counter<<'\t'<<MeanAgres<<'\t'<<MeanHonor<<'\t'<<MeanFeiReputation<<'\t'<<MeanCallPolice<<'\t'<<MeanPower<<'\t'<<endl;
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
	unsigned StartPow=(SIDE+1)*SSIZ+char_height('X')+1;//Gdzie si� zaczyna wizualizacja pomocnicza
	unsigned StartDyn=StartPow+(SIDE+1)*SSIZ+char_height('X')+1;//Gdzie si� zaczyna wizualizacja aktywno�ci

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
				set_brush_rgb(Ag.GetFeiReputation()*255,0,Ag.CallPolice*255);
				set_pen_rgb(  Ag.GetFeiReputation()*255,0,Ag.CallPolice*255,1,SSH_LINE_SOLID);
				unsigned ASiz=1+/*sqrt*/(Ag.Power)*VSIZ;
				fill_rect_d(h*VSIZ,v*VSIZ,h*VSIZ+ASiz,v*VSIZ+ASiz);
				if(Ag.Agres>0 || Ag.Honor>0)
					plot_rgb(h*VSIZ+ASiz/2,v*VSIZ+ASiz/2,Ag.Agres*255,Ag.Agres*255,Ag.Honor*255);
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
		}
	}

	printc(1,screen_height()-char_height('X'),100,255,"%u MC %s MnAgres=%f  MnHonor=%f  MnPoli=%f Killed=%u  AllKilled=%u  ",
										step_counter,"COMPONENT VIEW",
									double(MeanAgres),double(MeanHonor),double(MeanCallPolice),LiczbaTrupowDzis,LiczbaTrupow);
	/*double MeanFeiReputation=0;
double MeanCallPolice=0;
double MeanPower=0;*/
	printc(spw,(SIDE+1)*SSIZ,150,255,"%s Mn.Fe=%g Mn.Cp=%g ","Reput.",double(MeanFeiReputation),double(MeanCallPolice));   // HonorAgent::MaxReputation
	printc(spw,StartPow+(SIDE+1)*SSIZ,50,255,"%s mn=%f ","Power",double( MeanPower ));
	printc(spw,StartDyn+(SIDE+1)*SSIZ,50,255,"%s H:%u F:%u C:%u ","Local interactions",Actions.HOOK,Actions.FIGHT,Actions.CALLAUTH);
	//HonorAgent::Max...=Real...;//Aktualizacja max-�w do policzonych przed chwil� realnych
	flush_plot();

	save_stat();

	mouse_activity(old);
}


/*  OGOLNA FUNKCJA MAIN */
/************************/
void Help();
void SaveScreen(unsigned step);
void mouse_check(wb_dynmatrix<HonorAgent>& World);


int main(int argc,const char* argv[])
{
	cout<<MODELNAME<<" v.:"<<VERSIONNUM<<endl<<
		"==============================="<<endl<<
  //		"(programmed by Wojciech Borkowski from University of Warsaw)\n"
		"        "<<endl
		<<endl;
	cout<<"\nuse -help for command line options for graphics\n"<<endl;

	mouse_activity(1);
	set_background(255);
	buffering_setup(1);// Czy wlaczona animacja
	shell_setup(MODELNAME,argc,argv);

	cout<<"\n MODEL CONFIGURATION: "<<endl;
	//cout<<"World is "<<SIDE<<'x'<<SIDE<<" of "<<endl;//SIDE*SIDE to rozmiar �wiata symulacji
	Parameters_dump(cout);


	if(!init_plot(SIDE*VSIZ+20+SIDE*SSIZ,SIDE*VSIZ,0,1)) //Na g��wn� wizualizacj� swiata i jakie� boki
	{
		cerr<<"Can't initialize graphics"<<endl;
		exit(1);
	}

	RANDOMIZE();//Musi by� chyba �e chce si� powtarzalnie debugowa�!
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
			case 'n':runs=0;one_step(World);replot(World);break;
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
			case EOF:cont=0;break;
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
				if(SOCIAL_IMPACT_INTENSITY_PERCENT>0)
					social_impact_step(World,SOCIAL_IMPACT_INTENSITY_PERCENT);

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
	cout<<endl<<"Bye, bye!"<<endl;
	close_plot();
	return 0;
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

void SaveScreen(unsigned step)
{
	wb_pchar Filename;
	Filename.alloc(128);
	Filename.prn("%u_honorwrld",step);
	dump_screen(Filename.get_ptr_val());
}

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
