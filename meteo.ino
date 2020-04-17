
    #include <SimpleDHT.h>
    #include <avr/power.h>
    #include <avr/sleep.h>
    #include <avr/power.h>
    #include <avr/wdt.h>
    #include <SoftwareSerial.h>

    SoftwareSerial softSerial(10, 11); // RX, TX

    String SSID = "Martel";  // SSID du réseau Wi-Fi
    String PASS = "00000000"; // Mot de passe Wi-Fi
    int pinDHT11 = 2;
    SimpleDHT11 dht11;
    volatile int f_wdt = 1;
    unsigned int tempo_desiree = 280; // mettre un multiple de 8, mais pas obligatoire
    unsigned int compteur_reveil = 280;


    void setup() {
      pinMode( 13, OUTPUT);
      softSerial.begin(115200); // baudrate par défaut de l'ESP8266
      delay(100);
      // on demande à utiliser un baudrate inférieur
      // (notre esp8266 s'est montrer plus stable ainsi)
     /*softSerial.println("AT+CIOBAUD=9600");
      delay(200);
      softSerial.begin(9600);*/

      //Optimisation de la consommation
      power_adc_disable(); // Convertisseur Analog / Digital pour les entrées analogiques
      power_spi_disable();
      power_twi_disable();
      // Si pas besoin de communiquer par l'USB
      power_usart0_disable();
      //Extinction des timers, attention timer0 utilisé par millis ou delay
      //power_timer0_disable();
      power_timer1_disable();
      power_timer2_disable();

      setup_watchdog(9);
    }

    // Watchdog Interrupt Service est exécité lors d'un timeout du WDT
    ISR(WDT_vect)
    {
      if (f_wdt == 0)
      {
        f_wdt = 1; // flag global }
      }
    }
    // paramètre : 0=16ms, 1=32ms, 2=64ms, 3=128ms, 4=250ms, 5=500ms, 6=1 sec,7=2 sec, 8=4 sec, 9=8 sec
    void setup_watchdog(int ii) {
        byte bb;
        int ww;
        if (ii > 9 ) ii = 9;
        bb = ii & 7;
        if (ii > 7) bb |= (1 << 5);
        bb |= (1 << WDCE);
        ww = bb;
        // Clear the reset flag
        MCUSR &= ~(1 << WDRF);
        // start timed sequence
        WDTCSR |= (1 << WDCE) | (1 << WDE);
        // set new watchdog timeout value
        WDTCSR = bb;
        WDTCSR |= _BV(WDIE);
    }


    void enterSleep(void) {
      set_sleep_mode(SLEEP_MODE_PWR_DOWN);
      sleep_enable();
      sleep_mode(); //Entre dans le mode veille choisi

      // Le programme va reprendre ici après le timeout du WDT

      sleep_disable(); // La 1ère chose à faire est de désactiver le mode veille
    }



    void loop() {
      if (f_wdt == 1) {
        compteur_reveil++;
        if ((compteur_reveil*8)>= tempo_desiree) {
          digitalWrite(13, HIGH);

          connectWifi();


          byte temperature = 0;
          byte humidity = 0;
          if (dht11.read(pinDHT11, &temperature, &humidity, NULL)) {
            return;
          }

          String temperature_string = String(temperature);
          sendValue("temperature", temperature_string);

          String humidity_string = String(humidity);
          sendValue("humidity", humidity_string);

          softSerial.println("AT+CWQAP");
          compteur_reveil = 0;
        }

        f_wdt = 0;
        enterSleep();
        digitalWrite(13, LOW);
          /*
        for (int i = 0;i<3;i++) {
           ///delay(8000);
        }*/
      }
    }

    void connectWifi()
    {
      delay(100);

      // on va se connecter à un réseau existant, donc on passe en mode station
      softSerial.println("AT+CWMODE=1");
      delay(1000);

      // on se connecte au réseau
      softSerial.println("AT+CWJAP=\""+SSID+"\",\""+PASS+"\"");
      delay(10000);
    }


    void sendValue(String metric, String value) {


      // mode "connexions multiples"
      softSerial.println("AT+CIPMUX=1");
      delay(2000);

      // on se connecte à notre serveur en TCP sur le port 80
      softSerial.println("AT+CIPSTART=4,\"TCP\",\"meteo.nathanaelmartel.net\",80");
      delay(500);

      String request = "/api/"+metric+"/"+value;

      // on donne la taille de la requête qu'on va envoyer, en ajoutant 2 car
      // println ajouter 2 caractères à la fin "\r" et "\n"
      //softSerial.println("AT+CIPSEND=4,"+String(request.length()+2));
      softSerial.println("AT+CIPSEND=4,"+String(83+request.length()));
      delay(500);

      // on envoie la requete
      //softSerial.println(request);
      softSerial.println("GET "+request+" HTTP/1.0");
      softSerial.println("Host: meteo.nathanaelmartel.net");
      softSerial.println("User-Agent: ESP8266-01,Arduino");
      softSerial.println("");
      delay(3000);

      // on ferme la connexion
      softSerial.println("AT+CIPCLOSE=4");
    }
   
   
