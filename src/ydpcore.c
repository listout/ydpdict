/*
  ydpdict
  (c) 1998-2003 wojtek kaniewski <wojtekka@irc.pl>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
                
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
                               
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <string.h>
#include <ctype.h>
#include "ydpcore.h"
#include "ydpconvert.h"
#include "config.h"

unsigned long fix32(unsigned long x)
{
#ifndef WORDS_BIGENDIAN
	return x;
#else
	return (unsigned long)
		(((x & (unsigned long) 0x000000ffU) << 24) |
                 ((x & (unsigned long) 0x0000ff00U) << 8) |
                 ((x & (unsigned long) 0x00ff0000U) >> 8) |
                 ((x & (unsigned long) 0xff000000U) >> 24));
#endif		
}

unsigned short fix16(unsigned short x)
{
#ifndef WORDS_BIGENDIAN
	return x;
#else
	return (unsigned short)
		(((x & (unsigned short) 0x00ffU) << 8) |
                 ((x & (unsigned short) 0xff00U) >> 8));
#endif
}

#define max(a,b) (a > b) ? a : b

/* otwiera s�ownik */
int opendict(char *path, char *index, char *def)
{
	int current = 0, bp = 0;
	unsigned long pos;
	char buf[256], ch, *p;
	
	ydperror = YDP_NONE;

	/* otw�rz plik indeksowy i plik z definicjami */
	if (!(p = (char*) malloc(max(strlen(index), strlen(def)) + strlen(path) + 2))) {
		ydperror = YDP_OUTOFMEMORY;
		return 0;
	}
	
	snprintf(p, sizeof(buf), "%s/%s", path, index);
	
	if ((fi = open(p, O_RDONLY)) == -1) {
		char *q;

		for (q = strrchr(p, '/'); *q; q++)
			*q = toupper(*q);

		if ((fi = open(p, O_RDONLY)) == -1) {
			free(p);
			ydperror = YDP_CANTOPENIDX;
			return 0;
		}
	}
	
	snprintf(p, sizeof(buf), "%s/%s", path, def);
	
	if ((fd = open(p, O_RDONLY)) == -1) {
		char *q;

		for (q = strrchr(p, '/'); *q; q++) 
			*q = toupper(*q);

		if ((fd = open(p, O_RDONLY)) == -1) {
			free(p);
			ydperror = YDP_CANTOPENDEF;
			return 0;
		}
	}

	free(p);

	/* wczytaj ilo�� s��w */
	lseek(fi, 0x08, SEEK_SET);
	wordcount = 0;
	read(fi, &wordcount, 2);
	wordcount = fix16(wordcount);
  
	/* zarezerwuj odpowiedni� ilo�� pami�ci */
	indexes = (unsigned long*) malloc(wordcount * sizeof(unsigned long));
	words = (char**) malloc((wordcount + 1) * sizeof(char*));
	if (!indexes || !words) {
		ydperror = YDP_OUTOFMEMORY;
		return 0;
	}
	words[wordcount] = 0;
    
	/* wczytaj offset tablicy indeks�w */
	lseek(fi, 0x10, SEEK_SET);
	pos = 0;
	read(fi, &pos, 4);
	pos = fix32(pos);
	lseek(fi, pos, SEEK_SET);

	/* wczytaj tablic� indeks�w */
	do {
		lseek(fi, 0x04, SEEK_CUR);
		indexes[current] = 0;
		read(fi, &indexes[current], sizeof(unsigned long));
		indexes[current] = fix32(indexes[current]);

		bp = 0;
		do {
			read(fi, &ch, 1);
			buf[(bp < 255) ? bp++ : bp] = ch;
		} while(ch);
		
		if (!(words[current] = strdup(buf))) {
			ydperror = YDP_OUTOFMEMORY;
			return 0;
		}
		
		convert_cp1250(words[current], 0);
	} while (++current < wordcount);

	return 1;
}

/* wczytuje definicj� */
char *readdef(int i)
{
	unsigned long dsize, size;
	unsigned char *def;
	
	/* za�aduj definicj� do bufora */
	lseek(fd, indexes[i], SEEK_SET);
	dsize = 0;
	read(fd, &dsize, sizeof(unsigned long));
	dsize = fix32(dsize);
	
	if (!(def = (char*) malloc(dsize + 1))) {
		ydperror = YDP_OUTOFMEMORY;
		return 0;
	}
	
	if ((size = read(fd, def, dsize)) != dsize) {
		free(def);
		ydperror = YDP_INVALIDFILE;
		return 0;
	}

	def[size] = 0;
	convert_cp1250(def, 0);

	ydperror = YDP_NONE;
	return def;
}

/* znajduje indeks s�owa zaczynaj�cego si� od podanego tekstu */
int findword(char *word)
{
	int x = 0;
	
	for (; x < wordcount; x++)
		if (!strncasecmp(words[x], word, strlen(word)))
			return x;
	
	return -1;
}

/* zamyka pliki i zwalnia pami�� */
void closedict()
{
	int x = 0;
  
	if (indexes)
		free(indexes);
	
	if (words) {
		while (words[x]) {
			free(words[x]);
			x++;
		}
		
		free(words);
	}
	
	close(fd);
	close(fi);
}

