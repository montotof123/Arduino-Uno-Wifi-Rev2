// ********************
// Alimentation V2
// Ajout du courant max
// totof 2019/08/16
// ********************

// Librairies
#include <Wire.h>
#include <LiquidCrystal.h>
#include <WiFiNINA.h>

#include "Adafruit_INA219.h"
#include "MCP42010.h"

#include "arduino_secrets.h"

// Liste des fonctions
constexpr auto CONTRASTE = 0;
constexpr auto LUMINOSITE = 1;
constexpr auto CONT_LUM_AUTO = 2;
constexpr auto SORTIE1 = 3;
constexpr auto SORTIE2 = 4;
constexpr auto VERROUILLAGE_SORTIES = 5;
constexpr auto ADRESSE_IP = 6;

// List des sorties
constexpr auto OFF = 0;
constexpr auto TENSION = 1;
constexpr auto COURANT = 2;
constexpr auto PUISSANCE = 3;
constexpr auto MAX_COURANT = 4;

// List des pins
constexpr auto RELAIS_SORTIE1 = 2;
constexpr auto RELAIS_SORTIE2 = 9;
constexpr auto TOUCHE_FONCTION = 16;
constexpr auto TOUCHE_MOINS = 15;
constexpr auto TOUCHE_PLUS = 14;

// Liste des potentiometres
constexpr auto POTENTIOMETRE_CONTRASTE = 1;
constexpr auto POTENTIOMETRE_LUMIERE = 2;

// ATTENTION-ATTENTION-ATTENTION-ATTENTION-ATTENTION-ATTENTION-ATTENTION
// Nombre de mesures pour la courbe web
// L'affichage de la courbe entraine une charge importante sur
// le programme, entrainant de fortes latences au niveau des touches et
// des informations sur l'afficheur mise à jour beaucoup moins souvent
// Plus il y a de points a afficher et plus cette latence est importante
// 800 points est déjà beaucoup avec une seule connexion d'un navigateur
// En cas de blocage, arrêter le(s) navigateur(s) connecté(s)
// ATTENTION-ATTENTION-ATTENTION-ATTENTION-ATTENTION-ATTENTION-ATTENTION
constexpr auto NB_MESURE = 800;

// Initialisation des peripheriques
LiquidCrystal lcd(3, 4, 5, 6, 7, 8);
Adafruit_INA219 out1 = Adafruit_INA219(0x44);
Adafruit_INA219 out2 = Adafruit_INA219(0x45);
MCP42010 pot = MCP42010(10, 13, 11);

// Fonction, initialise à SORTIE 1 pour l'affichage
uint8_t fonction = SORTIE1;

// Luminosite initialise au max
uint8_t valLumiere = 0;

// Contraste initialise au max
uint8_t valContraste = 0;

// Lumiere contraste auto
bool lumContrasteAuto = true;

// Sortie 1
uint8_t valSortie1 = OFF;

// Sortie2
uint8_t valSortie2 = OFF;

// Verrouillage sorties
bool verrouillageSorties = false;

// Memorise l'etat des touches pour detecte le changement d'etat
uint8_t valPrevTouchSwitchFnct = LOW;
uint8_t valPrevTouchSwitchFnctMoins = LOW;
uint8_t valPrevTouchSwitchFnctPlus = LOW;

// courant max
float max_courant1 = 0.0F;
float max_courant2 = 0.0F;

// Paramètres de connexion au WiFi
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;

// Serveur web
int status = WL_IDLE_STATUS;
WiFiServer server(80);
IPAddress ip;

// Tableau de mesure
float tabMesure1[NB_MESURE];

// *****************************************
//       ***** ***** ***** *   * *****
//       *     *       *   *   * *   *
//       ***** ****    *   *   * *****
//           * *       *   *   * *
//       ***** *****   *   ***** *
// *****************************************
void setup() {
	// Initialisation du moniteur
	Serial.begin(9600);
    while(!Serial){};
    Serial.println("Serial OK");
	
	// Initialisation du tableau de mesure
	for(int compteur = 0; compteur != NB_MESURE; compteur++) {
		tabMesure1[compteur] = 0.0F;
	}

	// Initialisation afficheur avec luminosite et contraste a fond
	MCP42010 pot = MCP42010(10, 13, 11);
	pot.setPot(POTENTIOMETRE_CONTRASTE, valContraste);
	pot.setPot(POTENTIOMETRE_LUMIERE, valLumiere);
	lcd.begin(16, 2);
	
	// Message d'initialisation
	lcd.setCursor(0, 0);
    lcd.print("  Alimentation  ");
	lcd.setCursor(0, 1);
	lcd.print("V2.00   Init... ");
	lcd.noCursor();
	Serial.println("Init afficheur");

	// Initialise les mesureurs
	Serial.println("Init mesureurs");
	out1.begin();
	out2.begin();

	// Initialise les pin
	pinMode(RELAIS_SORTIE1, OUTPUT);
	pinMode(RELAIS_SORTIE2, OUTPUT);
	pinMode(TOUCHE_FONCTION, INPUT);
	pinMode(TOUCHE_MOINS, INPUT);
	pinMode(TOUCHE_PLUS, INPUT);
	
	// Attente de connexion au WiFi
	Serial.println("Init serveur web");
	while (status != WL_CONNECTED) {
		status = WiFi.begin(ssid, pass);
		// attendre pour la connexion
		delay(5000);
	}
	server.begin();

	// Affiche l'adresse IP
	ip = WiFi.localIP();
	Serial.print("Addresse IP: ");
	Serial.println(ip);
	afficheAdresseIp();

	// Fin d'init
	Serial.println("Fin init");
	delay(4000);
}

// ****************************************
//         *     ***** ***** *****
//         *     *   * *   * *   *
//         *     *   * *   * *****
//         *     *   * *   * *
//         ***** ***** ***** *
// ****************************************

void loop() {
	// *******************
	// Lecture des touches
	// *******************

	// Action sur la touche fonction
	if (touchSwitchFnct()) {
		fonction = rotateFonction(fonction);
	}

	// Action sur la touche moins
	if (touchSwitchFnctMoins()) {
		switch (fonction) {
		case CONTRASTE:
			valContraste = downContraste();
			break;
		case LUMINOSITE:
			valLumiere = downLumiere();
			break;
		case CONT_LUM_AUTO:
			lumContrasteAuto = changeContrasteAuto();
			break;
		case SORTIE1:
			valSortie1 = rotateDownSortie1();
			break;
		case SORTIE2:
			valSortie2 = rotateDownSortie2();
			break;
		case VERROUILLAGE_SORTIES:
			verrouillageSorties = changeVerrouillageSorties();
			break;
		case ADRESSE_IP:
			break;
		}
	}

	// Action sur la touche plus
	if (touchSwitchFnctPlus()) {
		switch (fonction) {
		case CONTRASTE:
			valContraste = upContraste();
			break;
		case LUMINOSITE:
			valLumiere = upLumiere();
			break;
		case CONT_LUM_AUTO:
			lumContrasteAuto = changeContrasteAuto();
			break;
		case SORTIE1:
			valSortie1 = rotateUpSortie1();
			break;
		case SORTIE2:
			valSortie2 = rotateUpSortie2();
			break;
		case VERROUILLAGE_SORTIES:
			verrouillageSorties = changeVerrouillageSorties();
			break;
		case ADRESSE_IP:
			break;
		}
	}

	// ************************
	// Action sur les appareils
	// ************************
	// Luminosite contraste afficheur
	if (lumContrasteAuto) {
		// Calcul de la lumiere et du contraste si auto
		valContraste = analogRead(3) / 10;;
		valLumiere = valContraste;
		Serial.println("Auto");
	}
	pot.setPot(POTENTIOMETRE_CONTRASTE, valContraste);
	Serial.print("Contraste = ");
	Serial.println(valContraste);
	pot.setPot(POTENTIOMETRE_LUMIERE, valLumiere);
	Serial.print("Lumiere = ");
	Serial.println(valLumiere);

	// Mesureur
	float tension1 = out1.getBusVoltage_V();
	float courant1 = out1.getCurrent_mA();
	float puissance1 = out1.getPower_mW();
	float tension2 = out2.getBusVoltage_V();
	float courant2 = out2.getCurrent_mA();
	float puissance2 = out2.getPower_mW();
	// Evite l'affichage de courant très faible négatif
	if(courant1 < 0) {
		courant1 = 0;
	}
	if(courant2 < 0) {
		courant2 = 0;
	}
	// Courants max
	if(courant1 > max_courant1) {
		max_courant1 = courant1;
	}
	if(courant2 > max_courant2) {
		max_courant2 = courant2;
	}
	
	// Tableau des mesures, permet une avance des mesures de gauche à droite
	for(int compteur = NB_MESURE - 1; compteur != 0; compteur--) {
		tabMesure1[compteur] = tabMesure1[compteur - 1];
	}
	tabMesure1[0] = courant1;
	
	// Calcul du max
	float max1 = 30.0F;
	for(int compteur = 0; compteur != NB_MESURE - 1; compteur++) {
		if(tabMesure1[compteur] > max1) {
			max1 = tabMesure1[compteur];
		}
	}

	// Pas des lignes horizontales du graphique
	int pasLignesHorizontales = 250;
	if(max1 < 1000) {
		pasLignesHorizontales = 100;
	} 
	if(max1 < 500) {
		pasLignesHorizontales = 50;
	} 
	if(max1 < 100) {
		pasLignesHorizontales = 10;
	}
	
	
	// **********************
	// Gestion de l'affichage
	// **********************	
	// Mesure
	float mesure;
	
	// Gestion de la valeur à afficher
	switch (fonction) {
	case SORTIE1:
		switch (valSortie1) {
		case OFF: 
			sortie1Off();
			// Met le courant max a 0 si la sortie est desactivee
			max_courant1 = 0;
			// Delay pour les relais
			delay(10);
			break;
		case TENSION: 
			sortie1On();
			// Delay pour les relais
			delay(10);
			mesure = tension1;
			break;
		case COURANT: 
			sortie1On();
			// Delay pour les relais
			delay(10);
			mesure = courant1;
			break;
		case PUISSANCE:  
			sortie1On();
			// Delay pour les relais
			delay(10);
			mesure = puissance1;
			break;
		case MAX_COURANT:
			mesure = max_courant1;
			break;
		}
		break;
	case SORTIE2:
		switch (valSortie2) {
		case OFF:
			sortie2Off();
			// Met le courant max a 0 si la sortie est desactivee
			max_courant2 = 0;
			// Delay pour les relais
			delay(10);
			break;
		case TENSION:
			sortie2On();
			// Delay pour les relais
			delay(10);
			mesure = tension2;
			break;
		case COURANT:
			sortie2On();
			// Delay pour les relais
			delay(10);
			mesure = courant2;
			break;
		case PUISSANCE:
			sortie2On();
			// Delay pour les relais
			delay(10);
			mesure = puissance2;
			break;
		case MAX_COURANT:
			mesure = max_courant2;
			break;
		}
		break;
	}
	
	// Affichage
	switch (fonction) {
	case CONTRASTE:
		afficheContraste(valContraste);
		break;
	case LUMINOSITE:
		afficheLumiere(valLumiere);
		break;
	case CONT_LUM_AUTO:
		afficheContLumAuto(lumContrasteAuto);
		break;
	case SORTIE1: 
		afficheSortie(fonction, valSortie1, mesure);
		break;
	case SORTIE2: 
		afficheSortie(fonction, valSortie2, mesure);
		break;
	case VERROUILLAGE_SORTIES: 
		afficheVerrouillageSortie(verrouillageSorties);
		break;
	case ADRESSE_IP: 
		afficheAdresseIp();
		break;
	}
	
	// ***********
	// Serveur web
	// ***********
	
	// Ecoute des connexions
	WiFiClient client = server.available();

	if (client) {
		Serial.println("Nouveau client");
		// String de resultat du client
		String currentLine = "";   
		// Boucle tant que le client est connecté
		while (client.connected()) {  
			// Client actif
			if (client.available()) {   
				//Lecture de chaque octet venant du client
				char c = client.read();  
				// Un saut de ligne indique la fin du message
				if (c == '\n') {                    
					if (currentLine.length() == 0) {
						// HTTP headers demarre toujours avec un code réponse (ex: HTTP/1.1 200 OK)
						client.println("HTTP/1.1 200 OK");
						// et un content-type
						client.println("Content-type:text/html");
						// Il faut une ligne blanche entre l'entete et le corps pour que le HTTP soit valide
						client.println();

						// Contenu de la page
						client.println("<html>");
						client.println("	<head>");
						// Rafraichissement de la page toutes les 10s
						client.println("		<meta http-equiv=\"refresh\" content=\"10; url=/\" />");
						client.println("		<title>Alimentation</title>");
						// CSS
						client.println("		<style>");
						client.println("		  .txtrouge {color: red;}");
						client.println("		  .txtorange {color: orange;}");
						client.println("		  .txtbleu {color: blue;}");
						client.println("		  .txtvert {color: green;}");
						client.println("		</style>");
						client.println("	</head>");
						client.println("");
						client.println("	<body>");
						// Mesureur 1
						client.println("		<h1 class=\"txtrouge\">Mesureur 1</h1>");
						client.print  ("		<h2 class=\"txtorange\">&nbsp;&nbsp;&nbsp;Tension = ");
						client.print  (tension1);
						client.println("		V</h2>");
						client.print  ("		<h2 class=\"txtorange\">&nbsp;&nbsp;&nbsp;courant = ");
						client.print  (courant1);
						client.println("		mA</h2>");
						client.print  ("		<h2 class=\"txtorange\">&nbsp;&nbsp;&nbsp;puissance = ");
						client.print  (puissance1);
						client.println("		mW</h2>");
						client.print  ("		<h2 class=\"txtorange\">&nbsp;&nbsp;&nbsp;courant max = ");
						client.print  (max_courant1);
						client.println("		mA</h2>");
						client.println("		<h3>Historique du courant</h3>");
						client.print  ("		<canvas id=\"canvas1\" width=\""); client.print(NB_MESURE); client.println("\" height=\"300\"></canvas>");
						client.println("        <br>");
						client.println("        <br>");
						// Mesureur 2
						client.println("		<h1 class=\"txtbleu\">Mesureur 2</h1>");
						client.print  ("		<h2 class=\"txtvert\">&nbsp;&nbsp;&nbsp;Tension = ");
						client.print  (tension2);
						client.println("		V</h2>");
						client.print  ("		<h2 class=\"txtvert\">&nbsp;&nbsp;&nbsp;courant = ");
						client.print  (courant2);
						client.println("		mA</h2>");
						client.print  ("		<h2 class=\"txtvert\">&nbsp;&nbsp;&nbsp;puissance = ");
						client.print  (puissance2);
						client.println("		mW</h2>");
						client.print  ("		<h2 class=\"txtvert\">&nbsp;&nbsp;&nbsp;courant max = ");
						client.print  (max_courant2);
						client.println("		mA</h2>");
						// Courbe de courant
						client.println("		<script>");
						client.println("			// declaration variable");
						client.println("			var compteur;");
						client.println("			var mesure = [];");
						client.println("			var canvas = document.getElementById(\"canvas1\");");
						client.println("			var context = canvas.getContext(\"2d\");");
						client.println("			");
						client.println("			var min = 0;");
						client.print  ("			var max = "); client.print(max1); client.println(";");
						client.println("			var echelle = canvas.height / (max - min);");
						for(int compteur = 0; compteur != NB_MESURE; compteur++) {
							client.print  ("			mesure["); client.print(compteur); client.print("] = "); client.print(tabMesure1[compteur]); client.println(";");							
						}
						client.println("			");
						client.println("			// font");
						client.println("			context.beginPath();");
						client.println("			context.moveTo(0, 0);");
						client.println("			context.lineTo(0, canvas.height);");
						client.println("			context.lineTo(canvas.width, canvas.height);");
						client.println("			context.lineTo(canvas.width, 0);");
						client.println("			context.lineTo(0, 0);");
						client.println("			context.fillStyle = \"rgba(20, 20, 20, 0.3)\";");
						client.println("			context.fill();");
						client.println("			");
						client.println("			// entourage");
						client.println("			context.beginPath();");
						client.println("			context.moveTo(0, 0);");
						client.println("			context.lineTo(0, canvas.height);");
						client.println("			context.lineTo(canvas.width, canvas.height);");
						client.println("			context.lineTo(canvas.width, 0);");
						client.println("			context.lineTo(0, 0);");
						client.println("			context.strokeStyle = \"black\";");
						client.println("			context.stroke();");
						client.println("			");
						client.println("			// lignes horizontales");
						client.print  ("			for(compteur = min; compteur < max + 1; compteur += "); client.print(pasLignesHorizontales); client.println(") {");
						client.println("				context.beginPath();");
						client.println("				context.moveTo(0, canvas.height - compteur * echelle);");
						client.println("				context.lineTo(canvas.width, canvas.height - compteur * echelle);");
						client.println("				context.strokeStyle = \"red\";");
						client.println("				context.stroke();");
						client.println("				context.font = \"12pt Calibri,Geneva,Arial\";");
						client.println("				context.fillStyle = \"red\";");
						client.println("				context.fillText(compteur + \" mA\", 5, canvas.height - 3 - compteur * echelle);");	
						client.println("			}");
						client.println("			");
						client.println("			// Lignes verticales");
						client.println("			for(compteur = 0; compteur < canvas.width; compteur += 100) {");
						client.println("				context.beginPath();");
						client.println("				context.moveTo(compteur, 0);");
						client.println("				context.lineTo(compteur, canvas.height);");
						client.println("				context.strokeStyle = \"yellow\";");
						client.println("				context.stroke();");
						client.println("			}");
						client.println("			");
						client.println("			// courbe");
						client.println("			context.beginPath();");
						client.println("			context.moveTo(0, mesure[0]);");
						client.println("			for(compteur = 0; compteur != mesure.length; compteur++) {");
						client.println("				context.lineTo(compteur, canvas.height - mesure[compteur] * echelle);");
						client.println("			}");
						client.println("			context.strokeStyle = \"green\";");
						client.println("			context.stroke();");
						client.println("		</script>");
						client.println("	</body>");
						client.println("</html>");
						// La reponse HTTP se termine par une ligne blanche
						client.println();
						break;
					// Si on a un saut de ligne , on vide la string
					} else {    
						currentLine = "";
					}
				// Si on a autre chose qu'un retour chariot, on ajoute le caractére
				} else if (c != '\r') {  
					currentLine += c;
				}

			}
		}
    }
    // Ferme la connexion:
    client.stop();
    Serial.println("client deconnecté");
}

// ********************************************************************
//         ***** ***** *   * ***** *****   *  ***** *   * *****
//         *     *   * **  * *       *     *  *   * **  * *
//         ***   *   * * * * *       *     *  *   * * * * *****
//         *     *   * *  ** *       *     *  *   * *  **     *
//         *     ***** *   * *****   *     *  ***** *   * *****
// ********************************************************************

// ******************
// Affiche adresse IP
// ******************
void afficheAdresseIp(void) {
	Serial.println("--> afficheAdresseIp");
	lcd.setCursor(0, 0);
    lcd.print("Adresse IP:   ");
	lcd.setCursor(0, 1);
	lcd.print("                ");
	lcd.setCursor(0, 1);
	lcd.print(ip);
	Serial.println("<-- afficheAdresseIp");
}

// ******************
// Affiche sortie
// @Param la fonction
// @Param la sortie
// @Param la valeur
// ******************
void afficheSortie(uint8_t fonct, uint8_t sortie, float valeur) {
	Serial.println("--> afficheSortie");
	lcd.setCursor(0, 0);
	switch (fonct) {
	case SORTIE1:
		lcd.print("Sortie 1        ");
		break;
	case SORTIE2:
		lcd.print("Sortie 2        ");
		break;
	}
	lcd.setCursor(0, 1);
	switch (sortie) {
	case OFF:
		lcd.print("Off             ");
		break;
	case TENSION:
		lcd.print(valeur);
		lcd.setCursor(4, 1);
		lcd.print(" V          ");
		break;
	case COURANT:
		lcd.print(valeur);
		// Mise en page
		// Mesure superieure a 1000
		lcd.setCursor(7, 1);
		lcd.print(" mA       ");
		// Entre 1000 et 100
		if (valeur < 1000) {
			lcd.setCursor(6, 1);
			lcd.print(" mA        ");
		}
		// Entre 100 et 10
		if (valeur < 100) {
			lcd.setCursor(5, 1);
			lcd.print(" mA         ");
		}
		// Inferieure a 10
		if (valeur < 10) {
			lcd.setCursor(4, 1);
			lcd.print(" mA          ");
		}
		break;
	case PUISSANCE:
		lcd.print(valeur);
		// Mise en page
		// Mesure superieure a 1000
		lcd.setCursor(7, 1);
		lcd.print(" mW       ");
		// Entre 1000 et 100
		if (valeur < 1000) {
			lcd.setCursor(6, 1);
			lcd.print(" mW        ");
		}
		// Entre 100 et 10
		if (valeur < 100) {
			lcd.setCursor(5, 1);
			lcd.print(" mW         ");
		}
		// Inferieure a 10
		if (valeur < 10) {
			lcd.setCursor(4, 1);
			lcd.print(" mW          ");
		}
		break;
	case MAX_COURANT:
		lcd.print(valeur);
		// Mise en page
		// Mesure superieure a 1000
		lcd.setCursor(7, 1);
		lcd.print(" mA max   ");
		// Entre 1000 et 100
		if (valeur < 1000) {
			lcd.setCursor(6, 1);
			lcd.print(" mA max    ");
		}
		// Entre 100 et 10
		if (valeur < 100) {
			lcd.setCursor(5, 1);
			lcd.print(" mA max     ");
		}
		// Inferieure a 10
		if (valeur < 10) {
			lcd.setCursor(4, 1);
			lcd.print(" mA max      ");
		}
		break;
	}
	Serial.println("<-- afficheSortie");
}

// ***********************************
// Affichage du contraste lumiere auto
// @Param la valeur auto
// ***********************************
void afficheContLumAuto(bool valAuto) {
	Serial.println("--> afficheContLumAuto");
	lcd.setCursor(0, 0);
	lcd.print("Cont/Lum Auto   ");
	lcd.setCursor(0, 1);
	if (valAuto) {
		lcd.print("On              ");
	}
	else {
		lcd.print("Off             ");
	}
	Serial.println("<-- afficheContLumAuto");
}

// **************************************
// Affichage du verrouillage de la sortie
// @Param la valeur de verrouillage
// **************************************
void afficheVerrouillageSortie(bool valVerrouillage) {
	Serial.println("--> afficheVerrouillageSortie");
	lcd.setCursor(0, 0);
	lcd.print("Verrouillage    ");
	lcd.setCursor(0, 1);
	if (valVerrouillage) {
		lcd.print("On              ");
	}
	else {
		lcd.print("Off             ");
	}
	Serial.println("<-- afficheVerrouillageSortie");
}

// ***********************
// Affichage de la lumiere
// @Param la lumiere
// ***********************
void afficheLumiere(uint8_t lumiere) {
	Serial.println("--> afficheLumiere");
	lcd.setCursor(0, 0);
	lcd.print("Luminosite      ");
	char charVal[4];
	sprintf(charVal, "%d", lumiere);
	lcd.setCursor(0, 1);
	lcd.print("Valeur : ");
	lcd.setCursor(9, 1);
	lcd.print(charVal);
	// Un ou deux chiffres
	if(lumiere < 10) {
		lcd.setCursor(10, 1);
	} else {
		lcd.setCursor(11, 1);		
	}
	lcd.print("      ");
	Serial.println("<-- afficheLumiere");
}

// **********************
// Affichage du contraste
// @Param le contraste
// **********************
void afficheContraste(uint8_t contraste) {
	Serial.println("--> afficheContraste");
	lcd.setCursor(0, 0);
	lcd.print("Contraste       ");
	char charVal[4];
	sprintf(charVal, "%d", contraste);
	lcd.setCursor(0, 1);
	lcd.print("Valeur : ");
	lcd.setCursor(9, 1);
	lcd.print(charVal);
	// Un ou deux chiffres
	if(contraste < 10) {
		lcd.setCursor(10, 1);
	} else {
		lcd.setCursor(11, 1);		
	}
	lcd.print("      ");
	Serial.println("<-- afficheContraste");
}

// ****************
// Sortie 1 sur Off
// ****************
void sortie1Off(void) {
	Serial.println("--> sortie1Off");
	digitalWrite(RELAIS_SORTIE1, LOW);
	Serial.println("<-- sortie1Off");
}

// ***************
// Sortie 1 sur On
// ***************
void sortie1On(void) {
	Serial.println("--> sortie1On");
	digitalWrite(RELAIS_SORTIE1, HIGH);
	Serial.println("<-- sortie1On");
}

// ****************
// Sortie 2 sur Off
// ****************
void sortie2Off(void) {
	Serial.println("--> sortie2Off");
	digitalWrite(RELAIS_SORTIE2, LOW);
	Serial.println("<-- sortie2Off");
}

// ***************
// Sortie 2 sur On
// ***************
void sortie2On(void) {
	Serial.println("--> sortie2On");
	digitalWrite(RELAIS_SORTIE2, HIGH);
	Serial.println("<-- sortie2On");
}

// ****************************
// Rotation down de la sortie 1
// @Return La nouvelle sortie 1
// ****************************
uint8_t rotateDownSortie1(void) {
	Serial.println("--> rotateDownSortie1");
	Serial.println("<-- rotateDownSortie1");
	return rotateDownSortie(valSortie1);
}

// ****************************
// Rotation down de la sortie 2
// @Return La nouvelle sortie 2
// ****************************
uint8_t rotateDownSortie2(void) {
	Serial.println("--> rotateDownSortie2");
	Serial.println("<-- rotateDownSortie2");
	return rotateDownSortie(valSortie2);
}

// **************************
// Rotation down d'une sortie
// @Return La nouvelle sortie
// **************************
uint8_t rotateDownSortie(uint8_t valeur) {
	Serial.println("--> rotateDownSortie");
	switch (valeur) {
	case OFF: 
		Serial.println("<-- rotateDownSortie"); 
		return TENSION;
	case TENSION: 
		Serial.println("<-- rotateDownSortie"); 
		return COURANT;
	case COURANT: 
		Serial.println("<-- rotateDownSortie"); 
		return PUISSANCE;
	case PUISSANCE: 
		Serial.println("<-- rotateDownSortie"); 
		return MAX_COURANT;
	case MAX_COURANT:  
		Serial.println("<-- rotateDownSortie"); 
		// Si la sortie est verouillée, on évite le OFF
		if(verrouillageSorties) {
			return TENSION;
		} else {
			return OFF;
		}
	}
}

// ****************************
// Rotation up de la sortie 1
// @Return La nouvelle sortie 1
// ****************************
uint8_t rotateUpSortie1(void) {
	Serial.println("--> rotateUpSortie1");
	Serial.println("<-- rotateUpSortie1");
	return rotateUpSortie(valSortie1);
}

// ****************************
// Rotation up de la sortie 2
// @Return La nouvelle sortie 2
// ****************************
uint8_t rotateUpSortie2(void) {
	Serial.println("--> rotateUpSortie2");
	Serial.println("<-- rotateUpSortie2");
	return rotateUpSortie(valSortie2);
}

// **************************
// Rotation up d'une sortie
// @Return La nouvelle sortie
// **************************
uint8_t rotateUpSortie(uint8_t valeur) {
	Serial.println("--> rotateUpSortie");
	switch (valeur) {
	case OFF: 
		Serial.println("<-- rotateUpSortie"); 
		return MAX_COURANT;
	case TENSION: 
		Serial.println("<-- rotateUpSortie"); 
		// Si la sortie est verouillée, on évite le OFF
		if(verrouillageSorties) {
			return MAX_COURANT;
		} else {
			return OFF;
		}
	case COURANT: 
		Serial.println("<-- rotateUpSortie"); 
		return TENSION;
	case PUISSANCE: 
		Serial.println("<-- rotateUpSortie"); 
		return COURANT;
	case MAX_COURANT: 
		Serial.println("<-- rotateUpSortie"); 
		return PUISSANCE;
	}
}

// *********************************
// Inverse le contraste auto
// @Return le contraste auto inverse
// *********************************
bool changeContrasteAuto(void) {
	Serial.println("--> changeContrasteAuto");
	Serial.println("<-- changeContrasteAuto");
	return !lumContrasteAuto;
}

// ******************************************
// Inverse le verrouillage des sorties
// @Return le verrouillage des sortie inverse
// ******************************************
bool changeVerrouillageSorties(void) {
	Serial.println("--> changeVerrouillageSorties");
	Serial.println("<-- changeVerrouillageSorties");
	return !verrouillageSorties;
}

// ****************************************
// Rotation de la touche fonction
// @Return la nouvelles valeur de la touche
// ****************************************
uint8_t rotateFonction(uint8_t valeur) {
	Serial.println("--> rotateFonction");
	valeur++;
	if (valeur > ADRESSE_IP) {
		valeur = CONTRASTE;
	}
	Serial.println("<-- rotateFonction");
	return valeur;
}

// ****************************************************
// Baisse le contraste
// @Return la valeur du contraste baisse si possible
// @Remark désactive le contraste et la luminosité auto
// ****************************************************
uint8_t downContraste(void) {
	Serial.println("--> downContraste");
	lumContrasteAuto = false;
	Serial.println("<-- downContraste");
	return downAfficheur(valContraste);
}

// ****************************************************
// Baisse la lumiere
// @Return la valeur de la lumiere baisse si possible
// @Remark désactive le contraste et la luminosité auto
// ****************************************************
uint8_t downLumiere(void) {
	Serial.println("--> downLumiere");
	lumContrasteAuto = false;
	Serial.println("<-- downLumiere");
	return downAfficheur(valLumiere);
}

// *************************************
// Baisse une valeur de l'afficheur
// @Return la valeur baissée si possible
// *************************************
uint8_t downAfficheur(uint8_t valeur) {
	Serial.println("--> downAfficheur");
	// 0 est le contraste et la luminosite max
	if (valeur > 5) {
		valeur -= 5;
	} else {
		valeur = 0;
	}
	Serial.print("Valeur = ");
	Serial.println(valeur);
	Serial.println("<-- downAfficheur");
	return valeur;
}

// ****************************************************
// Augmente le contraste
// @Return la valeur du contraste augmente si possible
// @Remark désactive le contraste et la luminosité auto
// ****************************************************
uint8_t upContraste(void) {
	Serial.println("--> upContraste");
	lumContrasteAuto = false;
	Serial.println("<-- upContraste");
	return upAfficheur(valContraste);
}

// ****************************************************
// Augmente la lumiere
// @Return la valeur de la lumiere augmente si possible
// @Remark désactive le contraste et la luminosité auto
// ****************************************************
uint8_t upLumiere(void) {
	Serial.println("--> upLumiere");
	lumContrasteAuto = false;
	Serial.println("<-- upLumiere");
	return upAfficheur(valLumiere);
}

// **************************************
// Augmente une valeur de l'afficheur
// @Return la valeur augmente si possible
// **************************************
uint8_t upAfficheur(uint8_t valeur) {
	Serial.println("--> upAfficheur");
	// 80 est le contraste et la luminosite min (quasi eteint)
	if (valeur < 80) {
		valeur += 5;
	}
	Serial.print("Valeur = ");
	Serial.println(valeur);
	Serial.println("<-- upAfficheur");
	return valeur;
}

// *******************************************************************
// Valeur de la touche des fonctions
// Code duplique pour eviter des variables locales 
// @Return l'etat de la touche avec historique
// @Remark table de verite
// valeur precedente    valeur lue Resultat Explication
// LOW                  LOW        false    Pas d'action
// HIGH                 LOW        false    Touche relache
// LOW                  HIGH       true     Touche vient d'etre active
// HIGH                 HIGH       false    Action deja traitee
// *******************************************************************
bool touchSwitchFnct(void) {
	Serial.println("--> touchSwitchFnct");
	uint8_t valeur = digitalRead(TOUCHE_FONCTION);
	if (valeur == HIGH && valPrevTouchSwitchFnct == LOW) {
		valPrevTouchSwitchFnct = valeur;
		Serial.println("<-- touchSwitchFnct");
		return true;
	} else {
		valPrevTouchSwitchFnct = valeur;
		Serial.println("<-- touchSwitchFnct");
		return false;
	}
}

// *******************************************************************
// Valeur de la touche des fonctions moins
// Code duplique pour eviter des variables locales 
// @Return l'etat de la touche avec historique
// @Remark table de verite
// valeur precedente    valeur lue Resultat Explication
// LOW                  LOW        false    Pas d'action
// HIGH                 LOW        false    Touche relache
// LOW                  HIGH       true     Touche vient d'etre active
// HIGH                 HIGH       false    Action deja traitee
// *******************************************************************
bool touchSwitchFnctMoins(void) {
	Serial.println("--> touchSwitchFnctMoins");
	uint8_t valeur = digitalRead(TOUCHE_MOINS);
	if (valeur == HIGH && valPrevTouchSwitchFnctMoins == LOW) {
		valPrevTouchSwitchFnctMoins = valeur;
		Serial.println("<-- touchSwitchFnctMoins");
		return true;
	} else {
		valPrevTouchSwitchFnctMoins = valeur;
		Serial.println("<-- touchSwitchFnctMoins");
		return false;
	}
}

// *******************************************************************
// Valeur de la touche des fonctions plus
// Code duplique pour eviter des variables locales 
// @Return l'etat de la touche avec historique
// @Remark table de verite
// valeur precedente    valeur lue Resultat Explication
// LOW                  LOW        false    Pas d'action
// HIGH                 LOW        false    Touche relache
// LOW                  HIGH       true     Touche vient d'etre active
// HIGH                 HIGH       false    Action deja traitee
// *******************************************************************
bool touchSwitchFnctPlus(void) {
	Serial.println("--> touchSwitchFnctPlus");
	uint8_t valeur = digitalRead(TOUCHE_PLUS);
	if (valeur == HIGH && valPrevTouchSwitchFnctPlus == LOW) {
		valPrevTouchSwitchFnctPlus = valeur;
		Serial.println("<-- touchSwitchFnctPlus");
		return true;
	} else {
		valPrevTouchSwitchFnctPlus = valeur;
		Serial.println("<-- touchSwitchFnctPlus");
		return false;
	}
}
