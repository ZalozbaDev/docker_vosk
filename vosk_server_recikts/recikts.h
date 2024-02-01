/*
 * Spracherkennungsbibliothek des IKTS
 * 
 * Copyright: Fraunhofer IKTS, Dresden
 * 
 * Diese Software wurde durch das Fraunhofer Insitut für keramische Technologien
 * und Systeme (IKTS) erstellt.
 * Die Software wird der Stiftung für das sorbische Volk im Rahmen des Auftrags zur
 * "Vorbereitung der Spracherkennung für das Obersorbische für eine Diktierfunktion"
 * bereitgestellt.
 * 
 *   Die Software zur Spracherkennung und zur Aufbereitung der Modelle
 *   für die Spracherkennung wird dem Auftraggeber als Bibliothek zur Verfügung gestellt.
 * 
 *   Diese Anwendung darf nur auf Rechnern und Servern der Stiftung für das sorbische Volk
 *   benutzt werden und nicht an Dritte weitergegeben werden.
 *   Außerdem darf die Anwendung nur für die Erkennung ober- und niedersorbischer Sprache
 *   benutzt werden.
 * 
 *   Sämtliche gelieferte Software sowie die Dokumentation darf ausdrücklich nicht
 *   dupliziert oder Dritten zugänglich gemacht werden.
 *   Sie darf nur im Objektcode auf der in der Dokumentation spezifizierten Hardware
 *   verwendet werden.
 * 
 *   In dem Programm testbld sind
 *   teile der Softwarepakete openfst (http://www.openfst.org/) und
 *   opengrm (http://www.opengrm.org/) einkompiliert.
 *   Die Lizenz diese Softwarepakete ist daher zu beachten:
 * 
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use these files except in compliance with the License.
 *   You may obtain a copy of the License at
 * 
 *       http://www.apache.org/licenses/LICENSE-2.0
 * 
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 * 
 *   Copyright 2005-2018 Google, Inc.
 */

#define _RECIKTS2_H

#include <stdint.h>

/* Recognizer configuration */
struct cfgikts {
  void *buf;          /* Configuration and analysis parameters */
  uint32_t size;      /* Number of bytes in buf */
};

/* Return version code of library */
const char* recikts_version();

/* Get one error message from error buffer
 *   buf:    Error message will be written here (terminated with '\0')
 *   ize:    Size of message buffer buf (message will be truncated if too long)
 *   return: No error in buffer: 0
 *           Error message read: error code
 */
char recikts_err(char* buf,int size);

/* Read configuration from file
 *   fn:     File name
 *   cfg:    Pointer to instance of cfgikts for returned configuration
 *   return: success: 1; failure: 0 (use recikts_err())
 */
char cfgikts_load(const char *fn,struct cfgikts *cfg);

/* Free memory of configuration
 *   cfg:    Configuration
 *   return: success: 1; failure: 0 (use recikts_err())
 */
char cfgikts_free(struct cfgikts *cfg);

/* Start recognizer with configuration
 *   cfg:    Configuration (from cfgikts_load or cfgikts_pack)
 *   return: success: 1; failure: 0 (use recikts_err())
 */
char recikts_start(struct cfgikts cfg);

/* Stop, finish and unload recognizer
 * (Callback function may be called while this function is running)
 *   return: success: 1; failure: 0 (use recikts_err())
 */
char recikts_stop();

/* Send audio data to recognizer and process these data
 * (Callback function may be called while this function is running)
 *   buf:     Audio data (Mono, sample rate 16 kHz)
 *   samples: Number of samples in buffer buf
 *   return:  success: 1; failure: 0 (use recikts_err())
 */
char recikts_audio(int16_t *buf,uint32_t samples);

/* Stop and finish one recognition and start a new one with the same configuration
 * Call this between voice segments.
 * (Callback function may be called while this function is running)
 *   return:  success: 1; failure: 0 (use recikts_err())
 */
char recikts_restart();


/* Callback function data */
struct recikts_callback_dat {
  char word[255];  /* Recognized word or token ("": no value) */
  uint32_t tstart; /* Begin in milli seconds */
  uint32_t tend;   /* End in milli seconds */
  float nld;       /* Likelihood of word (0..INF, greater value = uncertain result) */
  float sigmax;    /* Max. signal amplitude (-1.f: no value) */
  float trgmax;    /* Max. trigger value (-1.f: no value) */
  char  trgchg;    /* Trigger change (-1: no change, 0: off, 1: on) */
};
/* Callback function
 *   dat:      Callback data
 *   userdata: Pointer provided in register callback
 */
typedef void (*recikts_callback_fnc)(struct recikts_callback_dat dat,void *userdata);
/* Register callback function
 *   fnc:      Callback function
 *   userdata: Pointer provided to callback function
 *   return:   success: 1; failure: 0 (use recikts_err())
 */
char recikts_callback_register(recikts_callback_fnc fnc,void *userdata);

/* Reimplemtation of glibc strsep */
char* mstrsep(char** stringp, const char* delim);

