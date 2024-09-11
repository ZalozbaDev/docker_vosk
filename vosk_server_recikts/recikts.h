/*
 * Spracherkennungsbibliothek des IKTS
 * 
 * Copyright: Fraunhofer IKTS, Dresden
 * 
 *     Diese Software wurde durch das Fraunhofer Insitut für keramische Technologien
 *     und Systeme (IKTS) erstellt.
 *     Die Software wird der Stiftung für das sorbische Volk im Rahmen des Auftrags zur
 *     "Vorbereitung der Spracherkennung für das Obersorbische für eine Diktierfunktion"
 *     bereitgestellt.
 * 
 *     Die Software zur Spracherkennung und zur Aufbereitung der Modelle
 *     für die Spracherkennung wird dem Auftraggeber als Bibliothek zur Verfügung gestellt.
 * 
 *     Diese Anwendung darf nur auf Rechnern und Servern der Stiftung für das sorbische Volk
 *     benutzt werden und nicht an Dritte weitergegeben werden.
 *     Außerdem darf die Anwendung nur für die Erkennung ober- und niedersorbischer Sprache
 *     benutzt werden.
 * 
 *     Sämtliche gelieferte Software sowie die Dokumentation darf ausdrücklich nicht
 *     dupliziert oder Dritten zugänglich gemacht werden.
 *     Sie darf nur im Objektcode auf der in der Dokumentation spezifizierten Hardware
 *     verwendet werden.
 * 
 * Die kompilierten Progamme und Bibliotheken ``recikts64rel.so``, ``testrec``, ``testrec_static`` und ``testbld``
 * enthalten Teile folgender Softwarpakete:
 * 
 * * **WebRTC** (https://webrtc.googlesource.com/src) mit folgender Lizenz:
 * 
 *     Copyright (c) 2011, The WebRTC project authors. All rights reserved.
 * 
 *     Redistribution and use in source and binary forms, with or without
 *     modification, are permitted provided that the following conditions are
 *     met:
 * 
 *       * Redistributions of source code must retain the above copyright
 *         notice, this list of conditions and the following disclaimer.
 * 
 *       * Redistributions in binary form must reproduce the above copyright
 *         notice, this list of conditions and the following disclaimer in
 *         the documentation and/or other materials provided with the
 *         distribution.
 * 
 *       * Neither the name of Google nor the names of its contributors may
 *         be used to endorse or promote products derived from this software
 *         without specific prior written permission.
 * 
 *     THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *     "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *     LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *     A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *     HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *     SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *     LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *     DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *     THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *     (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *     OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * * **zlib** (https://www.zlib.net/) mit folgender Lizenz:
 * 
 *     Copyright (C) 1995-2005 Jean-loup Gailly and Mark Adler
 * 
 *     This software is provided 'as-is', without any express or implied
 *     warranty.  In no event will the authors be held liable for any damages
 *     arising from the use of this software.
 * 
 *     Permission is granted to anyone to use this software for any purpose,
 *     including commercial applications, and to alter it and redistribute it
 *     freely, subject to the following restrictions:
 * 
 *     1. The origin of this software must not be misrepresented; you must not
 *        claim that you wrote the original software. If you use this software
 *        in a product, an acknowledgment in the product documentation would be
 *        appreciated but is not required.
 *     2. Altered source versions must be plainly marked as such, and must not be
 *        misrepresented as being the original software.
 *     3. This notice may not be removed or altered from any source distribution.
 * 
 *     Jean-loup Gailly        Mark Adler
 *     jloup@gzip.org          madler@alumni.caltech.edu
 * 
 *     The data format used by the zlib library is described by RFCs (Request for
 *     Comments) 1950 to 1952 in the files http://www.ietf.org/rfc/rfc1950.txt
 *     (zlib format), rfc1951.txt (deflate format) and rfc1952.txt (gzip format).
 * 
 * Das kompilierte Progamm ``testbld`` enthält darüberhinaus Teile folgender Softwarepakete:
 * 
 * * **openfst** (http://www.openfst.org/) und **opengrm** (http://www.opengrm.org/) mit folgender Lizenz:
 * 
 *     Licensed under the Apache License, Version 2.0 (the "License");
 *     you may not use these files except in compliance with the License.
 *     You may obtain a copy of the License at
 * 
 *       http://www.apache.org/licenses/LICENSE-2.0
 * 
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 * 
 *     Copyright 2005-2018 Google, Inc.
 * 
 * * **libyaml** (https://github.com/yaml/libyaml) mit folgender Lizenz:
 * 
 *     Copyright (c) 2017-2020 Ingy döt Net
 *     Copyright (c) 2006-2016 Kirill Simonov
 * 
 *     Permission is hereby granted, free of charge, to any person obtaining a copy of
 *     this software and associated documentation files (the "Software"), to deal in
 *     the Software without restriction, including without limitation the rights to
 *     use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 *     of the Software, and to permit persons to whom the Software is furnished to do
 *     so, subject to the following conditions:
 * 
 *     The above copyright notice and this permission notice shall be included in all
 *     copies or substantial portions of the Software.
 * 
 *     THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *     IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *     FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *     AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *     LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *     OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *     SOFTWARE.
 * 
 * * **kazlib** (https://www.kylheku.com/~kaz/kazlib.html) mit folgender Lizenz:
 * 
 *     Copyright (C) 1997 Kaz Kylheku <kaz@ashi.footprints.net>
 * 
 *     Free Software License:
 * 
 *     All rights are reserved by the author, with the following exceptions:
 *     Permission is granted to freely reproduce and distribute this software,
 *     possibly in exchange for a fee, provided that this copyright notice appears
 *     intact. Permission is also granted to adapt this software to produce
 *     derivative works, as long as the modified versions carry this copyright
 *     notice and additional notices stating that the work has been modified.
 *     This source code may be translated into executable form and incorporated
 *     into proprietary software; there is no requirement for such software to
 *     contain a copyright notice related to this source.
 * 
 * * **xpat** mit folgender Lizenz:
 * 
 *     Copyright (c) 1998, 1999, 2000 Thai Open Source Software Center Ltd and Clark Cooper
 *     Copyright (c) 2001, 2002, 2003, 2004, 2005, 2006 Expat maintainers.
 * 
 *     Permission is hereby granted, free of charge, to any person obtaining
 *     a copy of this software and associated documentation files (the
 *     "Software"), to deal in the Software without restriction, including
 *     without limitation the rights to use, copy, modify, merge, publish,
 *     distribute, sublicense, and/or sell copies of the Software, and to
 *     permit persons to whom the Software is furnished to do so, subject to
 *     the following conditions:
 * 
 *     The above copyright notice and this permission notice shall be included
 *     in all copies or substantial portions of the Software.
 * 
 *     THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *     EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *     MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *     IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 *     CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 *     TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *     SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 * * **CLAPACK** (https://netlib.org/clapack/) mit folgender Lizenz:
 * 
 *     CLAPACK is a freely-available software package.
 *     It is available from netlib via anonymous ftp and the World Wide Web.
 *     Thus, it can be included in commercial software packages (and has been).
 *     We only ask that proper credit be given to the authors.
 *     Namely, we ask that you cite the LAPACK Users' Guide, Third Edition.
 * 
 *     Like all software, it is copyrighted.
 *     It is not trademarked, but we do ask the following:
 * 
 *     If you modify the source for these routines we ask that you change the name
 *     of the routine and comment the changes made to the original.
 * 
 *     We will gladly answer any questions regarding the software.
 *     If a modification is done, however,
 *     it is the responsibility of the person who modified the routine to provide support.
 * 
 * * The Speech Signal Processing Toolkit (**SPKT**) (http://sp-tk.sourceforge.net/) mit folgender Lizenz:
 * 
 *      Copyright (c) 1984-2007  Tokyo Institute of Technology
 *                               Interdisciplinary Graduate School of
 *                               Science and Engineering
 * 
 *                    1996-2011  Nagoya Institute of Technology
 *                               Department of Computer Science
 * 
 *     All rights reserved.
 * 
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 * 
 *     - Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     - Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     - Neither the name of the SPTK working group nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 * 
 *     THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 *     CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 *     INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *     MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *     DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *     BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 *     EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 *     TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *     DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *     ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 *     OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 *     OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *     POSSIBILITY OF SUCH DAMAGE.
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
 *   newspk:  Start with new speaker (1) or continue with previous one (0)
 *   return:  success: 1; failure: 0 (use recikts_err())
 */
char recikts_restart(char newspk);


/* Callback function data */
struct recikts_callback_dat {
  char word[255];  /* Recognized word or token ("": no value) */
  uint32_t tstart; /* Begin in milli seconds */
  uint32_t tend;   /* End in milli seconds */
  float nld;       /* Likelihood of word (0..INF, greater value = uncertain result) */
  float sigmax;    /* Max. signal amplitude (-1.f: no value) */
  float trgmax;    /* Max. trigger value (-1.f: no value) */
  int8_t trgchg;   /* Trigger change (-1: no change, 0: off, 1: on) */
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

