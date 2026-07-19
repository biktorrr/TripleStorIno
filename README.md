# Arduino RDF Triple Store with SPARQL Endpoint

A lightweight RDF triple store running on an **Arduino Uno** with an **Arduino Ethernet Shield (W5100)** and an optional **16x2 LCD**. The project demonstrates that a resource-constrained microcontroller can expose a simple Semantic Web interface over HTTP and use RDF triples to control physical devices.

## Features

* Lightweight in-memory RDF triple store
* HTTP server using the Arduino Ethernet library
* SPARQL-like endpoint over HTTP GET
* Supports:

  * `SELECT`
  * `INSERT DATA`
  * `DELETE DATA`
  * `DELETE WHERE`
* Duplicate triple detection
* 16x2 LCD displaying the current number of stored triples
* RDF-controlled LED actuators
* Designed to run on an Arduino Uno (2 KB SRAM)

## Hardware

* Arduino Uno
* Arduino Ethernet Shield (W5100)
* LCM1602C 16x2 LCD (parallel interface)
* 10 kΩ potentiometer for LCD contrast
* Red LED + 220 Ω resistor
* Green LED + 220 Ω resistor

## Wiring

### LCD

| LCD pin | Arduino       |
| ------- | ------------- |
| RS      | A0            |
| E       | A1            |
| D4      | A2            |
| D5      | A3            |
| D6      | A4            |
| D7      | A5            |
| RW      | GND           |
| VSS     | GND           |
| VDD     | 5V            |
| VO      | Potentiometer |
| A       | 5V            |
| K       | GND           |

The Ethernet shield uses D10–D13 for SPI.

### LEDs

| Component           | Arduino                    |
| ------------------- | -------------------------- |
| Red LED anode (+)   | D6                         |
| Green LED anode (+) | D7                         |
| LED cathodes (-)    | GND through 220 Ω resistor |

## HTTP API

The Arduino exposes a SPARQL endpoint:

```text
http://<arduino-ip>/sparql?query=<SPARQL query>
```

Example:

```text
http://192.168.178.50/sparql?query=...
```

---

# SPARQL Operations

## INSERT DATA

Add a triple:

```sparql
INSERT DATA {
  <alice> <knows> <bob>
}
```

Example:

```text
http://192.168.178.50/sparql?query=INSERT+DATA+%7B+%3Calice%3E+%3Cknows%3E+%3Cbob%3E+%7D
```

Response:

```text
OK
triples=1
```

---

## SELECT

Retrieve all triples:

```sparql
SELECT ?s ?p ?o
WHERE {
  ?s ?p ?o
}
```

Example output:

```text
alice | knows | bob
bob | type | person
```

---

## DELETE DATA

Delete an exact triple:

```sparql
DELETE DATA {
  <alice> <knows> <bob>
}
```

---

## DELETE WHERE

Delete triples matching a pattern.

Delete all triples:

```sparql
DELETE WHERE {
  ?s ?p ?o
}
```

Delete all relationships of Alice:

```sparql
DELETE WHERE {
  <alice> <knows> ?o
}
```

Delete all type statements:

```sparql
DELETE WHERE {
  ?s <type> ?o
}
```

---

# RDF Controlled LEDs

The LEDs are controlled by RDF triples.

The Arduino continuously interprets the RDF graph and updates the physical outputs.

## Turn on red LED

Insert:

```sparql
INSERT DATA {
  <ledred> <status> <on>
}
```

The Arduino switches:

```
D6 = HIGH
```

## Turn on green LED

Insert:

```sparql
INSERT DATA {
  <ledgreen> <status> <on>
}
```

The Arduino switches:

```
D7 = HIGH
```

The RDF graph acts as the device state:

```turtle
<ledred>   <status> <on>.
<ledgreen> <status> <on>.
```

---

# Example Workflow

Clear the device:

```sparql
DELETE WHERE {
  ?s ?p ?o
}
```

Add a device state:

```sparql
INSERT DATA {
  <ledred> <status> <on>
}
```

Query the graph:

```sparql
SELECT ?s ?p ?o
WHERE {
  ?s ?p ?o
}
```

Result:

```text
ledred | status | on
```

The physical LED reflects the RDF state.

---

# Limitations

This project is intentionally minimal to fit within Arduino Uno constraints.

Current limitations:

* Small in-memory graph
* Limited number of triples (depends on configuration)
* Short resource names
* Simplified SPARQL parser
* No prefixes (`PREFIX`)
* No full SPARQL algebra
* No inference/reasoning
* No persistence after reboot

## Future Work

Possible extensions:

* SD card persistence
* RDF/Turtle export endpoint
* Full triple pattern matching
* SPARQL prefixes
* RDFS reasoning
* RFID-controlled resources
* Servo and sensor control through RDF
* Generic semantic actuator framework
* Larger Arduino boards (Mega, ESP32)

# Motivation

This project explores how Semantic Web technologies can be embedded into very small devices.

Instead of exposing device-specific APIs, the Arduino exposes a graph of resources and states:

```
RDF graph
    |
    v
SPARQL endpoint
    |
    v
Physical world
```

The goal is to demonstrate a tiny semantic IoT node where devices can describe and control themselves using Linked Data principles.
