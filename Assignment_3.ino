
//H00288896
//Samuel Hardman

//This program carries out the 9 tasks outlined in the assignmnet breif, 
//the tasks run in a cyclic executive to carry out the tasks at specified rates, 
//with the notable meausrements being submitted to the serial port every 5 seconds in csv format

//Defines all the constant parameters of the program
#define Wdg 21      //define pin for watchdog (pin 21)
#define Sw1 19      //define pin for the push button input (pin 19)
#define Sw2 12      //define pin for the 2nd push button input (pin 12)
#define SqWv 13     //define pin for square wave input (pin 17)
#define An1 15      //define pin for the anolog input (pin 15)
#define ErrLED 23   //define pin for the error LED (pin 23)
#define TestLED 22  //define pin for the TestLED to demonstrate the timing of task 4(pin 22)
static const uint8_t AveQueueLength = 2;
static QueueHandle_t AveQueue;

//Sets the global variables 
int Sw1State;     //defines the global variable that will store the state of the push button input
int Sw2State;
int Freq;         //defines the global variable that will store the frequency of the square wave input
int An1Data[4];   //defines the global array that will store the values of the anolog input
//int An1Ave;       //defines the global variable that will store the filtered anolog value of the analog input
int error_code;   //defines the global variable that will store the state of the error_code


//Subroutine to create the watchdog pulse (task 1)
void TaskWatchdog(void * Parameters){  
  for (;;){
    digitalWrite(Wdg, HIGH);    //Starts pulse on SigB
    delayMicroseconds(50);      //Delay for width of the Pulse
    digitalWrite(Wdg, LOW);     //End Pulse on SigB
    vTaskDelay(17.54/portTICK_PERIOD_MS); // wait for one second
  }
}


//Subroutine to read the state of the push button input (task 2)
void TaskSw1Check(void * parameters){
  for(;;){
    Sw1State = digitalRead(Sw1);    //read the value of digital input Sw1 and store it in the variable Sw1State
    vTaskDelay(200/portTICK_PERIOD_MS);
  }
}

//Subroutine to read the frequency of the square wave input (task 3)
void TaskFreqCheck(void * parameters){
  int TimeHigh;
  for(;;){
    TimeHigh = pulseIn(SqWv, HIGH);
    Freq = 1000000/(TimeHigh*2);
    vTaskDelay(1000/portTICK_PERIOD_MS);
  }
}

//Subroutine to read the value of the analog input  (task 4 & 5)
void TaskAn1Read(void * parameters){
  int sum = 0;      //defines the  variable that will store the sum of the past 4 values of the analog input
  int An1Current = 0;   //defines the  variable that will store the current value of the analog input
  for(;;){
    digitalWrite(TestLED, HIGH);
    sum = 0;      //defines the  variable that will store the sum of the past 4 values of the analog input
    An1Current = analogRead(An1);   //read the value of the analog input and store in the variable An1Current
    digitalWrite(TestLED, LOW);
    
  


    //move all values of the array An1Data up 1 place
    for(int i = 3; i > 0; i--){     
      An1Data[i] = An1Data[i-1];
    }

    An1Data[0] = An1Current;    //store the current analog reading in the first place of the array An1Data

    //sum all of the values in the An1Data array 
    for(int x = 0; x <4; x++){
      sum = sum + An1Data[x];
    }

    An1Ave = sum/4; //calculate the filtered analog value and store it in the variable An1Ave

    xQueueOverwrite(AveQueue, (void *)&An1Ave, 5);
    
    vTaskDelay(42/portTICK_PERIOD_MS);
  }
}

//Subroutine to carry out the asmTask 1000 times (task 6)
void TaskASM(void * parameters){
  for(;;){
    for (int y = 0; y < 1000; y++){   //for a 1000 iterations do below
      __asm__ __volatile__ ("nop");   //carry out asm instruction
    }
    vTaskDelay(100/portTICK_PERIOD_MS);
  }
}

//Subroutine to carry out the defined error check and set the LED appropriatley (task 7 & 8)
void TaskErrCheck(void * parameters){
 for(;;){
    if(An1Ave > 2030){    //if the filtered analog value is greater than 2030
      error_code = 1;     //set the error code variable to a value of 1
      digitalWrite(ErrLED, HIGH);   //set the error_code LED on
    }
    else{
      error_code = 0;     //set the error code variable to a value of 0
      digitalWrite(ErrLED, LOW);    //set the error_code LED off
    }
    vTaskDelay(330/portTICK_PERIOD_MS);
 }
}

//Subroutine to print the state of the push button, the frequency of the 
//square wave and the average value of the anolog input in csv format (task 9)
void TaskCSVPrint(void * parameters){
  for(;;){
    Sw2State = digitalRead(Sw2);  
    if (Sw2State == HIGH){
      Serial.print(Sw1State);   //print the state of the push button to the serial line
      Serial.print(", ");       //print a comma to the serial line
      Serial.print(Freq);       //print the frequency of the square wave input to the serial line
      Serial.print(", ");       //print a comma to the serial line
      Serial.println(An1Ave);   //print the filtered analog value of the analog input to the serial line
      vTaskDelay(5000/portTICK_PERIOD_MS);
    }
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

  AveQueue = xQueueCreate(AveQueueLength, sizeof(int)); 

  xTaskCreate(
    TaskWatchdog,
    "Watchdog",   // A name just for humans
    1023,  // This stack size can be checked & adjusted by reading the Stack Highwater
    NULL,
    3,  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    NULL );

  xTaskCreate(
    TaskSw1Check
    ,  "Sw1Check"   // A name just for humans
    ,  1023  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  2  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL );

  xTaskCreate(
    TaskFreqCheck
    ,  "FreqCheck"   // A name just for humans
    ,  1023  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  2  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL );
  
  xTaskCreate(
    TaskAn1Read
    ,  "An1Read"   // A name just for humans
    ,  1023  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  2  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL );

  xTaskCreate(
    TaskASM
    ,  "ASM"   // A name just for humans
    ,  1023  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  1  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL );

  xTaskCreate(
    TaskErrCheck
    ,  "ErrCheck"   // A name just for humans
    ,  1023  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  1  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL );

  xTaskCreate(
    TaskCSVPrint
    ,  "CSVPrint"   // A name just for humans
    ,  1023  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  2  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL );
  
}


//main (empty as all task calling occurs in the cyclic executive schedule 'TickFlag')
void loop() {
}