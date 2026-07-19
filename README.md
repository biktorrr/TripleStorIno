# Arduino RDF Triple Store with SPARQL Endpoint

A lightweight RDF triple store running on an **Arduino Uno** with an **Arduino Ethernet Shield (W5100)** and an optional **16x2 LCD**. The project demonstrates that a resource-constrained microcontroller can expose a simple Semantic Web interface over HTTP.

## Features

* Lightweight in-memory RDF triple store
* Simple HTTP server using the official Ethernet library
* SPARQL-like endpoint over HTTP GET
* Supports:

  * `SELECT`
  * `INSERT DATA`
* 16x2 LCD displaying the current number of stored triples
* Designed to run on an Arduino Uno (2 KB SRAM)

## Hardware

* Arduino Uno
* Official Arduino Ethernet Shield (W5100)
* LCM1602C 16x2 LCD (parallel interface)
* 10 kΩ potentiometer (LCD contrast)

## LCD Wiring

| LCD | Arduino       |
| --- | ------------- |
| RS  | A0            |
| E   | A1            |
| D4  | A2            |
| D5  | A3            |
| D6  | A4            |
| D7  | A5            |
| RW  | GND           |
| VSS | GND           |
| VDD | 5V            |
| VO  | Potentiometer |
| A   | 5V            |
| K   | GND           |

The Ethernet shield uses pins D10–D13 for SPI.

## HTTP API

The Arduino exposes a SPARQL endpoint at:

```text
http://<arduino-ip>/sparql?query=...
```

### Insert

```sparql
INSERT DATA {
  <alice> <knows> <bob>
}
```

Example URL:

```text
http://192.168.178.50/sparql?query=INSERT+DATA+%7B+%3Calice%3E+%3Cknows%3E+%3Cbob%3E+%7D
```

### Select all triples

```sparql
SELECT ?s ?p ?o
WHERE {
  ?s ?p ?o
}
```

Example URL:

```text
http://192.168.178.50/sparql?query=SELECT+%3Fs+%3Fp+%3Fo+WHERE+%7B+%3Fs+%3Fp+%3Fo+%7D
```

## Example Output

```text
alice | knows | bob
bob | type | person
```

## Limitations

This project is intentionally minimal to fit within the Arduino Uno's hardware constraints.

Current limitations include:

* Approximately 15–30 triples (depending on configuration)
* Short resource names (fixed-length strings)
* Single triple pattern in `WHERE`
* No `FILTER`
* No `OPTIONAL`
* No `UNION`
* No prefixes (`PREFIX`)
* No inference or reasoning
* Data is stored only in RAM (no persistence)

## Motivation

The project explores how Semantic Web concepts can be applied to embedded systems and Internet of Things (IoT) devices. Instead of exposing device-specific APIs, the Arduino exposes its state through RDF and can be queried and updated using SPARQL.

This serves as a proof of concept for lightweight semantic devices that can participate directly in Linked Data ecosystems.

## Future Work

Possible extensions include:

* RDF/Turtle export endpoint
* SD card persistence
* SPARQL `DELETE DATA`
* Multiple triple patterns
* Prefix support
* RDF serialization formats
* Semantic control of LEDs, servos, sensors and RFID readers
* Basic RDFS reasoning
* Support for additional Arduino boards with larger memory (e.g. Mega 2560)
