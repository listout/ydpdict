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
#include <getopt.h>
#include "config.h"

#include "ydpconfig.h"
#include "ydpconvert.h"

#ifdef HAVE_GETOPT_LONG
/* informacje dla getopt */
static struct option const longopts[] = {
  { "help", no_argument, 0, 'h' },
  { "version", no_argument, 0, 'v' },
  { "pol", no_argument, 0, 'p' },
  { "ang", no_argument, 0, 'a' },
  { "path", required_argument, 0, 't' },
  { "cdpath", required_argument, 0, 'c' },
  { "nopl", no_argument, 0, 'n' },
  { "iso", no_argument, 0, 'i' },
  { "unicode", no_argument, 0, 'u' },
  { "unicodeset", no_argument, 0, 'U' },
  { "player", required_argument, 0, 'P' },
  { 0, 0, 0, 0 }
};
#endif /* HAVE_GETOPT_LONG */

/* instrukcja u�ycia */
void usage(char *argv0) {
  printf("\
U�ycie: %s [OPCJE]\n\
  -a, --ang             uruchamia s�ownik angielski-polski (domy�lne)\n\
  -p, --pol             uruchamia s�ownik polsko-angielski\n\
  -n, --nopl            wy��cza wy�wietlanie polskich liter\n\
  -i, --iso		wy�wietla polskie literki w standardzie ISO-8859-2\n\
  -u, --unicode		wy�wietla polskie literki u�ywaj�c unikodu\n\
  -U, --unicodeset	prze��cza konsol� w tryb unikodu na czas dzia�ania\n\
  -f, --path=�CIE�KA    podaje �cie�k� do plik�w danych\n\
  -c, --cdpath=�CIE�KA  podaje �cie�k� do p�yty CD\n\
  -P, --player=�CIE�KA  podaje �cie�k� do odtwarzacza plik�w WAV\n\
      --version		wy�wietla wersj� programu\n\
  -h, --help		wy�wietla ten tekst\n\
\n", argv0);
}

/* jakie�tam zmienne */
char *e_labels[] = E_LABELS;
int *e_vals[] = E_VALS;
char *color_defs[] = COLOR_DEFS;
int color_vals[] = COLOR_VALS;

/* wczytuje konfiguracj� z pliku */
int read_config(int argc, char **argv) {
  char linia[256], *par;
  int optc, l = 0, x, y;
  FILE *f;

  /* warto�ci pocz�tkowe */
  filespath = "./";
  use_color = 1;
  charset = 1;
  dict_ap = 1;
  config_text = COLOR_WHITE;
  config_cf1 = COLOR_CYAN | A_BOLD;
  config_cf2 = COLOR_GREEN | A_BOLD;

  /* sprawd� czy plik istnieje */
  f = fopen(CONFIGFILE_CWD1, "r");
  if (!f) f = fopen(CONFIGFILE_CWD2, "r");
  snprintf(linia, 256, "%s/%s", getenv("HOME"), CONFIGFILE_CWD1);
  if (!f) f = fopen(linia, "r");
  snprintf(linia, 256, "%s/%s", getenv("HOME"), CONFIGFILE_CWD2);
  if (!f) f = fopen(linia, "r");
  if (!f) f = fopen(CONFIGFILE_GLOBAL, "r");
  if (!f) return -1;
  
  /* ka�d� lini� z osobna */
  while(fgets(linia, 255, f)) {
    /* obr�b lini� tak, �eby�my nie dostawali �mieci */
    if(linia[strlen(linia) - 1] == '\n') linia[strlen(linia) - 1] = '\0';
    if(linia[0] == '#' || linia[0] == '\0') continue;
    l++;

    /* sprawd� czy co� pasuje do zadeklarowanych w�a�ciwo�ci */
    for(x = 0; e_labels[x]; x++) {
      if(strncasecmp(linia, &e_labels[x][2], strlen(e_labels[x]) - 2)) continue;
      par = &linia[strlen(&e_labels[x][2]) + 1];
      switch(e_labels[x][0]) {
        /* jaki� �adny kolorek */
        case 'c':
          for(y = 0; color_defs[y]; y++)
            if(!strcasecmp(par, color_defs[y])) *e_vals[x] = color_vals[y];
	  break;
	/* warto�� bool'owska: on lub off */
	case 'b':
	  if(!strncasecmp(par, "On", 2)) *e_vals[x] = TRUE;
	  if(!strncasecmp(par, "Of", 2)) *e_vals[x] = FALSE;
	  break;
	/* warto�� ci�gu */
	case 's':
	  *(char**)e_vals[x] = strdup(par);
	  break;
        /* zestaw znak�w */
	case 'h':
	  if(!strncasecmp(par, "No", 2)) charset = 0; /* bez polskich */
	  if(!strncasecmp(par, "ISO", 3)) charset = 1; /* iso-8859-2 */
	  if(!strcasecmp(par, "Unicode")) charset = 2; /* unikod */
	  if(!strcasecmp(par, "UnicodeSet")) charset = 3; /* unikod-2 */
	  break;
      }
      break;
    }

    if(!e_labels[x]) {
      fprintf(stderr, "B��D: plik konfiguracyjny, linia %d: %s\n", l, linia);
      exit(1);
    }
  }

  fclose(f);
  
#ifdef HAVE_GETOPT_LONG
  while ((optc = getopt_long(argc, argv, "hvVpaf:c:niuU", longopts, (int*) 0)) != EOF) {
#else
  while ((optc = getopt(argc, argv, "hvVpaf:c:niuU")) != EOF) {
#endif
    switch(optc) {
      case 'h':
        usage(argv[0]);
        exit(0);
      case 'v':
      case 'V':
        printf("ydpdict-" VERSION "\n");
	exit(0);
      case 'p':
        dict_ap = 0;
	break;
      case 'a':
        dict_ap = 1;
	break;
      case 'n':
        charset = 0;
	break;
      case 'i':
        charset = 1;
	break;
      case 'u':
        charset = 2;
	break;
      case 'U':
        charset = 3;
	break;
      case 'f':
        filespath = strdup(optarg);
	break;
      case 'c':
        cdpath = strdup(optarg);
	break;
      case 'P':
        player = strdup(optarg);
	break;
      default:
        usage(argv[0]);
	exit(1);
    }
  }
  
  return 0;
}
