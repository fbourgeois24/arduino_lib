/*
 * Programme pour recevoir les données du raspberry 
 * via le port série et activer des sorties
 * TOR (relais) ou analog (mosfet)
 * Ainsi que lire les entrées et les renvoyer au rapsberry
 * 
 * Au démarrage, le raspberry envoie la config des IO -> aucune IO préconfigurée par l'arduino
 * 
 * Messages échangés avec le raspberry
 * structure : 'type','addr','value'
 * Le type définit le type de message
 * - 0 -> config IO
 * - 1 -> digital
 * - 2 -> analog ou pwm
 * - 9 -> message d'erreur (seulement pour réponse). Message spécial, voir plus bas
 * Addr donne l'adresse de la pin
 * Value donne la valeur (dépend du type)
 * - Si type = 0 -> type de pin pour la config
 *    0 = input
 *    1 = output (actif haut)
 *    2 = input pullup
 *    3 = output (actif bas)
 *    5 = Dallas DS18#20
 * - Si type = 1 ou 2
 *    Si valeur >= 0 -> Valeur à appliquer sur la sortie
 *    Si valeur = -1 -> Lire l'entrée 
 *    
 * Message spécial erreur:
 * type = 9
 * addr : type du message reçu qui a généré l'erreur
 * value : code d'erreur
 * 
 * type | code erreur | message
 * -----|-------------|--------
 *   0  |     0       | Type d'IO inconnu
 * 
 * 
 */
 #include <OneWire.h>
 OneWire  ds(7);



// Variables
int type, value, addr; // Récupération des messages sur le port série



void setup() 
{
   // Démarrage du port série
  Serial.begin(115200);
}




void loop() 
{
  // Si il y a un message sur le port série, on le lit
  if (Serial.available() > 0)
  {
    type = Serial.parseInt();
    addr = Serial.parseInt();
    value = Serial.parseInt();
//    Serial.println(type);
//    Serial.println(addr);
//    Serial.println(value);
    
    // On "consomme" tout le reste du port série jusqu'au retour chariot
    while (Serial.read() != 13) {}

//    Serial.println(F("Fin de lecture du message"));

    // Si c'est une config de pin
    if (type == 0)
    {
      if (value <= 2)
      {
        // Si la valeur est entre 0 et 2 on peut la rentrer direct
        pinMode(addr, value);
        // On répond au message
        Serial.print(String(type) + "," + String(addr) + "," + String(value));
      }
      else if (value == 3)
      {
        // Si sortie actif bas, on force la sortie à 1
        pinMode(addr, 1);
        digitalWrite(addr, HIGH);
        // On répond au message
        Serial.print(String(type) + "," + String(addr) + "," + String(value));
      }
      else if (value == 5)
      {
        // Capteur Dallas DS18#20
        OneWire  ds(addr);
        Serial.print(String(type) + "," + String(addr) + "," + String(value));
      }      
    }
    // Si c'est un digital
    else if (type == 1)
    {
        // Ecriture Faux
        if (value == 0)
        {
          digitalWrite(addr, false);
          Serial.print(String(type) + "," + String(addr) + "," + String(value));
        }
        // Ecriture vrai
        else if (value == 1)
        {
          digitalWrite(addr, true);
          Serial.print(String(type) + "," + String(addr) + "," + String(value));
        }
        // Lecture
        else if (value == -1)
        {
          Serial.print(String(type) + "," + String(addr) + "," + String(digitalRead(addr)));
        }   
    }
    // Si c'est un analogique
    else if (type == 2)
    {
      // Ecriture 
      if (value >= 0)
      {
        // Max
        if (value > 255)
        {
          value = 255;
        }

        analogWrite(addr, value);
        Serial.print(String(type) + "," + String(addr) + "," + String(value));
      }
      // Lecture
      else if (value == -1)
      {
        Serial.print(String(type) + "," + String(addr) + "," + String(analogRead(addr)));
      }
    }
    // Si c'est une sonde Dallas DS18#20
    else if (type == 5)
    {
      if (value = -1)
      {
        // Lecture de la sonde
        byte i;
        byte present = 0;
        byte type_s;
        byte data[12];
        byte d_addr[8];
        float celsius;

        ds.search(d_addr);
        
//        // Recherche des différents chip
//        if ( !ds.search(d_addr)) 
//        {
//          ds.reset_search();
//          delay(250);
//          return;
//        }
        
        if (OneWire::crc8(d_addr, 7) != d_addr[7]) 
        {
            // message crc invalide
            Serial.println("9,5,0");
            return;
        }
      
       
        // Détection du type de chip en fonction du premier byte de l'adresse
        switch (d_addr[0]) 
        {
          case 0x10:
            //Serial.println("  Chip = DS18S20");  // or old DS1820
            type_s = 1;
            break;
          case 0x28:
            //Serial.println("  Chip = DS18B20");
            type_s = 0;
            break;
          case 0x22:
            //Serial.println("  Chip = DS1822");
            type_s = 0;
            break;
          default:
            //Serial.println("Device is not a DS18x20 family device.");
            return;
        } 
      
        ds.reset();
        ds.select(d_addr);
        ds.write(0x44, 1);        // start conversion, with parasite power on at the end
        
        delay(1000);     // maybe 750ms is enough, maybe not
        // we might do a ds.depower() here, but the reset will take care of it.
        
        present = ds.reset();
        ds.select(d_addr);    
        ds.write(0xBE);         // Read Scratchpad
      
        //Serial.print("  Data = ");
        //Serial.print(present, HEX);
        //Serial.print(" ");
        for ( i = 0; i < 9; i++) {           // we need 9 bytes
          data[i] = ds.read();
          //Serial.print(data[i], HEX);
          //Serial.print(" ");
        }
        //Serial.print(" CRC=");
        //Serial.print(OneWire::crc8(data, 8), HEX);
        //Serial.println();
      
        // Convert the data to actual temperature
        // because the result is a 16 bit signed integer, it should
        // be stored to an "int16_t" type, which is always 16 bits
        // even when compiled on a 32 bit processor.
        int16_t raw = (data[1] << 8) | data[0];
        if (type_s) {
          raw = raw << 3; // 9 bit resolution default
          if (data[7] == 0x10) {
            // "count remain" gives full 12 bit resolution
            raw = (raw & 0xFFF0) + 12 - data[6];
          }
        } else {
          byte cfg = (data[4] & 0x60);
          // at lower res, the low bits are undefined, so let's zero them
          if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
          else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
          else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
          //// default is 12 bit resolution, 750 ms conversion time
        }
        celsius = (float)raw / 16.0;
        Serial.print(String(type) + "," + String(addr) + "," + String(celsius));
      }
      
    }
  }
}
