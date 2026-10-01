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

#include <WiFi.h>        // Gestion du WiFi de l'ESP32
#include <ESPmDNS.h>     // Nom réseau "esp32-xxx.local"
#include <WiFiUdp.h>     // Transport UDP utilisé par ArduinoOTA
#include <ArduinoOTA.h>  // Bibliothèque OTA

// ------------------ PARAMÈTRES À ADAPTER --------------------
// Les valeurs réelles sont dans l'onglet "arduino_secrets.h"
#include "arduino_secrets.h"

const char WIFI_SSID[]     = SECRET_WIFI_SSID;
const char WIFI_PASSWORD[] = SECRET_WIFI_PASSWORD;
const char OTA_HOSTNAME[]  = SECRET_OTA_HOSTNAME;
const char OTA_PASSWORD[]  = SECRET_OTA_PASSWORD;
// ------------------------------------------------------------

// ---------- Connexion WiFi (mode station) -------------------
void connexionWiFi() {
  WiFi.mode(WIFI_STA);                 // ESP32 = client du routeur
  WiFi.setHostname(OTA_HOSTNAME);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connexion WiFi");
  unsigned long debut = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis() - debut > 20000) {    // 20 s sans succès -> redémarrage
      Serial.println("\nEchec WiFi, redemarrage...");
      ESP.restart();
    }
  }
  Serial.print("\nConnecte ! IP : ");
  Serial.println(WiFi.localIP());
}

// ---------- Configuration OTA -------------------------------
void setupOTA() {
  ArduinoOTA.setHostname(OTA_HOSTNAME);  // nom mDNS
  ArduinoOTA.setPassword(OTA_PASSWORD);  // protège les mises à jour
  ArduinoOTA.setPort(3232);              // port par défaut ESP32

  // Début de la mise à jour
  ArduinoOTA.onStart([]() {
    String type = (ArduinoOTA.getCommand() == U_FLASH) ? "programme" : "systeme de fichiers";
    // Si LittleFS/SPIFFS est mis à jour : LittleFS.end(); ici
    Serial.println("OTA : debut mise a jour du " + type);
  });

  // Fin de la mise à jour
  ArduinoOTA.onEnd([]() {
    Serial.println("\nOTA : termine, redemarrage");
  });

  // Progression (en %)
  ArduinoOTA.onProgress([](unsigned int progression, unsigned int total) {
    Serial.printf("OTA : %u%%\r", (progression * 100) / total);
  });

  // Gestion des erreurs
  ArduinoOTA.onError([](ota_error_t erreur) {
    Serial.printf("OTA erreur [%u] : ", erreur);
    if      (erreur == OTA_AUTH_ERROR)    Serial.println("Mot de passe refuse");
    else if (erreur == OTA_BEGIN_ERROR)   Serial.println("Echec au demarrage");
    else if (erreur == OTA_CONNECT_ERROR) Serial.println("Echec de connexion");
    else if (erreur == OTA_RECEIVE_ERROR) Serial.println("Echec de reception");
    else if (erreur == OTA_END_ERROR)     Serial.println("Echec de finalisation");
  });

  ArduinoOTA.begin();                    // démarre le service OTA
  Serial.println("OTA pret.");
}

// ---------- Programme principal -----------------------------
void setup() {
  Serial.begin(115200);
  connexionWiFi();
  setupOTA();
  // ... votre initialisation (capteurs, moteurs, etc.)
}

void loop() {
  ArduinoOTA.handle();   // OBLIGATOIRE : écoute les demandes OTA
  // ... votre code, SANS delay() long (utiliser millis())
}
