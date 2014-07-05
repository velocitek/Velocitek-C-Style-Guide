// Copyright (c) 2014 Velocitek Inc. All rights reserved.
// Author: Alec Stewart
// Hierarchical State Machine Template by Ed Carryer, Stanford University

// See sm_normal_operation.h for an explanation of how this state machine is meant
// to be used.

#include "blackboxmain.h"

#include "flag_manager.h"
#include "no_signal.h"
#include "units.h"
#include "SMevents.h"
#include "SMnormaloperation.h"
#include "SMrace.h"
#include "SMtimer.h"

#define NO_SIGNAL_STATE 0
#define READY_STATE 1
#define RACE_STATE 2
#define REVIEW_STATE 3

#define NO_SIGNAL_TIMEOUT_MINUTES 20
#define NO_SIGNAL_TIMEOUT_SECONDS (NO_SIGNAL_TIMEOUT_MINUTES * 60.0)

static void DuringNoSignal(int8 event);
static void DuringReady(int8 event);
static void DuringRace(int8 event);
static void DuringReview(int8 event);

static int8 currentNormalOperationState = NO_SIGNAL_STATE;

// This function handles state transitions based on events
// and calls the current state's During function.
int8 RunNormalOperationSM(int8 currentEvent)
{
   int8 makeTransition = FALSE;
   int8 nextState = RACE_STATE;

   nextState = currentNormalOperationState;

   switch (currentNormalOperationState)
   {
      
    case NO_SIGNAL_STATE :
         DuringNoSignal(currentEvent);
         if (currentEvent != EV_NO_EVENT)
         {
            switch (currentEvent)
            {
                                    
               case EV_GPS_SOLUTION_FOUND :
                  nextState = READY_STATE;
                  makeTransition = TRUE;               
                  break;                                             
            }
         }
         break;
      
    case READY_STATE :
         DuringReady(currentEvent);
         if (currentEvent != EV_NO_EVENT)
         {
            switch (currentEvent)
            {
                                    
               case EV_GPS_SOLUTION_LOST :
                  nextState = NO_SIGNAL_STATE;
                  makeTransition = TRUE;               
                  break;
         
              case EV_BUTTON_PWR_PRESSED :
                  nextState = RACE_STATE;
                  makeTransition = TRUE;               
                  break;
                  
            }
         }
         break;
        
    case RACE_STATE :
         DuringRace(currentEvent);
         if (currentEvent != EV_NO_EVENT)
         {
            switch (currentEvent)
            {     
               case EV_BUTTON_PWR_PRESSED :
                  nextState = REVIEW_STATE;
                  makeTransition = TRUE;
                  break;
            }
         }
         break;
         
     
      case REVIEW_STATE :
         DuringReview(currentEvent);
         if (currentEvent != EV_NO_EVENT)
         {
            switch (currentEvent)
            {
               case EV_BUTTON_PWR_PRESSED :
               
                  if (QueryGpsSM() == GPS_AVAILABLE_STATE) nextState = READY_STATE;
                  else nextState = NO_SIGNAL_STATE;
                                    
                  makeTransition = TRUE;
                  break;
                  
            }
         }
         break;
   }
                                           
   if (makeTransition == TRUE)
   {
      switch (currentNormalOperationState)
      {
         case NO_SIGNAL_STATE :
            DuringNoSignal(EV_EXIT);
            break; 
  
     case READY_STATE :
            DuringReady(EV_EXIT);
            break; 
     
     case RACE_STATE :
            DuringRace(EV_EXIT);
            break;
         
         case REVIEW_STATE :
            DuringReview(EV_EXIT);
            break;           
      }

      currentNormalOperationState = nextState;

      switch (currentNormalOperationState)
      {
         
     case NO_SIGNAL_STATE :
            DuringNoSignal(EV_ENTRY);
            break; 
  
     case READY_STATE :
            DuringReady(EV_ENTRY);
            break; 
     
     case RACE_STATE :
            DuringRace(EV_ENTRY);
            break;
         
         case REVIEW_STATE :
            DuringReview(EV_ENTRY);
            break;

      }
   }
   return(currentEvent);
}

void StartNormalOperationSM(void)
{
   currentNormalOperationState = NO_SIGNAL_STATE;
         
   RunNormalOperationSM(EV_ENTRY);
}

int8 QueryNormalOperationSM(void)
{
   return currentNormalOperationState;
}

static void DuringNoSignal(int8 event)
{
   
  static int16 secondsWithoutSignal;
   
  if (event == EV_ENTRY)
  {
    secondsWithoutSignal = 0;
    SetSearchingIcons();
  }
  else if (event == EV_EXIT)
  {
    ClearSearchingIcons();
  }
  else
  {
    DisplaySearchingAnimation(event);      
    if(event == EV_BLINK) {
      secondsWithoutSignal++;
      if(secondsWithoutSignal == NO_SIGNAL_TIMEOUT_SECONDS) {
        GenerateSoftwareEvent(EV_NO_SIGNAL_TIMEOUT);
      }
    }
  }
   
} 
 
static void DuringReady(int8 event)
{
   
   if (event == EV_ENTRY)
   {
    DisplayMode(READY_MODE);
    DisplayZeroDistance();  
    DisplaySpeed(GetSpeed());
    DisplayZeroTime();
    
    
   }
   else if (event == EV_EXIT)
   {
      DisplayMode(CLEAR_MODE);
      DisplayBlankMainArea();
   }
   else
   {
   
    if (event == EV_FAST_BLINK)
    {
            
      DisplaySpeed(GetSpeed());
     
    }
       
   }
   
}
 
static void DuringRace(int8 event)
{
   if (event == EV_ENTRY)
   {
      
      ClearMaxSpeeds();
      ClearDistance();
      ResetAverageSpeed();
      ResetStartElapsedTime();            
      StartRaceSM();
     
   }
   else if (event == EV_EXIT)
   {
      // cleanup LCD
      stopTimer();
      DisplayBlankMainArea();
      RunRaceSM(event);
   }
   else
   {
       
    RunRaceSM(event);
      
   }
}

static void DuringReview(int8 event)
{
   
   #define BLINKS_PER_MAX_AVG_TOGGLE 2
   
   static float averageSpeed;
   static float maxSpeed;
   
   static int8 blinkCount = 0;
   static int1 showAverageNotMax;
      
   if (event == EV_ENTRY) {
      //Always start off by recalling the average speed.
      showAverageNotMax = TRUE;
      
      averageSpeed = GetAverageSpeed();
      maxSpeed = GetMaxSpeed();
        
      DisplayMode(RESULTS_MODE);      
      DisplayDistance (GetDistance());
      DisplayAvgSpeed(averageSpeed);      
      DisplayTimerElapsedTime();      
   }
   else if (event == EV_EXIT)
   {         
      DisplayMode(CLEAR_MODE);
      DisplayBlankMainArea();
   }
   else
   {
    
    if(event == EV_BLINK)
    {
      blinkCount++;
      if(blinkCount == BLINKS_PER_MAX_AVG_TOGGLE)
      {      
        showAverageNotMax = !showAverageNotMax;
        blinkCount = 0;
        
        if(showAverageNotMax)
        {
          DisplayAvgSpeed(averageSpeed);
        }
        else
        {
          DisplayMaxSpeed(maxSpeed);
        }      
      }
                      
    }
   }
   
}

