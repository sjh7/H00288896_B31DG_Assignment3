
//H00288896
//Samuel Hardman

//This program carries out the 10 tasks outlined in the assignmnet breif, 
//the tasks use FreeRTOS to carry out the tasks at specified rates, 
//with the notable meausrements being submitted to the serial port every 5 seconds in csv format,
//as required a queue is used to link requirement 5 & 7, and a global struct with semaphore protection, 
//is used to store the data to be printed in requirement 10

//Defines all the constant parameters of the program
#define Wdg 21      //define pin for watchdog (pin 21)
#define Sw1 19      //define pin for the push button input (pin 19)
#define Sw2 16      //define pin for the 2nd push button input (pin 12)
#define SqWv 13     //define pin for square wave input (pin 17)
#define An1 15      //define pin for the anolog input (pin 15)
#define ErrLED 23   //define pin for the error LED (pin 23)
#define TestLED 22  //define pin for the TestLED to demonstrate the timing of task 4(pin 22)
static const uint8_t AveQueueLength = 1;  //defines the length of the queue 
static QueueHandle_t AveQueue;            //defines the name/handle of the queue
static SemaphoreHandle_t ToPrintSecurity; //defines the name/handle of the Semaphore

//Sets the global variables 
//int Sw1State;     //defines the global variable that will store the state of the push button input
//int Freq;         //defines the global variable that will store the frequency of the square wave input
//int An1Data[4];   //defines the global array that will store the values of the anolog input
//int An1Ave;       //defines the global variable that will store the filtered anolog value of the analog input
//int error_code;   //defines the global variable that will store the state of the error_code
//int time1 = 0;
//int time2 = 0;

struct Struct_data {  //sets up the struct that will store the data to be printed
  int Sw1StateTP;
  int FreqTP;
  int An1AveTP;
};

Struct_data ToPrint;  //creates the struct that will store the data to be printed


//Task to create the watchdog pulse (requirement 1)
void TaskWatchdog(void * Parameters){  
  for (;;){
    digitalWrite(Wdg, HIGH);    //Starts pulse on SigB
    delayMicroseconds(50);      //Delay for width of the Pulse
    digitalWrite(Wdg, LOW);     //End Pulse on SigB

    vTaskDelay(17.54/portTICK_PERIOD_MS); //task delayed for 17.54ms
    
  }
}


//Task to read the state of the push button input (requirement 2)
void TaskSw1Check(void * parameters){
  int Sw1State;   //defines the variable that will store the state of the push button input
  for(;;){
    Sw1State = digitalRead(Sw1);    //read the value of digital input Sw1 and store it in the variable Sw1State

    xSemaphoreTake(ToPrintSecurity,10); //obtain the semaphore before accessing shared data struct
    ToPrint.Sw1StateTP = Sw1State;    //edit switch state value in the shared data struct
    xSemaphoreGive(ToPrintSecurity);  //return the semaphore after accessing shared data struct
 
    vTaskDelay(200/portTICK_PERIOD_MS);   //task delayed by 200ms
  }
}

//Task to read the frequency of the square wave input (requirement 3)
void TaskFreqCheck(void * parameters){
  int TimeHigh;   //defines the variable that will store the high time of the square wave input
  int Freq;       //defines the variable that will store the frequency of the square wave input
  for(;;){
    TimeHigh = pulseIn(SqWv, HIGH);   //measure time high of input and store in TimeHigh
    Freq = 1000000/(TimeHigh*2);      //calulate frequency and store in Freq

    xSemaphoreTake(ToPrintSecurity,10);   //obtain the semaphore before accessing shared data struct
    ToPrint.FreqTP = Freq;                //edit frequency value in the shared data struct
    xSemaphoreGive(ToPrintSecurity);  //return the semaphore after accessing shared data struct


    vTaskDelay(1000/portTICK_PERIOD_MS);    //task delayed by 1s
  }
}

//Task to read the value of the analog input  (requirement 4 & 5)
void TaskAn1Read(void * parameters){
  int sum = 0;      //defines the  variable that will store the sum of the past 4 values of the analog input
  int An1Current = 0;   //defines the  variable that will store the current value of the analog input
  int An1Ave;   //defines the variable that will store the filtered anolog value of the analog input
  int An1Data[4];   //defines the global array that will store the values of the anolog input

  for(;;){
    //digitalWrite(TestLED, HIGH);
    sum = 0;      //defines the  variable that will store the sum of the past 4 values of the analog input
    An1Current = analogRead(An1);   //read the value of the analog input and store in the variable An1Current
    //(TestLED, LOW);
    
  


    //move all values of the array An1Data up 1 place
    for(int i = 3; i > 0; i--){     
      An1Data[i] = An1Data[i-1];
    }

    An1Data[0] = An1Current;    //store the current analog reading in the first place of the array An1Data

    //sum all of the values in the An1Data array 
    for(int x = 0; x <4; x++){
      sum = sum + An1Data[x];
    }

    An1Ave = sum/4;     //calculate the filtered analog value and store it in the variable An1Ave
    xQueueOverwrite(AveQueue, &An1Ave);     //update value in queue

    xSemaphoreTake(ToPrintSecurity,10);   //obtain the semaphore before accessing shared data struct
    ToPrint.An1AveTP = An1Ave;           //edit average filtered value in the shared data struct
    xSemaphoreGive(ToPrintSecurity);    //return the semaphore after accessing shared data struct


    vTaskDelay(42/portTICK_PERIOD_MS);    //task delayed by 42ms

  }
}

//Task to carry out the asmTask 1000 times (requirement 6)
void TaskASM(void * parameters){
  for(;;){

    for (int y = 0; y < 1000; y++){   //for a 1000 iterations do below
      __asm__ __volatile__ ("nop");   //carry out asm instruction
    }
    vTaskDelay(100/portTICK_PERIOD_MS);   //task delayed by 100ms
  }
}

//Task to carry out the defined error check and set the LED appropriatley (requirement 7 & 8)
void TaskErrCheck(void * parameters){
  int An1AveC;
  int error_code;
 for(;;){
    xQueueReceive(AveQueue, &An1AveC, 43);    //read average filtered value from queue
    if(An1AveC > 2030){    //if the filtered analog value is greater than 2030
      error_code = 1;     //set the error code variable to a value of 1
      digitalWrite(ErrLED, HIGH);   //set the error_code LED on
    }
    else{
      error_code = 0;     //set the error code variable to a value of 0
      digitalWrite(ErrLED, LOW);    //set the error_code LED off
    }
    vTaskDelay(330/portTICK_PERIOD_MS);   //task delayed by 330ms
 }
}

//Task to print the state of the push button, the frequency of the 
//square wave and the average value of the anolog input in csv format when Sw2 is pressed (requirement 9 & 10)
void TaskCSVPrint(void * parameters){
  int Sw2State;  
  for(;;){
    Sw2State = digitalRead(Sw2);  //read state of Sw2 and store in Sw2State
    if (Sw2State == HIGH){    //if Sw2 pressed carry out below
        
      xSemaphoreTake(ToPrintSecurity, 10);    //obtain the semaphore before accessing shared data struct

      Serial.print(ToPrint.Sw1StateTP);   //print the state of the push button to the serial line
      Serial.print(",");       //print a comma to the serial line
      Serial.print(ToPrint.FreqTP);       //print the frequency of the square wave input to the serial line
      Serial.print(",");       //print a comma to the serial line
      Serial.println(ToPrint.An1AveTP);   //print the filtered analog value of the analog input to the serial line

      xSemaphoreGive(ToPrintSecurity);    //return the semaphore after accessing shared data struct
    }
    vTaskDelay(5000/portTICK_PERIOD_MS);    //task delayed by 330ms
  }  
}



//Setup for all Pins used in the program & carry out setup of ticker and serial comms
void setup() {
  pinMode(Wdg, OUTPUT);   //Sets the pin of Wdg as an output
  pinMode(ErrLED, OUTPUT);    //Sets the pin of ErrLED as an output
  pinMode(Sw1, INPUT);    //Sets the pin of Sw1 as an output
  pinMode(Sw2, INPUT);    //Sets the pin of Sw1 as an output
  pinMode(SqWv, INPUT);   //Sets the pin of SqWv as an output
  pinMode(An1, INPUT);    //Sets the pin of An1 as an output
  pinMode(TestLED, OUTPUT);   //Sets the pin of TestLED as an output
  digitalWrite(Sw1, HIGH);    //Activates the pullup resistor of the input pin of Sw1
  digitalWrite(Sw2, HIGH);    //Activates the pullup resistor of the input pin of Sw1

  Serial.begin(9600);   // initialize Serial communication
  while (!Serial);    // wait for the serial port to open

  ToPrintSecurity = xSemaphoreCreateBinary();   //create binary semaphore called ToPrintSecurity
  xSemaphoreGive(ToPrintSecurity);    //give the semaphore initial value

  AveQueue = xQueueCreate(AveQueueLength, sizeof(int));   //create queue  called AveQueue

//Create watchdog task
  xTaskCreate(
    TaskWatchdog,  //Task routine name
    "Watchdog",   // debug name
    512,  //stack size (optimized using uxTaskGetStackHighWaterMark)
    NULL, 
    3,  //Priority
    NULL );

//Create Sw1 check task
  xTaskCreate(
    TaskSw1Check,
    "Sw1Check",   
    512,  
    NULL,
    2,  
    NULL );

//Create frequency check task
  xTaskCreate(
    TaskFreqCheck,
    "FreqCheck",   
    1000,  
    NULL,
    2,  
    NULL );
  
  //Create anoloug read task
  xTaskCreate(
    TaskAn1Read,
    "An1Read",   
    900,  
    NULL,
    2,  
    NULL );

//Create ASM task
  xTaskCreate(
    TaskASM,
    "ASM",   
    512,  
    NULL,
    1,  
    NULL );

//Create Error check task
  xTaskCreate(
    TaskErrCheck
    ,  "ErrCheck"   
    ,  720  
    ,  NULL
    ,  1  
    ,  NULL );

//Create CSV print task
  xTaskCreate(
    TaskCSVPrint
    ,  "CSVPrint"   
    ,  720  
    ,  NULL
    ,  2  
    ,  NULL );
  
}


//main (empty as all task calling occurs in FreeRTOS)
void loop() {
}
