#ifndef __EVENT_H
   #define __EVENT_H

#include <bstime.h>
#include <economic.h>
#include <technics.h>
#include <legislat.h>
#include <operations_manager_class.h>
#include <manure_manager.h>
#include <croprot.h>
#include <livestoc.h>
#include <products.h>
#include <output.h>
#include <theBuild.h>
#include <budget.h>


class eventControl: public base
{
   /* Attributes */
   private:
   	livestock * theLivestock;
      economics * theEconomics;
      cropRotation * theCropRotation;
//      manager * theManager;
      Operations_manager_class *theOperational_manager;
      double CapitalInterest;
      double MinFieldArea;
	   int ScaleBuildings;
      int AnimalModel;
		budget Nbudget;          
		bool abort;

      /* Actions */
      void InitVariables();
      void Initialize(bsTime * stopTime, char * climateFilename, char * outputPath);
      void ReceivePlan(char * fileExtension, char *inputDir);
      void DailyUpdate();
      void GiveIndicator(indicatorTypes indicatorType, int yearNumber);
      void RenameFile(string oldFileName,string newFileName, string extension, char * outputDir);
      void CopyFile(const char* inputFileName, const char* outputFileName,const char* inputDir,const char* outputDir);
      void RenameFiles(char * outputDir, char* inputDir, int GenerateFixedPlans, int yearNumber);
      void BasicLPParameters();
      void CalcLP(int periods);

   public:
      // Default Constructor
      eventControl();
      // Constructor with arguments
      eventControl(const char * aName, const int aIndex = -1, const base * aOwner = NULL);
      // Destructor
      ~eventControl();
      // Other functions
      void TestGams(bsTime stopTime, char * inputDir, char * outputDir, char * climateFilename);
      void Simulation(int runNumber, bsTime stopTime, int useGams, int NumberOfFixedYears, int GenerateFixedPlans,
                      char * inputDir, char * outputDir,
                      char * climateFilename, string economicIndicatorDate, string environmentalIndicatorDate);
      void EndSimulation(bool show = false);
};

#endif


