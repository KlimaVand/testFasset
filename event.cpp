/****************************************************************************\
  $Id$
  $HeadURL$
  $Author$
  $Revision$
  $Date$
\****************************************************************************/
#include <common.h>
#include "event.h"
#include <climclas.h>
#include <timeUse.h>
#include <output.h>
#include <contrparm.h>
/*#ifdef __ANSICPP__
#include <dir.h>
#endif
*/
/****************************************************************************\
  Default Constructor
\****************************************************************************/
eventControl::eventControl()
   : base()
{
   InitVariables();
}

/****************************************************************************\
  Constructor with arguments
\****************************************************************************/
eventControl::eventControl(const char * aName, const int aIndex, const base * aOwner)
   : base(aName, aIndex, aOwner)
{
   InitVariables();
}

/****************************************************************************\
  Destructor
\****************************************************************************/
eventControl::~eventControl()
{
   if (theOperational_manager)
      delete theOperational_manager;
   if (theCropRotation)
  	   delete theCropRotation;
   if (theLivestock)
    delete theLivestock;
}

/****************************************************************************\
   Only called by constructor, to avoid redundant code
\****************************************************************************/
void eventControl::InitVariables()
{
   theLivestock           = NULL;
   theCropRotation        = new cropRotation;
	theOperational_manager = new Operations_manager_class() ; // added by NJH
   CapitalInterest        = 0.0;                             // default interest on capital
   MinFieldArea           = 0.0;                             // lowest field area
   ScaleBuildings			  = 0;
   Nbudget.SetNames("farm","N"); // ???!!!
}

/****************************************************************************\
\****************************************************************************/
void eventControl::Initialize(bsTime * stopTime, char * climateFilename, char * outputPath)
{
	abort = false;
   char filenameCSV[100];
   char filenameField[100];
   char filenameCattle[100];

   sprintf(filenameCSV,"%s\\INDICATX.XLS",outputPath);
   sprintf(filenameField,"%s\\Fielddata.txt",outputPath);   //@@@
   sprintf(filenameCattle,"%s\\Cattledata.txt",outputPath); //@@@
   theOutput->Initialize(filenameCSV);

   int Watering = 1;
   int daysBetweenIrrigation = 0;
   int irrigationDelay = 0;
   //inserted by NJH Oct 2007
   int SelltheStrawProducts = 0;
   int SelltheNonRoughageProducts = 0;
   //
   bool pigfarm = false;
   bool dairyfarm = false;
   bool beeffarm = false;
   int FingerFlow = 0;
   string FarmID = "";

   SetCritical();
   if (OpenInputFile("farm.dat"))
   {
      FindSection("farm");
      UnsetCritical();
      GetParameter("CapitalInterest",&CapitalInterest);
      GetParameter("MinFieldArea",&MinFieldArea);
      GetParameter("Watering",&Watering);
      GetParameter("DaysBetweenIrrigation",&daysBetweenIrrigation);
      GetParameter("IrrigationDelay",&irrigationDelay);
      GetParameter("FingerFlow",&FingerFlow);
      GetParameter("ScaleBuildings",&ScaleBuildings);
   //inserted by NJH Oct 2007
      GetParameter("SelltheStrawProducts",&SelltheStrawProducts);
      GetParameter("SelltheNonRoughageProducts",&SelltheNonRoughageProducts);
      //
#ifdef TUPLE
      GetParameter("FarmID",&FarmID);
      theControlParameters->SetFarmID(FarmID);
#endif
      SetCritical();
      FindSection("livestock");
      bool dum=false;
      GetParameter("pigfarm",&dum);
      pigfarm = dum;
      dum=false;
      GetParameter("dairyfarm",&dum);
      dairyfarm = dum;
      dum=true;
      GetParameter("beeffarm",&dum);
      beeffarm = dum;
      CloseInputFile();
   }
   else
		theMessage->FatalError("Event:: error in opening farm initilization file");
   if ((beeffarm)&&(dairyfarm))
   	beeffarm=false;

   if (pigfarm ||dairyfarm || beeffarm)
      theLivestock = new livestock;

   theOutputControl->Initialize("output.dat");
   theProducts->InitializeProducts("products.dat");//,(dairyfarm || beeffarm));
   theProducts->InitializePrices("prices.dat",&theTime,stopTime);
   theClimate->InitClimate(climateFilename);
   theCropRotation->Initialize(1,Watering,daysBetweenIrrigation,irrigationDelay,
                               FingerFlow,0,MinFieldArea);
   theProducts->UpdateProductPtr();
   theTechnics->Initialize("operatio.dat","techeqp.dat");
   theBuildings->InitializeAll("building.dat", pigfarm, dairyfarm, beeffarm);
   if (theLivestock)
      theLivestock->Initialize("livestock.dat",pigfarm,dairyfarm,beeffarm,theBuildings, filenameCattle);
   theLegislation->Initialize("legislat.dat");
   theOperational_manager->Initialize(theLivestock,theCropRotation,theBuildings);    //added by NJH 1.5.00

   //inserted by NJH Oct 2007
   theProducts->SetSelltheStrawProducts(SelltheStrawProducts);
   theProducts->SetSelltheNonRoughageProducts(SelltheNonRoughageProducts);
      //


}

/****************************************************************************\
\****************************************************************************/
void eventControl::ReceivePlan(char * fileExtension, char *inputDir)
{
   char fileName[13];
   strcpy(fileName,"FIELDS.");
   strcat(fileName,fileExtension);

   theCropRotation->ReceivePlan(fileName);
   if (theLivestock)
   {
      theLivestock->ReceivePlan(fileExtension);
   }
   theOperational_manager->ReceivePlan(fileExtension,inputDir);
   //scale buildings after call to theOperational_manager->ReceivePlan, so know which buildings are in use

   if ((theLivestock)&&(!ScaleBuildings==0))
         theBuildings->ScaleBuildings();
}

/****************************************************************************\
Has not been used since 2002 - the LP side of the model is not updatet
\****************************************************************************/
void eventControl::BasicLPParameters()
{
#ifndef __ANSICPP__
   chdir("\\STRATEGI\\LP_MODEL");
#endif
   fstream * f = new fstream;;
	f->open("MINLAND.INC",ios::out);
   *f << "MINLAND The minimum size of land on one field with one crop" << endl;
   *f << "/ " << MinFieldArea << " /" << endl;
   f->close();
   delete f;
}

/****************************************************************************\
Has not been used since 2002 - the LP side of the model is not updatet
\****************************************************************************/
void eventControl::CalcLP(int periods)
{
   string numberString[] = {"zero ","one ","two ","three ","four "};
   cout << "Creating input files for " << numberString[periods] << "period LP-model" << endl;					// Transfer data to LP model
   if (periods>1)
   	BasicLPParameters();
// theProducts->CalcLP();
   theLivestock->CalcLP(periods);
// theCropRotation->CalcLP(periods);
   //theTechnics->CalcLP();
// theEconomics->CalcLP();
   theBuildings->CalcLP();
   theLegislation->CalcLP();
}

/****************************************************************************\
 Function:   Fixed plans generate fixed production plans from the GAMS output
\****************************************************************************/
void eventControl::CopyFile(const char* inputFileName, const char* outputFileName, const char* inputDir, const char* outputDir)
{
	fstream inputFile, outputFile;
   char ch;
   chdir(outputDir);
   outputFile.open(outputFileName,ios::out);
   chdir(inputDir);
   inputFile.open(inputFileName,ios::in);

   while (inputFile.get(ch))
      outputFile.put(ch);

   inputFile.close();
   outputFile.close();
}

/****************************************************************************\
\****************************************************************************/
void eventControl::RenameFile(string oldFileName,string newFileName, string extension, char * outputDir)
{
	string aNewFileName;
   aNewFileName = (string) outputDir  + newFileName + extension;
   remove(aNewFileName.c_str());
   if (rename(oldFileName.c_str(), aNewFileName.c_str()) == 0)
       cout << "Renamed " << oldFileName << " to " << aNewFileName << endl;
   else
       cout << "Renaming " << oldFileName << " to " << aNewFileName << " failed" << endl;
}

/****************************************************************************\
\****************************************************************************/
void eventControl::RenameFiles(char * outputDir, char* inputDir, int GenerateFixedPlans, int yearNumber)
{
   char extension[4];
   char newFileName[40];
   string oldFileName;
   oldFileName    = "";
   newFileName[0] = '\0';
   extension[0]   = '\0';

   sprintf(extension,"%02d",theTime.GetYear() % 100);
	RenameFile("FIELDS.LP1","FIELDS1.",extension, outputDir);
   RenameFile("FIELDS.LP3","FIELDS3.",extension, outputDir);
   RenameFile("PIGS.LP1","PIGS1.",extension, outputDir);
   RenameFile("PIGS.LP3","PIGS3.",extension, outputDir);
	RenameFile("PIGSFEED.LP1","PIGFEED1.",extension, outputDir);
   RenameFile("PIGSFEED.LP3","PIGFEED3.",extension, outputDir);
	RenameFile("DATAOUT.LP3","DATAOUT3.",extension, outputDir);
	RenameFile("DATAOUT.LP1","DATAOUT1.",extension, outputDir);
   if (GenerateFixedPlans)
   {
      sprintf(newFileName,"Fields.f%02d",yearNumber);
      oldFileName = "FIELDS1."+ (string) extension;
   	CopyFile(oldFileName.c_str(),newFileName, outputDir, inputDir);
      sprintf(newFileName,"pigs.f%02d",yearNumber);
   	oldFileName = "PIGS1."+ (string) extension;
   	CopyFile(oldFileName.c_str(),newFileName, outputDir, inputDir);
      sprintf(newFileName,"pigsfeed.f%02d",yearNumber);
   	oldFileName = "PIGFEED1."+ (string) extension;
   	CopyFile(oldFileName.c_str(),newFileName, outputDir, inputDir);
   }

   chdir(outputDir);
   RenameFile("RUN3.LST","RUN3.",extension,outputDir);
   RenameFile("RUN1.LST","RUN1.",extension,outputDir);
}

/****************************************************************************\
\****************************************************************************/
void eventControl::DailyUpdate()
{
/*   fstream * filehandle = theMessage->GiveDebugHandle();
   *filehandle << theTime.GetJulianDay() << " " ;
   theMessage->CloseDebugHandle();*/
   if((theTime.GetDay()==30) && (theTime.GetMonth()==9))// && (theTime.GetYear()==1989))
   	cout << " ";
   theClimate->Update();          //Load today's climate
	theOutput->DailyUpdate();
   theOperational_manager->daily_update();
   if (theLivestock)
      theLivestock->DailyUpdate();
   theCropRotation->DailyUpdate();
   theBuildings->DailyUpdate();
   theOperational_manager->GetStatus(); // NJH added this November 2006
   if((theTime.GetDay()==5) || (theTime.GetDay()==15) || (theTime.GetDay()==25))
   	cout << ".";

/*     filehandle = theMessage->GiveDebugHandle();
      *filehandle << endl ;
    theMessage->CloseDebugHandle();
  */
}

/****************************************************************************\
\****************************************************************************/
void eventControl::GiveIndicator(indicatorTypes indicatorType, int yearNumber)
{
   if (indicatorType==economicIndicator)
   {
      theOutput->AddIndicator(economicIndicator,"05.70 Ha premium","Dkr",theCropRotation->GetTotalPremium()); // BMP added �
      theOutput->AddIndicator(economicIndicator,"03.01 Value of arable land","Dkr",theCropRotation->GetValueofLand()); // BMP added �
   	theOutput->AddIndicator(economicIndicator,"06.10 Plant production misc. costs","Dkr",-theCropRotation->GetDiversePlant()); // BMP �
	   if (theLivestock)
      {
      	double animalUnits = theLivestock->GetDE();
         theOutput->AddStateIndicator(economicIndicator,"01.02 DE (old)","no",animalUnits);
         if (theCropRotation->GetTotalArea()>0)
	         theOutput->AddStateIndicator(economicIndicator,"01.03 DE/ha (old)","no/ha",animalUnits/theCropRotation->GetTotalArea());
	      theLivestock->GiveIndicator(indicatorType);
      }
		theEconomics = new economics;
		theEconomics->SetInterest(CapitalInterest);
	   theEconomics->GiveIndicator();
      delete theEconomics;
   }
   if (indicatorType==environmentalIndicator && theLivestock)
      theLivestock->FlushIndicator(indicatorType);
//   theOutput->AddStateIndicator(economicIndicator,"98.91 current year"," ",theTime.GetYear());
   theOutput->FlushIndicator(indicatorType,yearNumber);
}

/****************************************************************************\
\****************************************************************************/
void eventControl::Simulation(int runNumber, bsTime stopTime, int useGams, int NumberOfFixedYears, int GenerateFixedPlans,
                              char * inputDir, char * outputDir,
                              char * climateFilename, string economicIndicatorDate, string environmentalIndicatorDate)
{
   int yearNumber = 0;
   bsTime economicIndicatorTime, environmentalIndicatorTime, yearStopTime;

   economicIndicatorTime.SetTime(economicIndicatorDate);               				// default 1/9
   environmentalIndicatorTime.SetTime(environmentalIndicatorDate);      				// default 1/9
   if (economicIndicatorTime<=theTime)     economicIndicatorTime.AddOneYear();                  // added by JB
   if (environmentalIndicatorTime<=theTime) environmentalIndicatorTime.AddOneYear();

   cout << "Input directory    " << inputDir << endl;
   cout << "Output directory   " << outputDir << endl;
   cout << "Climate file       " << climateFilename << endl;

   if (chdir(outputDir)!=0)
#ifndef __ANSICPP__
      mkdir(outputDir);                                              				// make output directory if not present
#else
	#ifdef BUILDER
	  mkdir(outputDir);
	#else
	  cout << "Error - directory not found " << endl;
	  cout << "Press any key to exit " << endl;
	  char dum;
	  cin >> dum;
	  exit(99);
	#endif
#endif
   if (chdir(inputDir)!=0)
		theMessage->FatalError("Event:: input directory ",inputDir," not found!");
   Initialize(&stopTime,climateFilename,outputDir);

   if (!abort)
   {
      bool onceOnly=false;
      // Yearly outerloop begins
      while((theTime < stopTime)&& (!onceOnly))
      {
         cout << endl;
         cout <<  "Run "  << setw(2) << setfill('0') << runNumber
              <<  " Year "  << setw(2) << setfill('0') << yearNumber << endl;
         if (useGams && (yearNumber>=NumberOfFixedYears))
         {
/*            CalcLP(3);
			theManager->RunLP(3,outputDir);
			CalcLP(1);
			theManager->RunLP(1,outputDir);
			cout << endl << "Receiving production plans generated by LP-model" << endl;	// Initialize modules with results from LP model
			chdir("\\STRATEGI\\LP_MODEL");
			ReceivePlan("lp1",inputDir);
			RenameFiles(outputDir,inputDir,GenerateFixedPlans,yearNumber);
            chdir(inputDir);*/
         }
         else
         {
            // Initialize modules with results from fixed plan
            char fileExtension[4];
            fileExtension[0] = '\0';
            if (yearNumber<100)
               sprintf(fileExtension,"F%02d",yearNumber);
            else
               sprintf(fileExtension,"F%03d",yearNumber);
            cout << endl << "Receiving fixed production plans" << endl;
            ReceivePlan(fileExtension,inputDir);
         }

         cout << endl
              << "Simulating production ...................................."
              << endl << "Progress              ";

         // innerloop of production
         yearStopTime = theTime;
         yearStopTime.AddOneYear();
         while((theTime < yearStopTime)&& (!onceOnly))
         {
            onceOnly = false;    //enable if you want to pass through the daily loop only once per year
            // theProducts->SellPlantItems(); !!!???
            DailyUpdate();
            // if (!theLivestock) theProducts->SellPlantItems();  //sell plant products only if no animals present!! NJH April 2001
            theTime.AddOneDay();
            if (theTime==economicIndicatorTime)
            {
               theTechnics->YearlyCalc();
               GiveIndicator(economicIndicator,yearNumber);
               theTechnics->Reset();
               theLegislation->Update();                       // problems with date's???
            }
            if (theTime==environmentalIndicatorTime)
               GiveIndicator(environmentalIndicator,yearNumber);
         }

         // Reset before next year
         theProducts->resetProducts();
         theProducts->YearlyUpdate();
         yearNumber++;
         timeConsumption->outputtimeUse();
         economicIndicatorTime.AddOneYear();
         environmentalIndicatorTime.AddOneYear();
      }  // yearly outerloop ends
   }
/*
	EndSimulation();
   Above line creates memory problems sometimes - see soilProfile::GetMaximumDepth. Removed for now - BMP 11. November 2006 ??? !!!!
*/
}

/****************************************************************************\
\****************************************************************************/
void eventControl::EndSimulation(bool show)
{
   if (theLivestock)
      theLivestock->checkBalances(show);
   theBuildings->EndBudget(show);
   theCropRotation->EndBudget();
}



