/*
Extracted from:
https://www.circuitbasics.com/how-to-set-up-the-dht11-humidity-sensor-on-the-raspberry-pi/

Heat Index calculation:
https://www.tiempo.com/ram/289402/la-sensacion-termica-y-sus-calculos-por-la-aemet/

V1.1: Nos aseguramos que las 
*/
#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <iostream>
#include <fstream>
#include <ctime>
#include <string>
#include <cmath>

#define MAXTIMINGS         85
#define DHTPIN              7
#define BAD_MEASUREMENT     -300

#define VERSION                1.1

#ifdef DEBUG
 #define UPDATE_TIME         5
#else
 #define UPDATE_TIME         30
#endif



int dht11_dat[5] = {0, 0, 0, 0, 0};

void read_dht11_dat()
{
    uint8_t laststate = HIGH;
    uint8_t counter = 0;
    uint8_t j = 0, i;
    float f;

    dht11_dat[0] = dht11_dat[1] = dht11_dat[2] = dht11_dat[3] = dht11_dat[4] = 0;

    pinMode(DHTPIN, OUTPUT);
    digitalWrite(DHTPIN, LOW);
    delay(18);
    digitalWrite(DHTPIN, HIGH);
    delayMicroseconds(40);
    pinMode(DHTPIN, INPUT);

    for (i = 0; i < MAXTIMINGS; i++)
    {
        counter = 0;
        while (digitalRead(DHTPIN) == laststate)
        {
            counter++;
            delayMicroseconds(1);
            if (counter == 255)
            {
                break;
            }
        }
        laststate = digitalRead(DHTPIN);

        if (counter == 255)
            break;

        if ((i >= 4) && (i % 2 == 0))
        {
            dht11_dat[j / 8] <<= 1;
            if (counter > 16)
                dht11_dat[j / 8] |= 1;
            j++;
        }
    }

    if ((j >= 40) && (dht11_dat[4] == ((dht11_dat[0] + dht11_dat[1] + dht11_dat[2] + dht11_dat[3]) & 0xFF)))
    {
        f = dht11_dat[2] * 9. / 5. + 32;
        #ifdef DEBUG
        printf("Humidity = %d.%d %% Temperature = %d.%d C (%.1f F)\n", dht11_dat[0], dht11_dat[1], dht11_dat[2], dht11_dat[3], f);
        #endif
    }
    else
    {    
        dht11_dat[0] = dht11_dat[1] = dht11_dat[2] = dht11_dat[3] = dht11_dat[4] = BAD_MEASUREMENT;
        #ifdef DEBUG
        printf("Data not good, skip\n");
        #endif
    }
}

/*!
 *  @brief  Converts Celcius to Fahrenheit
 *  @param  c
 *					value in Celcius
 *	@return float value in Fahrenheit
 */
float convertCtoF(float c) { return c * 1.8 + 32; }

/*!
 *  @brief  Converts Fahrenheit to Celcius
 *  @param  f
 *					value in Fahrenheit
 *	@return float value in Celcius
 */
float convertFtoC(float f) { return (f - 32) * 0.55555; }

/*!
 *  @brief  Compute Heat Index
 *  				Using both Rothfusz and Steadman's equations
 *					(http://www.wpc.ncep.noaa.gov/html/heatindex_equation.shtml)
 *  @param  temperature
 *          temperature in selected scale
 *  @param  percentHumidity
 *          humidity in percent
 *  @param  isFahrenheit
 * 					true if fahrenheit, false if celcius
 *	@return float heat index
 */
float computeHeatIndex(float temperature, float percentHumidity, bool isFahrenheit)
{
  float hi;
  if (!isFahrenheit)
    temperature = convertCtoF(temperature);

  hi = 0.5 * (temperature + 61.0 + ((temperature - 68.0) * 1.2) + (percentHumidity * 0.094));

  if (hi > 79)
  {
    hi = -42.379 + 2.04901523 * temperature + 10.14333127 * percentHumidity +
         -0.22475541 * temperature * percentHumidity +
         -0.00683783 * pow(temperature, 2) +
         -0.05481717 * pow(percentHumidity, 2) +
         0.00122874 * pow(temperature, 2) * percentHumidity +
         0.00085282 * temperature * pow(percentHumidity, 2) +
         -0.00000199 * pow(temperature, 2) * pow(percentHumidity, 2);

    if ((percentHumidity < 13) && (temperature >= 80.0) && (temperature <= 112.0))
      hi -= ((13.0 - percentHumidity) * 0.25) *
            sqrt((17.0 - abs(temperature - 95.0)) * 0.05882);

    else if ((percentHumidity > 85.0) && (temperature >= 80.0) && (temperature <= 87.0))
      hi += ((percentHumidity - 85.0) * 0.1) * ((87.0 - temperature) * 0.2);
  }

  return isFahrenheit ? hi : convertFtoC(hi);
}

std::string GetDate(std::tm* n) {
    char date[25];
    
    sprintf(date, "%02d-%02d-%02d", n->tm_year + 1900, n->tm_mon + 1, n->tm_mday);
    
    std::string str(date);
    return date;
}

std::string GetTime(std::tm* n) {
    char date[25];
    
    sprintf(date, "%02d:%02d:%02d", n->tm_hour, n->tm_min, n->tm_sec);
    
    std::string str(date);
    return date;
}

int main(void)
{
    float fResult;
    std::time_t tCurrentTime;
    std::tm* now; 
    float fTemperature;
    float fHumidity;
    float fHeatIndex = 0;
    float fHeatIndexAvg = 0;
    std::string sFileName;
    std::string sDate;
    std::string sTime;
    std::string sLine;
    char cCounter = 0;
    char cMeasCounter = 0;
    std::ofstream fsFile;
    char cValue[5];

    printf("Raspberry Pi wiringPi DHT11 Temperature test program. Version: %s\n", VERSION);

    if (wiringPiSetup() == -1)
    {
        printf("WiringPi library not found. Exiting program.");
        exit(1);
    }

    while (1)
    {    
        if ( cCounter == UPDATE_TIME ) {
            tCurrentTime = std::time(0); 
            now = std::localtime(&tCurrentTime);
            // sDate = std::to_string(now->tm_year + 1900) + "-" + std::to_string(now->tm_mon + 1) + "-" + std::to_string(now->tm_mday);
            sDate = GetDate(now);
            // sTime = std::to_string(now->tm_hour) + ":" + std::to_string(now->tm_min) + ":" + std::to_string(now->tm_sec);
            sTime = GetTime(now); 
            sFileName = "/home/pi/temperature_log_" + sDate + ".csv";
            #ifdef DEBUG
            std::cout << sFileName << "\n";
            std::cout << sTime << "\n";
            #endif
            fHeatIndexAvg = fHeatIndexAvg / cMeasCounter;
            fsFile.open(sFileName, std::ios_base::app);
            fsFile << sDate << "_" << sTime << ";" << fTemperature << ";" << fHumidity << ";" << fHeatIndex << ";" << fHeatIndexAvg << std::endl;
            fsFile.close();
            
            cCounter = 0;
            cMeasCounter = 0;
            fHeatIndexAvg = 0;
        }


        read_dht11_dat();
        if (dht11_dat[0] != BAD_MEASUREMENT ) {
            fTemperature = dht11_dat[2] + (dht11_dat[3] / 10.0) ;
            fHumidity = dht11_dat[0] + (dht11_dat[1] / 10.0) ;
            fHeatIndex = computeHeatIndex(fTemperature, fHumidity, false);
            fHeatIndexAvg += fHeatIndex; 
            cMeasCounter++;
            #ifdef DEBUG
            printf("Leído: %0.1f ºC, %0.1f%%, %0.1f ºC\n", fTemperature, fHumidity, fHeatIndex);
            #endif
        }
        
        cCounter++;
        delay(1000);
        
    }

    return (0);
}