
#include <Adafruit_GFX.h>
#include <UTFTGLUE.h>


#define LCD_CS A3               // Chip Select goes to Analog 3
#define LCD_CD A2               // Command/Data goes to Analog 2
#define LCD_WR A1               // LCD Write goes to Analog 1
#define LCD_RD A0               // LCD Read goes to Analog 0
#define LCD_RESET A4            // Can alternately just connect to Arduino's reset pin
UTFTGLUE myGLCD(0x9488, LCD_CD, LCD_WR, LCD_CS, LCD_RESET, LCD_RD);

#define DISPH    479            // Hauteur ecran en pixels
#define DISPW    319            // Largeur ecran en pixels
#define NEXTPIN  51             // Broche d'entrée pour le bouton Vue Suivante

#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF
#define GREY    0x7BEF
#define DGREY   0x0202

#define NOALM   0
#define ALMLL   1
#define ALML    2
#define ALMH    3
#define ALMHH   4

#define ANGMIN  2.967           //170° en Radians
#define ANGMAX  6.457           //370° en Radians
#define ANGZON  3.490           //200° en Radians
#define POIBA   0.5             //28° en Radians

#define NBMES    10             // Nombre de mesures en provenance de la centrale d'acquisition
#define MSGSIZ   66             // Taille du message de la centrale d'acquisition   
#define HISTNB   180            // Nombre de points historisés
#define HISTPER  10000          // Periode d'historisation en millisecondes
#define HISTREF  30000          // Periode de raffraichissement historiques en millisecondes
#define HISTSIZ  2              // Taille des afficheurs dans les vues historique
#define NBSCALE  7              // Nombre d'échelles d'historique 

typedef struct {
  char   Name[10];   char   Unit[6];
  int    DisWid;     int    DisDec;     short DisCol;     int   TicNb;  // Format : Nb digit, Nb decimales, Couleur d'affichage, Nombre de traits
  float  Value;      float  Min;        float Max;        float DeadB;  // Valeur,  Min/Max et Bande morte
  float  ThrLL;      float  ThrL;       float ThrH;       float ThrHH;  // Seuils d'alarmes
  short    Alarm;                                                       // Valeur Etat d'alarme (NOALM,ALML,ALMLL,ALMH,ALMHH)
  int HistScale;     int HistCol;       short    Histo [HISTNB];        // Tableau  d'historique,  valeurs max de visu courbe, couleur courbes
} StrVal ;                                                              // Structure de stockage d'une valeur de mesure

typedef struct {
  int    Rad;        int   PoiRad;                                      // Rayon, Rayon du pointeur
  float  TicRat;     float ArcRat;                                      // Ratio diametre des tics et arcs de couleur
  int    DisSiz;     int   UniSiz;     int   MaxSiz;                    // Taille caracteres afficheur, Unité, Maxi
} StrIPar ;                                                             // Structure de definition d'un type afficheur

typedef struct {
  int    PosX;       int   PosY;                                        // Position afficheur (Centre Pointeur)
  float  MemVal;     float MemAlphaR;                                   // Memoires pour limiter les raffraichissements
  StrVal *Val;                                                          // Lien avec la mesure
  StrIPar *Par;                                                         // Lien avec la definition le type d'afficheur
} StrIndic ;                                                            // Structure de positionnement d'un afficheur


typedef struct {
  int  Min;         int Max;                                            // Min/Max pour echelle d'historique
  short Col;        char Unit[6];                                       // Unité et couleur pour Historique
} StrScale ;
           
// Parametres d'affichages
#define SCRCOL   BLACK          // Couleur de fond d écran
#define UNICOL   CYAN           // Couleur des unités des indicateurs
#define TICCOL   WHITE          // Couleur des Marques des indicateurs
#define BCKCOL   BLACK          // Couleur de fond des indicateurs
#define POICOL   WHITE          // Couleur des aiguilles des indicateurs

static StrVal    Mes[NBMES];    // Table des structures des mesures
static StrIPar   IndTyp[4];     // Table des structures des formats d'indicateurs
static StrIndic  Ind[10];       // Table des indicateurs de la page visualisée  (Maxi 10 indicateurs ronds)
static int       NbDis[3];      // Nombre d'indicateurs des pages
static int       DispNb;        // Numéro de la vue en cours

static int       IDat [NBMES];  // Table des données d'entrée converties en entier
static StrScale  HistScale[10]; // Table des échelles h'historique
static int       MemCpt;        // Memoire compteur de vie
static long      NbErr;         // Nombre d'erreurs de recption détectées (Nb de cycles sans chgt de compteur de vie)
static long      LastHist;      // Memo instant dernier enregistrement historique
static long      LastHistRef;   // Memo instant dernier instant de raffraichissement vue historique
static int       PosHisto;      // Position historique en cours
static bool      PrevSt;        // Ancien etat entrée demande de changenment de vue
static int       CurvH;         // Hauteur utile pour les tracés de courbes 
static int       CurvB;         // Coordonnée de la base des courbes 


//**********************************************************************************//
// Impression nombre avec format (Attention Nb De Digit<10 sinon ca plante)         //
//**********************************************************************************//
void FmtNb (char * str, float Val, int DigitNb, int DecNb)
{
  int   EntNb, i;
  float Tmp;
  char  Fmt[5];
  char  Stmp[20];

  if (DecNb == 0) EntNb = DigitNb;
  else EntNb = DigitNb - DecNb - 1;
  strcpy(Fmt, "%0vd"); Fmt[2] = char(EntNb + 48);
  sprintf(Stmp, Fmt, (int)Val);

  if (strlen(Stmp) > EntNb)
  {
    for (i = 0; i < DigitNb; i++) str[i] = 'E';
    str[DigitNb] = 0;
  }
  else
  {
    strcpy(str, Stmp);
    if (DecNb > 0)
    {
      Tmp = (int)(abs(Val)); Tmp = abs(Val) - Tmp; Tmp = Tmp * pow(10, DecNb); // A ce stade la partie entiere = valeur decimale de val
      strcat (str, ".");
      sprintf(Stmp, "%d", (int)Tmp);
      strcat (str, Stmp);
      if (Val < 0) str[0] = '-';
    }
  }
}

//**********************************************************************************//
// Mise à jour valeur d'une mesure                                                  //
//**********************************************************************************//

void MesUpd (StrVal *Mes, int iVal, bool Rec)
{
  float TH, TL, THH, TLL, DB, Val;

  switch (Mes->DisDec)
  {
    case 1:
      Val = 0.1 * iVal;
      break;
    case 2 :
      Val = 0.01 * iVal;
      break;
    case 3 :
      Val = 0.001 * iVal;
      break;
    default:
      Val = iVal;
  }

  TLL = Mes->ThrLL;  TL = Mes->ThrL;
  TH = Mes->ThrH;   THH = Mes->ThrHH;
  DB = Mes->DeadB;

  // Recadrage de la valeur dans la plage
  if (Val <= Mes->Min) Val = Mes->Min;
  else if (Val >= Mes->Max) Val = Mes->Max;
  Mes->Value = Val;

  // Recalcul des seuils pour prendre en compte l'hysteresis
  if      (Mes->Alarm == ALMLL) TLL += DB;
  else if (Mes->Alarm == ALML)  TL += DB;
  else if (Mes->Alarm == ALMHH) THH -= DB;
  else if (Mes->Alarm == ALMH)  TH -= DB;

  if      (Val < TLL)  Mes->Alarm = ALMLL;
  else if (Val > THH)  Mes->Alarm = ALMHH;
  else if (Val < TL)   Mes->Alarm = ALML;
  else if (Val > TH)   Mes->Alarm = ALMH;
  else                 Mes->Alarm = NOALM;

  // Historisation de la valeur si demandé
 if ( Rec )
  {
    Mes->Histo[PosHisto] = iVal;
  }

}


//**********************************************************************************//
// Initialisation d'un indicateur                                                   //
//**********************************************************************************//

void IndicInit (StrIndic *Indic)
{
  float val, alphaR, temp, temp1;
  int  PosX, PosY ;
  char str[8];
  PosX = Indic->PosX;
  PosY = Indic->PosY;

  Indic->MemVal = Indic->Val->Value + 1.1 * Indic->Val->DeadB;

  // Tracage Fond d'afficheur
  myGLCD.setColor(BCKCOL);
  myGLCD.fillCircle(PosX, PosY, Indic->Par->Rad);

  // Tracage des zones de couleurs
  for (val = Indic->Val->Min; val < Indic->Val->Max; val = val + (Indic->Val->Max - Indic->Val->Min) / (Indic->Par->Rad))
  {
    alphaR = ANGMIN + ANGZON * (val - Indic->Val->Min) / (Indic->Val->Max - Indic->Val->Min);

    if (val < Indic->Val->ThrLL)  myGLCD.setColor(RED);
    else if (val < Indic->Val->ThrL)  myGLCD.setColor(YELLOW);
    else if (val > Indic->Val->ThrHH)  myGLCD.setColor(RED);
    else if (val > Indic->Val->ThrH)  myGLCD.setColor(YELLOW);
    else myGLCD.setColor(Indic->Val->DisCol);
    temp = Indic->Par->Rad * Indic->Par->ArcRat;
    myGLCD.drawLine(PosX + temp * cos(alphaR), PosY + temp * sin(alphaR), PosX + Indic->Par->Rad * cos(alphaR), PosY + Indic->Par->Rad * sin(alphaR));
  }

  // Tracage du cercle et des marques
  myGLCD.setColor(TICCOL);
  for (alphaR = ANGMIN ; alphaR <= (ANGMAX + 0.0001) ; alphaR = alphaR + ANGZON / ((float)Indic->Val->TicNb - 1))
  {
    temp = Indic->Par->Rad * Indic->Par->TicRat;
    myGLCD.drawLine(PosX + temp * cos(alphaR), PosY + temp * sin(alphaR), PosX + Indic->Par->Rad * cos(alphaR), PosY + Indic->Par->Rad * sin(alphaR));
  }
  myGLCD.drawCircle(PosX, PosY, Indic->Par->Rad);

  // Affichage Min / Max
  myGLCD.setBackColor(BCKCOL);
  myGLCD.setTextSize(Indic->Par->MaxSiz);
  sprintf (str, "%d", int(Indic->Val->Min));
  myGLCD.print(str, PosX - 0.93 * Indic->Par->Rad, PosY + 0.2 * Indic->Par->Rad);
  sprintf (str, "%d", int(Indic->Val->Max));
  myGLCD.print(str, 2 + PosX + 0.93 * Indic->Par->Rad - strlen(str)*Indic->Par->MaxSiz * 6, PosY + 0.2 * Indic->Par->Rad);

  // Tracage Ligne de base
  myGLCD.setColor(SCRCOL);
  temp = 12 + Indic->Par->Rad / 10 + Indic->Par->DisSiz * 8;
  temp1 = sqrt (((float)Indic->Par->Rad * (float)Indic->Par->Rad - temp * temp));
  temp = PosY + temp;
  myGLCD.fillRect(PosX - Indic->Par->Rad, temp , PosX + Indic->Par->Rad, PosY + Indic->Par->Rad);
  myGLCD.setColor(TICCOL);
  myGLCD.drawLine(PosX - (int)temp1, temp , PosX + (int)temp1, temp);
}
// Fin Initialisation d'un Indicateur **********************************************//

//**********************************************************************************//
// Raffraichissement d'un indicateur avec une nouvelle valeur                       //
//**********************************************************************************//
void IndicRefresh (StrIndic *Indic)
{
  float MemAlphaR, alphaR, Val, temp;
  int x, y, x0, y0, x1, y1, x2, y2, xp, yp, xp0, yp0, xp1, yp1, xp2, yp2, PoiLg, PosX, PosY;
  char str[10];

  Val = Indic->Val->Value;
  MemAlphaR = Indic->MemAlphaR ;
  PosX = Indic->PosX;  PosY = Indic->PosY;
  PoiLg = 0.97 * Indic->Par->TicRat * Indic->Par->Rad;

  // Affichage Numerique
  switch (Indic->Val->Alarm)
  {
    case ALML:
    case ALMH:  myGLCD.setColor(YELLOW); break;
    case ALMLL:
    case ALMHH: myGLCD.setColor(RED);  break;
    default :   myGLCD.setColor(Indic->Val->DisCol);
  }
  myGLCD.setBackColor(BCKCOL);
  myGLCD.setTextSize(Indic->Par->DisSiz);
  FmtNb (str, Val, Indic->Val->DisWid, Indic->Val->DisDec);
  myGLCD.print(str, PosX - Indic->Val->DisWid * 3 * Indic->Par->DisSiz, PosY + 10 + Indic->Par->Rad / 10);

  // Affichage aiguille si ecart depuis dernier affichage superieur à bande morte
  if (abs(Val - Indic->MemVal) > Indic->Val->DeadB)
  {
    alphaR =    ANGMIN + ANGZON * ((Val - Indic->Val->Min) / (Indic->Val->Max - Indic->Val->Min));
    x =  PosX + PoiLg * cos(alphaR);
    y =  PosY + PoiLg * sin(alphaR);
    xp = PosX + PoiLg * cos(MemAlphaR);
    yp = PosY + PoiLg * sin(MemAlphaR);

    x0 = PosX + Indic->Par->PoiRad * cos(alphaR);
    y0 = PosY + Indic->Par->PoiRad * sin(alphaR);
    xp0 = PosX + Indic->Par->PoiRad * cos(MemAlphaR);
    yp0 = PosY + Indic->Par->PoiRad * sin(MemAlphaR);

    x1 = PosX + Indic->Par->PoiRad * cos(alphaR - POIBA);
    y1 = PosY + Indic->Par->PoiRad * sin(alphaR - POIBA);
    xp1 = PosX + Indic->Par->PoiRad * cos(MemAlphaR - POIBA);
    yp1 = PosY + Indic->Par->PoiRad * sin(MemAlphaR - POIBA);

    x2 = PosX + Indic->Par->PoiRad * cos(alphaR + POIBA);
    y2 = PosY + Indic->Par->PoiRad * sin(alphaR + POIBA);
    xp2 = PosX + Indic->Par->PoiRad * cos(MemAlphaR + POIBA);
    yp2 = PosY + Indic->Par->PoiRad * sin(MemAlphaR + POIBA);


    // Effacement ancien tracé
    myGLCD.setColor(BCKCOL);
    myGLCD.drawLine(xp0, yp0, xp, yp);
    myGLCD.drawLine(xp1, yp1, xp, yp);
    myGLCD.drawLine(xp2, yp2, xp, yp);

    // Réaffichage unité
    myGLCD.setColor(UNICOL);


    // JJR        temp = PosY - Indic->Par->PoiRad - Indic->Par->UniSiz * 8 - Indic->Par->Rad / 20;
    myGLCD.setTextSize(1);

    // JJR        myGLCD.print(Indic->Val->Unit, PosX - strlen(Indic->Val->Unit)*Indic->Par->UniSiz * 3, (int)temp);
    myGLCD.print(Indic->Val->Unit, PosX + 3 * Indic->Par->PoiRad , PosY - Indic->Par->PoiRad ) ;

    myGLCD.setTextSize(Indic->Par->UniSiz);

    // JJR myGLCD.print(Indic->Val->Name, PosX - strlen(Indic->Val->Name)*Indic->Par->UniSiz * 3, (int)temp - Indic->Par->UniSiz * 12);
    myGLCD.print(Indic->Val->Name, PosX - strlen(Indic->Val->Name)*Indic->Par->UniSiz * 3,  PosY - 0.4 * (Indic->Par->Rad + 8 * Indic->Par->UniSiz) );

    // Nouveau Tracé
    myGLCD.setColor(POICOL);
    myGLCD.fillCircle(PosX, PosY, Indic->Par->PoiRad);
    myGLCD.drawLine(x0, y0, x, y);
    myGLCD.drawLine(x1, y1, x, y);
    myGLCD.drawLine(x2, y2, x, y);

    // Memorisation anciennes valeurs
    Indic->MemVal = Val;
    Indic->MemAlphaR = alphaR;
  }
}
// Fin raffraichissement d'un indicateur avec une nouvelle valeur **********************//

//**************************************************************************************//
// Afficheur Numerique simple
//**************************************************************************************//
void DisplayVal (StrVal *Val, int PosX, int PosY, int Siz)
{
  char str[10];

  // Affichage Numerique
  switch (Val->Alarm)
  {
    case ALML:
    case ALMH:  myGLCD.setColor(YELLOW); break;
    case ALMLL:
    case ALMHH: myGLCD.setColor(RED);  break;
    default :   myGLCD.setColor(Val->DisCol);
  }
  myGLCD.setBackColor(BCKCOL);
  myGLCD.setTextSize(Siz);
  FmtNb (str, Val->Value, Val->DisWid, Val->DisDec);
  myGLCD.print(str, PosX , PosY);
}
// Fin raffraichissement d'un indicateur avec une nouvelle valeur **********************//

//**************************************************************************************//
// Affichage du compteur de vie en haut a droite taille 2
//**************************************************************************************//

void DisplayAliveCounter (int Cpt)
{

  char str[3];

  if (Cpt == MemCpt)
  {
    if (NbErr > 10)
    {
      myGLCD.setColor(WHITE);
      myGLCD.setBackColor(RED);
    }
    else
      NbErr = NbErr + 1;
  }
  else
  {
    NbErr = 0;
    if (( Cpt > 0) && (abs(Cpt - MemCpt) > 1))
    {
      myGLCD.setColor(BLACK);
      myGLCD.setBackColor(YELLOW);
    }
    else
    {
      myGLCD.setColor(DGREY);
      myGLCD.setBackColor(SCRCOL);
    }
  }
  myGLCD.setTextSize(2);
  FmtNb (str, Cpt, 2, 0);
  myGLCD.print(str, DISPW - 24 , 0);
  MemCpt = Cpt;
}

//Fin affichage compteur de vie*********************************************************//

//**************************************************************************************//
// Affichage d'une courbe d'historique
//**************************************************************************************//
void DrawCurve (StrVal* Mes, int x0, int y0,  int Width, int Height )

  {
  long j, Pos, x, xp, y, yp;
  float Scale, Min, C;
  float ValP;
  if (Mes->HistScale > 0)
    {
    myGLCD.setColor(Mes->HistCol);
    xp=x0; yp=y0;
    switch (Mes->DisDec)
      {
      case 1:  C = 10  ; break;
      case 2 : C = 100 ; break;
      case 3 : C = 1000; break;
      default: C = 1;
      }
    Min = C * HistScale[Mes->HistScale].Min;
    Scale =  (C * HistScale[Mes->HistScale].Max) - Min;

    Pos = PosHisto;
    for (j = 0; j < HISTNB; j++)
      {
      x = x0 + (j * Width) / HISTNB;
      
      ValP = ( Mes->Histo[Pos] - Min) / Scale;
      if (ValP>1) ValP=1;
      if (ValP<0) ValP=0;

      y = y0 - Height * ValP ;

      if (j > 0) myGLCD.drawLine(xp, yp, x, y);
      xp = x;
      yp = y;
      Pos++;
      if (Pos == HISTNB ) Pos = 0;
      }
    }
  }

//Fin affichage d'une courbe************************************************************//

//**************************************************************************************//
// Affichage de la vue trend
//**************************************************************************************//
void DisplayTrends ()
  {
  int i,x,y,a;
  char txt[11];
  a= HISTPER / 1000;
  a= a * HISTNB / 60 ;
  // Lister les variables
  myGLCD.setColor(TICCOL);
  myGLCD.setBackColor(SCRCOL);
     
  myGLCD.print("Time:     Min",0,0);
  sprintf( txt, "%d" , a  );
  myGLCD.print(txt,72,0);
  
  
  y = 12 * HISTSIZ ;
  
  // Impression des noms et unités de mesure
  for (i = 0; i < NBMES; i++)
    {
    if (Mes[i].HistScale > 0)
      {
      a = i % 2 ;  // Valeur 1 pour les cycles impaires
      x = a * (DISPW / 2 + 6 * HISTSIZ);
      myGLCD.setColor(Mes[i].HistCol);
      myGLCD.setTextSize (HISTSIZ);
      myGLCD.print(Mes[i].Name,x,y);
      
      x = x + 62 * HISTSIZ;
      myGLCD.setTextSize (1);
      myGLCD.print(Mes[i].Unit,x,y);
      
      y= y + 8* HISTSIZ * a;
      }
    }

  // Impression des échelles

  y=y+8*HISTSIZ;
  x=0;
  myGLCD.setTextSize (2);
  for (i = 1; i < NBSCALE; i++)
    {
    myGLCD.setColor(HistScale[i].Col);
    myGLCD.print(HistScale [i].Unit,x,y);
    
    sprintf (txt, "%d" , HistScale [i].Max);
    myGLCD.print(txt,x,y+12*HISTSIZ);
    
    sprintf (txt, "%d" , HistScale [i].Min);
    myGLCD.print(txt, x, DISPH - 8*HISTSIZ);
 
    x = x + DISPW / (NBSCALE-1);
    }

  CurvH = DISPH - 64 - y;
  CurvB = DISPH -  8*HISTSIZ - 5 ;
  }



// ------------------------------Raffraichissement de la vue historique-----------------------------
// Affichage des valeurs actuelles.
// Raffraichissement ds courbes uniquement si le flag refresh curve est positionné

void RefreshTrends ( bool RefCurve )
  {
  int x , y , i , a;
  
  y= 12 * HISTSIZ;                          // On laisse une  ligne de titre

  // Raffraichir les données temps réel
  for (i = 0; i < NBMES; i++) 
    {
    a = i % 2 ;  // Valeur 1 pour les cycles impaires 0 sinon
    x = a * (DISPW / 2 + 6 * HISTSIZ) + 36 * HISTSIZ ;
    DisplayVal ( &Mes[i], x, y, HISTSIZ) ; // Affichage dernière valeurs dispo avec couleur d'alarme
    y= y + 8* HISTSIZ * a;
    }   

    // Tracer les courbes si raffraichissement courbe demandé
  
  if (RefCurve)
    {
    
    // Tracer un cadre pour les trends
    myGLCD.setColor(SCRCOL); 
    myGLCD.fillRect(0, CurvB , DISPW, CurvB - CurvH);
    myGLCD.setColor(TICCOL);
    myGLCD.drawRect (0, CurvB , DISPW, CurvB - CurvH);

    for (i = 0; i < NBMES; i++)  DrawCurve ( &Mes[i], 0, CurvB, DISPW, CurvH);
    }
  }
 
//**************************************************************************************//
// ROUTINE D'INITIALISATION
//**************************************************************************************//
void setup()
{
  int i;
  //----------------------------Initialisation des echelles d'historique--------------_-

#define HRPM  1  // Vitesse moteur
  HistScale [HRPM].Min = 0;  HistScale [HRPM].Max = 5000; strcpy(HistScale [HRPM].Unit , "Rpm"); HistScale [HRPM].Col = RED; 
#define HPRS  2  // Pression 
  HistScale [HPRS].Min = 0;  HistScale [HPRS].Max = 1000; strcpy(HistScale [HPRS].Unit , "kPa"); HistScale [HPRS].Col = YELLOW;
#define HLTP  3  // Basse Temperature 
  HistScale [HLTP].Min = 0;  HistScale [HLTP].Max = 150;  strcpy(HistScale [HLTP].Unit , "Deg"); HistScale [HLTP].Col = GREEN;
#define HHTP  4  // Haute Temperature 
  HistScale [HHTP].Min = 0;  HistScale [HHTP].Max = 1000; strcpy(HistScale [HHTP].Unit , "Deg"); HistScale [HHTP].Col = WHITE;
#define HTEN  5  // Tension Batterie 
  HistScale [HTEN].Min = 8;  HistScale [HTEN].Max = 16;   strcpy(HistScale [HTEN].Unit , "V");   HistScale [HTEN].Col = MAGENTA;
#define HINT  6  // COurant Batterie 
  HistScale [HINT].Min =-30; HistScale [HINT].Max = 30;   strcpy(HistScale [HINT].Unit , "A");   HistScale [HINT].Col = MAGENTA;
  
  //----------------------------Initialisation des formats d'afficheurs-------------------
#define TGRAND 0  // Très Grand afficheur-------------------------------------------------
  IndTyp[TGRAND].Rad = 146;       IndTyp[TGRAND].PoiRad = 10;
  IndTyp[TGRAND].TicRat = 0.8;     IndTyp[TGRAND].ArcRat = 0.85;
  IndTyp[TGRAND].DisSiz = 8;       IndTyp[TGRAND].UniSiz = 3;      IndTyp[TGRAND].MaxSiz = 1;
#define GRAND 1  // Grand afficheur-------------------------------------------------------
  IndTyp[GRAND].Rad = 114;        IndTyp[GRAND].PoiRad = 12;
  IndTyp[GRAND].TicRat = 0.8;      IndTyp[GRAND].ArcRat = 0.85;
  IndTyp[GRAND].DisSiz = 6;        IndTyp[GRAND].UniSiz = 2;       IndTyp[GRAND].MaxSiz = 1;
#define MOYEN 2  // Moyen afficheur-------------------------------------------------------
  IndTyp[MOYEN].Rad = 75;        IndTyp[MOYEN].PoiRad = 6;
  IndTyp[MOYEN].TicRat = 0.8;    IndTyp[MOYEN].ArcRat = 0.85;
  IndTyp[MOYEN].DisSiz = 4;      IndTyp[MOYEN].UniSiz = 2;     IndTyp[MOYEN].MaxSiz = 1;
#define PETIT 3  // Petit afficheur-------------------------------------------------------
  IndTyp[PETIT].Rad = 51;         IndTyp[PETIT].PoiRad = 6;
  IndTyp[PETIT].TicRat = 0.8;      IndTyp[PETIT].ArcRat = 0.85;
  IndTyp[PETIT].DisSiz = 3;        IndTyp[PETIT].UniSiz = 2;       IndTyp[PETIT].MaxSiz = 1;

  //----------------------------Initialisation de la table des Mesures---------------------
#define RPM 0  // Mesure vitesse moteur----------------------------------------------------
  strcpy(Mes[RPM].Name, "RPM");  strcpy(Mes[RPM].Unit, "Rpm");
  Mes[RPM].DisWid = 4;    Mes[RPM].DisDec = 0;  Mes[RPM].TicNb = 9;   Mes[RPM].DisCol = GREEN;
  Mes[RPM].Value = 0;     Mes[RPM].Min = 0;     Mes[RPM].Max = 5000;   Mes[RPM].DeadB = 200;
  Mes[RPM].ThrLL = 1500;  Mes[RPM].ThrL = 1800; Mes[RPM].ThrH = 3700;  Mes[RPM].ThrHH = 3900;
  Mes[RPM].HistScale = HRPM; Mes[RPM].HistCol = RED;

#define WT 1  // Mesure Temperature Eau---------------------------------------------------
  strcpy(Mes[WT].Name, "Wat T");  strcpy(Mes[WT].Unit, "Deg");
  Mes[WT].DisWid = 3;     Mes[WT].DisDec = 0;   Mes[WT].TicNb = 13;    Mes[WT].DisCol = GREEN;
  Mes[WT].Value = 0;      Mes[WT].Min = 0;      Mes[WT].Max = 120;     Mes[WT].DeadB = 3;
  Mes[WT].ThrLL = 45;     Mes[WT].ThrL = 55;    Mes[WT].ThrH = 90;     Mes[WT].ThrHH = 98;
  Mes[WT].HistScale = HLTP;  Mes[WT].HistCol = GREEN;

#define OP 2  // Mesure Pression Huile----------------------------------------------------
  strcpy(Mes[OP].Name, "Oil P");  strcpy(Mes[OP].Unit, "kPa");
  Mes[OP].DisWid = 3;     Mes[OP].DisDec = 0;   Mes[OP].TicNb = 8;    Mes[OP].DisCol = GREEN;
  Mes[OP].Value = 0;      Mes[OP].Min = 0;      Mes[OP].Max = 700;    Mes[OP].DeadB = 30;
  Mes[OP].ThrLL = 100;    Mes[OP].ThrL = 200;   Mes[OP].ThrH = 600;    Mes[OP].ThrHH = 650;
  Mes[OP].HistScale = HPRS; Mes[OP].HistCol = YELLOW;

#define OT 3  // Mesure Temperature Huile-------------------------------------------------
  strcpy(Mes[OT].Name, "Oil T");  strcpy(Mes[OT].Unit, "Deg");
  Mes[OT].DisWid = 3;     Mes[OT].DisDec = 0;   Mes[OT].TicNb = 16;    Mes[OT].DisCol = GREEN;
  Mes[OT].Value = 0;      Mes[OT].Min = 0;      Mes[OT].Max = 150;     Mes[OT].DeadB = 3;
  Mes[OT].ThrLL = 45;     Mes[OT].ThrL = 55;    Mes[OT].ThrH = 110;    Mes[OT].ThrHH = 120;
  Mes[OT].HistScale = HLTP;  Mes[OT].HistCol = YELLOW;

#define AP 4  // Mesure Pression Turbo----------------------------------------------------
  strcpy(Mes[AP].Name, "Air P");  strcpy(Mes[AP].Unit, "kPa");
  Mes[AP].DisWid = 3;     Mes[AP].DisDec = 0;   Mes[AP].TicNb = 7;     Mes[AP].DisCol = GREEN;
  Mes[AP].Value = 0;      Mes[AP].Min = 0;      Mes[AP].Max = 300;     Mes[AP].DeadB = 5;
  Mes[AP].ThrLL = 50;     Mes[AP].ThrL = 75;    Mes[AP].ThrH = 220;    Mes[AP].ThrHH = 240;
  Mes[AP].HistScale = HPRS; Mes[AP].HistCol = CYAN;

#define AT 5  // Mesure Temperature Air -------------------------------------
  strcpy(Mes[AT].Name, "Air T");  strcpy(Mes[AT].Unit, "Deg");
  Mes[AT].DisWid = 3;     Mes[AT].DisDec = 0;   Mes[AT].TicNb = 16;    Mes[AT].DisCol = GREEN;
  Mes[AT].Value = 0;      Mes[AT].Min = 0;      Mes[AT].Max = 150;     Mes[AT].DeadB = 3;
  Mes[AT].ThrLL = 0;      Mes[AT].ThrL = 0;     Mes[AT].ThrH = 110;    Mes[AT].ThrHH = 130;
  Mes[AT].HistScale = HLTP;  Mes[AT].HistCol = CYAN;

#define EGT 6  // Mesure Temperature gaz echappement---------------------------------------
  strcpy(Mes[EGT].Name, "EGT");  strcpy(Mes[EGT].Unit, "Deg");
  Mes[EGT].DisWid = 4;      Mes[EGT].DisDec = 0;    Mes[EGT].TicNb = 11;       Mes[EGT].DisCol = GREEN;
  Mes[EGT].Value = 0;       Mes[EGT].Min = 0;       Mes[EGT].Max = 1000;       Mes[EGT].DeadB = 3;
  Mes[EGT].ThrLL = 0;       Mes[EGT].ThrL = 300;    Mes[EGT].ThrH = 700;       Mes[EGT].ThrHH = 750;
  Mes[EGT].HistScale = HHTP;   Mes[EGT].HistCol = WHITE;

#define CT 7  // Mesure Temperature Compartiment Moteur------------------------------------
  strcpy(Mes[CT].Name, "Mot T");  strcpy(Mes[CT].Unit, "Deg");
  Mes[CT].DisWid = 3;     Mes[CT].DisDec = 0;   Mes[CT].TicNb = 11;    Mes[CT].DisCol = GREEN;
  Mes[CT].Value = 0;      Mes[CT].Min = 0;      Mes[CT].Max = 100;     Mes[CT].DeadB = 3;
  Mes[CT].ThrLL = -30;    Mes[CT].ThrL = -20;   Mes[CT].ThrH = 70;     Mes[CT].ThrHH = 80;
  Mes[CT].HistScale = HLTP;  Mes[CT].HistCol = GREY;

#define BAV 8  // Mesure Tension Batterie-------------------------------------------------
  strcpy(Mes[BAV].Name, "Bat");  strcpy(Mes[BAV].Unit, "V");
  Mes[BAV].DisWid = 4;    Mes[BAV].DisDec = 1;  Mes[BAV].TicNb = 9;    Mes[BAV].DisCol = GREEN;
  Mes[BAV].Value = 0;     Mes[BAV].Min = 8 ;    Mes[BAV].Max = 16;      Mes[BAV].DeadB = 0.1;
  Mes[BAV].ThrLL = 11;    Mes[BAV].ThrL = 11.5; Mes[BAV].ThrH = 14;     Mes[BAV].ThrHH = 15;
  Mes[BAV].HistScale = HTEN;  Mes[BAV].HistCol = MAGENTA;

#define BAI 9  // Mesure Courant de charge Batterie---------------------------------------
  strcpy(Mes[BAI].Name, "Bat");  strcpy(Mes[BAI].Unit, "A");
  Mes[BAI].DisWid = 4;      Mes[BAI].DisDec = 1;    Mes[BAI].TicNb = 5;       Mes[BAI].DisCol = GREEN;
  Mes[BAI].Value = 0;       Mes[BAI].Min = -20;     Mes[BAI].Max = 20;        Mes[BAI].DeadB = 1;
  Mes[BAI].ThrLL = -5;      Mes[BAI].ThrL = -1;     Mes[BAI].ThrH = 10;      Mes[BAI].ThrHH = 15;
  Mes[BAI].HistScale = HINT;   Mes[BAI].HistCol = MAGENTA;

  //----------------------------------------------------------------------------------------

  // Initialisation des afficheurs page 0
  i = 0;
  //   Ind[i].PosX=115; Ind[i].PosY=115; Ind[i].Val=&Mes[RPM]; Ind[i].Par=&IndTyp[MOYEN]; i++;
  //   Ind[i].PosX=54;  Ind[i].PosY=300; Ind[i].Val=&Mes[OP];  Ind[i].Par=&IndTyp[PETIT];  i++;
  //   Ind[i].PosX=160; Ind[i].PosY=300; Ind[i].Val=&Mes[OT];  Ind[i].Par=&IndTyp[PETIT];  i++;
  //   Ind[i].PosX=266; Ind[i].PosY=300; Ind[i].Val=&Mes[WT];  Ind[i].Par=&IndTyp[PETIT];  i++;
  //   Ind[i].PosX=54;  Ind[i].PosY=400; Ind[i].Val=&Mes[AP]; Ind[i].Par=&IndTyp[PETIT];  i++;
  //   Ind[i].PosX=160; Ind[i].PosY=400; Ind[i].Val=&Mes[AT2]; Ind[i].Par=&IndTyp[PETIT];  i++;
  //   Ind[i].PosX=266; Ind[i].PosY=400; Ind[i].Val=&Mes[BAV]; Ind[i].Par=&IndTyp[PETIT];  i++;

  Ind[i].PosX = 80;  Ind[i].PosY = 76;  Ind[i].Val = &Mes[RPM]; Ind[i].Par = &IndTyp[MOYEN];  i++;
  Ind[i].PosX = 240; Ind[i].PosY = 76;  Ind[i].Val = &Mes[WT];  Ind[i].Par = &IndTyp[MOYEN];  i++;
  Ind[i].PosX = 80;  Ind[i].PosY = 204; Ind[i].Val = &Mes[OP];  Ind[i].Par = &IndTyp[MOYEN];  i++;
  Ind[i].PosX = 240; Ind[i].PosY = 204; Ind[i].Val = &Mes[OT];  Ind[i].Par = &IndTyp[MOYEN];  i++;
  Ind[i].PosX = 80;  Ind[i].PosY = 333; Ind[i].Val = &Mes[AP];  Ind[i].Par = &IndTyp[MOYEN];  i++;
  Ind[i].PosX = 240; Ind[i].PosY = 333; Ind[i].Val = &Mes[EGT]; Ind[i].Par = &IndTyp[MOYEN];  i++;

  Ind[i].PosX = 54;  Ind[i].PosY = 438; Ind[i].Val = &Mes[AT];  Ind[i].Par = &IndTyp[PETIT];  i++;
  Ind[i].PosX = 160; Ind[i].PosY = 438; Ind[i].Val = &Mes[BAV]; Ind[i].Par = &IndTyp[PETIT];  i++;
  Ind[i].PosX = 266; Ind[i].PosY = 438; Ind[i].Val = &Mes[BAI]; Ind[i].Par = &IndTyp[PETIT];  i++;
  NbDis[0] = i;


  
  // ---------------------------Initialisation afficheur TFT-------------------------------
  // myGLCD.InitLCD(LANDSCAPE);
  myGLCD.InitLCD(PORTRAIT);
  myGLCD.clrScr();
  myGLCD.setColor(SCRCOL);
  myGLCD.fillRect(0, 0, DISPW, DISPH);

  // Positionner la vue 0 par défaut
  DispNb = 0;
  
  // Tracer les indicateurs de la page 0 par défaut
  for (i = 0; i < NbDis[0]; i++) IndicInit (&Ind[i]);

  //  --------------------------Initialisation entrées logiques----------------------------
  pinMode( NEXTPIN , INPUT);      //  pin d'appel de la vue suivante  

  // ---------------------------Initialisation Compteur de vie et memoires temps ----------
  MemCpt = 0;
  LastHistRef = millis();
  LastHist = millis();
  PrevSt = true;
    
  // ---------------------------Initialisation ligne série---------------------------------
  Serial2.begin(4800);
  Serial2.setTimeout(300);
  Serial.begin(4800);

}


//**************************************************************************************//
// ROUTINE PRINCIPALE
//**************************************************************************************//


  
void loop()
{
  float f;
  char str [2];
  
  int i;
  int Compteur;
  
  String Msg;         // Variable message de reception

  char* Buff;         // Pointeur bufffer de reception
  long Now;           // Temps actuel en millisecondes
  bool Rec;           // Indicateur d'enregistrement historique
  bool Ref;           // Indicateur draffraicchissement courbes
  bool INext;;        // Entrée Demande de changement de vue

// Raz des demandes de raffraichissement et enregistrement
  Rec = false;
  Ref = false;
  
  // Lecture du messsage de la centrale
  Msg = Serial2.readStringUntil('\n');
  Buff = Msg.c_str();



  // Si taille de message correcte extraire les données dans un buffer local
  if ( strlen (Buff) == MSGSIZ)
  {
    
    sscanf(Buff, "%d %d %d %d %d %d %d %d %d %d %d",
           &Compteur,
           &IDat[BAI],
           &IDat[AT],
           &IDat[AP],
           &IDat[OP],
           &IDat[OT],
           &IDat[WT],
           &IDat[BAV],
           &IDat[CT],
           &IDat[EGT],
           &IDat[RPM]);
  }

  // Vérifier si à ce cycle on doit enregistrer les données historiques
  Now = millis();
  if (abs(Now - LastHist) > HISTPER)
  {
    LastHist = Now;
    Rec = true;
    // Incrémenter l'index d'enregistrement commun à toutes les mesures
    PosHisto++; 
    if (PosHisto == HISTNB) PosHisto = 0 ;
  }

  // Traiter les mesures (Alarmes, Filtres et historisation)
  for (i = 0; i < NBMES; i++)  MesUpd(&Mes[i], IDat[i], Rec);

  // Lire l'entrée changement de vue
 INext = digitalRead(NEXTPIN);
 
  // Traiter les demandes de changement de page
  if ((INext == false) && (PrevSt == true))               // Si front montant bouton de changement
    {
    myGLCD.clrScr();                                      // Effacer l'ecran
    myGLCD.setColor(SCRCOL); 
    myGLCD.fillRect(0, 0, DISPW, DISPH);

    if (DispNb == 0)                                      // si la vue 0 etait affichée
      {
      DispNb = 1;                                         // La  vue en cours devient la vue trend
      DisplayTrends ();                                   // Afficher les fonds de la vue trend
      Ref = true;                                         // On demande un raffraichissement des courbes
      }
    else
      {
      DispNb = 0;                                         // La  vue en cours devient la vue afficheur 0
      for (i = 0; i < NbDis[0]; i++) IndicInit (&Ind[i]); // retracer les afficheurs
      }
    }

  PrevSt = INext;

  // Raffraichissement des vues en temps réel----------------------------------------------------
  if ( DispNb == 0 )
    {
    // Raffraichissement vue Indicateurs: 
    // Raffraichir les indicateurs circulaires
    for (i = 0; i < NbDis[0]; i++)  IndicRefresh (&Ind[i]);
    
    // Afficher la temperature Compartiment moteur en haut à gauche
    DisplayVal (&Mes[CT], 0 , 0, 2);
    }
  else
    {
    // Raffraichissement vue Historique a faible frequence: 
    if (abs(Now - LastHistRef ) > HISTREF)
      {
      Ref = true;
      LastHistRef = Now;
      }
    RefreshTrends (Ref);
    }   
      
  // Dans tous les cas afficher le compteur de vie en haut a droite
  DisplayAliveCounter (Compteur);
}
