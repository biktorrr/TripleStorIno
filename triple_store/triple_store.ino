/*
  Arduino UNO RDF Triple Store + SPARQL GET endpoint

  Hardware:
  - Arduino UNO
  - Official Ethernet Shield W5100
  - LCM1602C LCD 16x2 parallel

  Supported:
  SELECT queries
  INSERT DATA

  Storage:
  - 100 RDF resources
  - 50 triples

*/

#include <SPI.h>
#include <Ethernet.h>
#include <LiquidCrystal.h>


// ------
// LEDS
// ------

#define RED_LED_PIN 6
#define GREEN_LED_PIN 7

// -------------------------
// LCD
// -------------------------

// RS,E,D4,D5,D6,D7
LiquidCrystal lcd(A0,A1,A2,A3,A4,A5);


// -------------------------
// Ethernet
// -------------------------

byte mac[] = {
  0xDE,0xAD,0xBE,0xEF,0xFE,0xED
};

IPAddress ip(192,168,178,50);

EthernetServer server(80);


// -------------------------
// RDF STORE
// -------------------------

#define MAX_RESOURCES 10
#define MAX_TRIPLES 10
#define MAX_TERM 10


struct Triple {
  byte s;
  byte p;
  byte o;
};


char dictionary[MAX_RESOURCES][MAX_TERM];

int resourceCount = 0;

Triple triples[MAX_TRIPLES];

int tripleCount = 0;



// -------------------------
// Find or create resource ID
// -------------------------

byte getResource(char *value)
{

  // already exists?
  for(int i=0;i<resourceCount;i++)
  {
    if(strcmp(dictionary[i],value)==0)
      return i;
  }


  // create new
  if(resourceCount < MAX_RESOURCES)
  {
    strcpy(dictionary[resourceCount],value);

    return resourceCount++;
  }

  return 255;
}



// -------------------------
// Delete RDF triple
// -------------------------
bool deleteTriple(char *s, char *p, char *o)
{
    byte sid = 255;
    byte pid = 255;
    byte oid = 255;

    // Find resource IDs
    for (int i = 0; i < resourceCount; i++) {
        if (strcmp(dictionary[i], s) == 0) sid = i;
        if (strcmp(dictionary[i], p) == 0) pid = i;
        if (strcmp(dictionary[i], o) == 0) oid = i;
    }

    if (sid == 255 || pid == 255 || oid == 255)
        return false;

    // Find matching triple
    for (int i = 0; i < tripleCount; i++) {

        if (triples[i].s == sid &&
            triples[i].p == pid &&
            triples[i].o == oid)
        {
            // Shift remaining triples down
            for (int j = i; j < tripleCount - 1; j++)
                triples[j] = triples[j + 1];

            tripleCount--;

            return true;
        }
    }

    return false;
}

// -------------------------
// Delete WHERE 
// -------------------------

bool deleteWhere(char *patternS, char *patternP, char *patternO)
{
    int deleted = 0;

    // Iterate backwards so shifting the array is easy
    for(int i = tripleCount - 1; i >= 0; i--)
    {
        bool match = true;

        if(patternS[0] != '?')
        {
            if(strcmp(dictionary[triples[i].s], patternS) != 0)
                match = false;
        }

        if(patternP[0] != '?')
        {
            if(strcmp(dictionary[triples[i].p], patternP) != 0)
                match = false;
        }

        if(patternO[0] != '?')
        {
            if(strcmp(dictionary[triples[i].o], patternO) != 0)
                match = false;
        }

        if(match)
        {
            // Remove triple by shifting remaining entries
            for(int j = i; j < tripleCount - 1; j++)
                triples[j] = triples[j + 1];

            tripleCount--;
            deleted++;
        }
    }

    return deleted > 0;
}

// -------------------------
// Add RDF triple
// -------------------------
bool addTriple(char *s, char *p, char *o)
{

  byte sid = getResource(s);
  byte pid = getResource(p);
  byte oid = getResource(o);


  if(sid == 255 || pid == 255 || oid == 255)
    return false;


  // Check if triple already exists
  for(int i=0; i<tripleCount; i++)
  {
    if(triples[i].s == sid &&
       triples[i].p == pid &&
       triples[i].o == oid)
    {
      return false;   // already stored
    }
  }


  if(tripleCount >= MAX_TRIPLES)
    return false;


  triples[tripleCount].s = sid;
  triples[tripleCount].p = pid;
  triples[tripleCount].o = oid;

  tripleCount++;

  return true;
}



// -------------------------
// LCD update
// -------------------------

void updateLCD()
{
  lcd.clear();

  lcd.setCursor(0,0);
  lcd.print("RDF Store");

  lcd.setCursor(0,1);
  lcd.print("Triples:");
  lcd.print(tripleCount);
}

// PART 2
// ======================================================
// SPARQL PARSER
// ======================================================


char queryBuffer[300];


// remove URL encoding
void urlDecode(char *str)
{
  char *src=str;
  char *dst=str;

  while(*src)
  {
    if(*src=='%' && src[1] && src[2])
    {
      char hex[3];
      hex[0]=src[1];
      hex[1]=src[2];
      hex[2]=0;

      *dst=(char)strtol(hex,NULL,16);

      src+=3;
    }
    else if(*src=='+')
    {
      *dst=' ';
      src++;
    }
    else
    {
      *dst=*src;
      src++;
    }

    dst++;
  }

  *dst=0;
}



// remove < > around URI
void cleanToken(char *token)
{
  int len=strlen(token);

  if(token[0]=='<')
  {
    memmove(token,token+1,len-2);
    token[len-2]=0;
  }
}



// find substring
bool contains(char *text,char *key)
{
  return strstr(text,key)!=NULL;
}


// ------------------------------------------------------
// DELETE WHERE
// ------------------------------------------------------
bool parseDeleteWhere(char *query)
{
    char patternS[32];
    char patternP[32];
    char patternO[32];

    char *where = strstr(query, "WHERE");

    if(!where)
        return false;

    char *body = strchr(where, '{');

    if(!body)
        return false;

    sscanf(
        body,
        "{ %31s %31s %31s",
        patternS,
        patternP,
        patternO
    );

    cleanToken(patternS);
    cleanToken(patternP);
    cleanToken(patternO);

    return deleteWhere(
        patternS,
        patternP,
        patternO
    );
}

// ------------------------------------------------------
// DELETE DATA
// ------------------------------------------------------

bool parseDelete(char *query)
{
    char *start = strstr(query, "{");

    if (!start)
        return false;

    char s[32];
    char p[32];
    char o[32];

    int found = sscanf(
        start,
        "{ <%31[^>]> <%31[^>]> <%31[^>]>",
        s, p, o);

    if (found == 3)
        return deleteTriple(s, p, o);

    return false;
}

// ------------------------------------------------------
// INSERT DATA
// ------------------------------------------------------

bool parseInsert(char *query)
{

  char *start=strstr(query,"{");

  if(!start)
    return false;


  char s[32];
  char p[32];
  char o[32];


  int found=sscanf(
    start,
    "{ <%31[^>]>\n <%31[^>]>\n <%31[^>]>",
    s,p,o
  );


  // allow spaces instead of new lines
  if(found!=3)
  {
    found=sscanf(
      start,
      "{ <%31[^>]> <%31[^>]> <%31[^>]>",
      s,p,o
    );
  }


  if(found==3)
  {
    addTriple(s,p,o);
    return true;
  }


  return false;
}



// ------------------------------------------------------
// SELECT query
// ------------------------------------------------------

void parseSelect(char *query,EthernetClient &client)
{

  char patternS[32];
  char patternP[32];
  char patternO[32];


  char *where=strstr(query,"WHERE");

  if(!where)
  {
    client.println("ERROR");
    return;
  }


  char *body=strchr(where,'{');


  if(!body)
  {
    client.println("ERROR");
    return;
  }


  sscanf(
    body,
    "{ %31s %31s %31s",
    patternS,
    patternP,
    patternO
  );


  cleanToken(patternS);
  cleanToken(patternP);
  cleanToken(patternO);



  for(int i=0;i<tripleCount;i++)
  {

    bool match=true;


    if(patternS[0]!='?')
    {
      if(strcmp(
        dictionary[triples[i].s],
        patternS)!=0)
        match=false;
    }


    if(patternP[0]!='?')
    {
      if(strcmp(
        dictionary[triples[i].p],
        patternP)!=0)
        match=false;
    }


    if(patternO[0]!='?')
    {
      if(strcmp(
        dictionary[triples[i].o],
        patternO)!=0)
        match=false;
    }


    if(match)
    {
      client.print(
        dictionary[triples[i].s]
      );

      client.print(" | ");

      client.print(
        dictionary[triples[i].p]
      );

      client.print(" | ");

      client.println(
        dictionary[triples[i].o]
      );
    }

  }

}

// PART 3

// ======================================================
// HTTP SERVER
// ======================================================


void sendHeader(EthernetClient &client)
{
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/plain");
  client.println("Connection: close");
  client.println();
}



// ------------------------------------------------------
// process SPARQL request
// ------------------------------------------------------

void processSPARQL(
  char *query,
  EthernetClient &client)
{

  urlDecode(query);


  lcd.clear();

  lcd.setCursor(0,0);
  lcd.print("SPARQL");


  if(contains(query,"INSERT DATA"))
  {

    bool ok=parseInsert(query);


    if(ok)
    {
      client.println("OK");
      client.print("triples=");
      client.println(tripleCount);

      lcd.setCursor(0,1);
      lcd.print("INSERT OK");
    }
    else
    {
      client.println("INSERT ERROR");

      lcd.setCursor(0,1);
      lcd.print("ERROR");
    }


    updateLCD();

  }

  else if (contains(query, "DELETE DATA"))
  {
      bool ok = parseDelete(query);

      if (ok)
      {
          client.println(F("OK"));
          client.print(F("triples="));
          client.println(tripleCount);
      }
      else
      {
          client.println(F("DELETE ERROR"));
      }

      updateLCD();
  }
  else if(contains(query,"SELECT"))
  {

    lcd.setCursor(0,1);
    lcd.print("SELECT");


    parseSelect(query,client);


    delay(500);

    updateLCD();

  }

else if (contains(query, "DELETE WHERE"))
{
    bool ok = parseDeleteWhere(query);

    if(ok)
    {
        client.println(F("OK"));
        client.print(F("triples="));
        client.println(tripleCount);
    }
    else
    {
        client.println(F("No triples deleted"));
    }

    updateLCD();
}

  else
  {
    client.println("Unknown SPARQL");
  }

  updateLEDs();

}



// ------------------------------------------------------
// HTTP connection handler
// ------------------------------------------------------

void handleEthernet()
{

  EthernetClient client =
    server.available();


  if(!client)
    return;


  bool blankLine=true;

  char request[400];

  int index=0;



  while(client.connected())
  {

    if(client.available())
    {

      char c=client.read();


      if(index<399)
        request[index++]=c;


      request[index]=0;



      if(c=='\n' && blankLine)
      {

        // first line:
        // GET /sparql?query=xxxx HTTP/1.1


        sendHeader(client);



        char *q=strstr(
          request,
          "query="
        );


        if(q)
        {

          q+=6;


          char *end=strchr(q,' ');


          if(end)
            *end=0;


          strcpy(
            queryBuffer,
            q
          );


          processSPARQL(
            queryBuffer,
            client
          );

        }
        else
        {
          client.println(
            "Arduino RDF Store"
          );

          client.print(
            "Triples="
          );

          client.println(
            tripleCount
          );
        }



        break;

      }


      if(c=='\n')
        blankLine=true;

      else if(c!='\r')
        blankLine=false;


    }

  }


  delay(1);
  client.stop();

}

// ------------
// UPDATE LEDS
// ------------

void updateLEDs()
{
  bool red = false;
  bool green = false;


  for(int i=0; i<tripleCount; i++)
  {
    char *s = dictionary[triples[i].s];
    char *p = dictionary[triples[i].p];
    char *o = dictionary[triples[i].o];


    if(strcmp(s,"ledred")==0 &&
       strcmp(p,"status")==0 &&
       strcmp(o,"on")==0)
    {
      red = true;
    }


    if(strcmp(s,"ledgreen")==0 &&
       strcmp(p,"status")==0 &&
       strcmp(o,"on")==0)
    {
      green = true;
    }
  }


  digitalWrite(RED_LED_PIN, red);
  digitalWrite(GREEN_LED_PIN, green);
}

// ======================================================
// ARDUINO SETUP
// ======================================================


void setup()
{

  // -------------------------
  // LEDS
  // -------------------------
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);

  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(GREEN_LED_PIN, LOW);

// --- serial ---
  Serial.begin(9600);


  // -------------------------
  // LCD
  // -------------------------

  lcd.begin(16,2);
  lcd.clear();

  lcd.setCursor(0,0);
  lcd.print("Starting RDF");
  lcd.setCursor(0,1);
  lcd.print("Store...");


  delay(1000);



  // -------------------------
  // Ethernet
  // -------------------------

 pinMode(10, OUTPUT);
 digitalWrite(10, HIGH);

  Serial.println(F("Starting Ethernet"));

Ethernet.begin(mac,ip);

Serial.println(F("Ethernet OK"));

  delay(1000);


  server.begin();


  Serial.print("IP: ");
  Serial.println(Ethernet.localIP());



  // -------------------------
  // Example RDF data
  // -------------------------

  addTriple(
    "alice",
    "knows",
    "bob"
  );


  addTriple(
    "bob",
    "type",
    "person"
  );


  updateLCD();



}



// ======================================================
// MAIN LOOP
// ======================================================


void loop()
{

  handleEthernet();

}