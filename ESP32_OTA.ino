/*
 * ============================================================
 *  ESP32 - Mise à jour OTA (Over-The-Air) via ArduinoOTA
 * ============================================================
 *  1er téléversement : par câble USB (obligatoire).
 *  Ensuite : Arduino IDE > Outils > Port > port réseau "esp32-xxx".
 *  Schéma de partition : doit contenir 2 zones OTA
 *  (ex. "Default 4MB with spiffs" ou "Minimal SPIFFS").
 * ============================================================
 */

#include <WiFi.h>        // Bibliothèque WiFi de l'ESP32 (connexion au routeur, IP, etc.)
#include <ESPmDNS.h>     // mDNS : permet de joindre la carte par un nom "esp32-xxx.local" au lieu de son IP
#include <WiFiUdp.h>     // Sockets UDP : ArduinoOTA s'en sert pour recevoir l'invitation de l'IDE
#include <ArduinoOTA.h>  // Bibliothèque qui gère la réception et l'écriture du nouveau firmware

// ------------------ PARAMÈTRES À ADAPTER --------------------
// Les valeurs réelles sont dans l'onglet "arduino_secrets.h"
#include "arduino_secrets.h"  // Fichier séparé contenant les identifiants (à ne pas partager)

const char WIFI_SSID[]     = SECRET_WIFI_SSID;      // Nom du réseau WiFi, repris depuis arduino_secrets.h
const char WIFI_PASSWORD[] = SECRET_WIFI_PASSWORD;  // Mot de passe du réseau WiFi
const char OTA_HOSTNAME[]  = SECRET_OTA_HOSTNAME;   // Nom de la carte sur le réseau (affiché dans le menu Port de l'IDE)
const char OTA_PASSWORD[]  = SECRET_OTA_PASSWORD;   // Mot de passe demandé par l'IDE avant chaque mise à jour OTA
// ------------------------------------------------------------

// ---------- Connexion WiFi (mode station) -------------------
void connexionWiFi() {                    // Fonction qui connecte l'ESP32 au routeur
  WiFi.mode(WIFI_STA);                    // Mode "station" : l'ESP32 se comporte comme un client du routeur
  WiFi.setHostname(OTA_HOSTNAME);         // Nom de la carte vu par le routeur (liste des appareils connectés)
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);   // Lance la tentative de connexion (non bloquante)

  Serial.print("Connexion WiFi");         // Message de début sur le moniteur série
  unsigned long debut = millis();         // Mémorise l'instant de départ (en ms) pour gérer un délai maximal
  while (WiFi.status() != WL_CONNECTED) { // Boucle tant que la connexion n'est pas établie
    delay(500);                           // Attend 0,5 s entre deux vérifications
    Serial.print(".");                    // Affiche un point pour montrer que l'attente progresse
    if (millis() - debut > 20000) {       // Si plus de 20 s se sont écoulées sans connexion...
      Serial.println("\nEchec WiFi, redemarrage..."); // ...on le signale...
      ESP.restart();                      // ...et on redémarre la carte pour réessayer proprement
    }                                     // Fin du test de délai
  }                                       // Fin de la boucle d'attente : on est connecté
  Serial.print("\nConnecte ! IP : ");     // Annonce la réussite
  Serial.println(WiFi.localIP());         // Affiche l'adresse IP attribuée par le routeur
}                                         // Fin de connexionWiFi()

// ---------- Configuration OTA -------------------------------
void setupOTA() {                         // Fonction qui prépare le service de mise à jour sans fil
  ArduinoOTA.setHostname(OTA_HOSTNAME);   // Nom annoncé en mDNS, qui apparaît comme port réseau dans l'IDE
  ArduinoOTA.setPassword(OTA_PASSWORD);   // Exige ce mot de passe : empêche quelqu'un d'autre de flasher la carte
  ArduinoOTA.setPort(3232);               // Port réseau d'écoute (3232 = valeur par défaut sur ESP32)

  // Début de la mise à jour
  ArduinoOTA.onStart([]() {               // Fonction (lambda) appelée au moment où une mise à jour commence
    String type = (ArduinoOTA.getCommand() == U_FLASH) ? "programme" : "systeme de fichiers"; // U_FLASH = nouveau programme, sinon = image du système de fichiers
    // Si LittleFS/SPIFFS est mis à jour : LittleFS.end(); ici  // (il faut démonter le système de fichiers avant de l'écraser)
    Serial.println("OTA : debut mise a jour du " + type); // Indique ce qui est en train d'être mis à jour
  });                                     // Fin de la lambda onStart

  // Fin de la mise à jour
  ArduinoOTA.onEnd([]() {                 // Lambda appelée quand la réception est terminée avec succès
    Serial.println("\nOTA : termine, redemarrage"); // La carte va redémarrer automatiquement sur le nouveau firmware
  });                                     // Fin de la lambda onEnd

  // Progression (en %)
  ArduinoOTA.onProgress([](unsigned int progression, unsigned int total) { // Lambda appelée à chaque bloc reçu (octets reçus / taille totale)
    Serial.printf("OTA : %u%%\r", (progression * 100) / total); // Affiche le pourcentage ; "\r" réécrit sur la même ligne
  });                                     // Fin de la lambda onProgress

  // Gestion des erreurs
  ArduinoOTA.onError([](ota_error_t erreur) {       // Lambda appelée si la mise à jour échoue, avec le code d'erreur
    Serial.printf("OTA erreur [%u] : ", erreur);     // Affiche le numéro de l'erreur
    if      (erreur == OTA_AUTH_ERROR)    Serial.println("Mot de passe refuse");    // Mauvais mot de passe OTA saisi dans l'IDE
    else if (erreur == OTA_BEGIN_ERROR)   Serial.println("Echec au demarrage");     // Pas assez de place / mauvais schéma de partition
    else if (erreur == OTA_CONNECT_ERROR) Serial.println("Echec de connexion");     // Impossible d'ouvrir la liaison avec le PC
    else if (erreur == OTA_RECEIVE_ERROR) Serial.println("Echec de reception");     // Transfert interrompu (WiFi instable, loop bloquée...)
    else if (erreur == OTA_END_ERROR)     Serial.println("Echec de finalisation");  // Firmware reçu mais invalide / vérification ratée
  });                                     // Fin de la lambda onError

  ArduinoOTA.begin();                     // Démarre réellement le service OTA (et l'annonce mDNS)
  Serial.println("OTA pret.");            // Confirme que la carte attend désormais les mises à jour
}                                         // Fin de setupOTA()

// ---------- Programme principal -----------------------------
void setup() {                            // Exécuté une seule fois au démarrage de la carte
  Serial.begin(115200);                   // Ouvre le port série à 115200 bauds pour les messages de debug
  connexionWiFi();                        // Se connecte au WiFi (indispensable avant l'OTA)
  setupOTA();                             // Configure et lance le service OTA
  // ... votre initialisation (capteurs, moteurs, etc.)
}                                         // Fin de setup()

void loop() {                             // Exécuté en boucle infinie après setup()
  ArduinoOTA.handle();                    // OBLIGATOIRE : vérifie si l'IDE demande une mise à jour et la traite
  // ... votre code, SANS delay() long (utiliser millis())  
  // un delay() long empêcherait handle() d'être appelé assez souvent 
  // Fin de loop() : on recommence au début
}                                         
