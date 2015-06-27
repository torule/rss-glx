/*
 * stringify.c
 *
 * utility for embedding binary data in to appliactions via
 * strings (under gcc strings are in the rodata section, that is good)
 *
 * Angus Mackay
 *
 * say you have a couple hunks of 8 bit audio data you want to embed
 * in a program (if it wasn't 8 bit you'd have to worry about endianness)
 * all you have to do is create a C file with this utility:
 *   stringify sound1.wav sound2.wav > sounds_data.c
 *
 * then you could compile sounds_data.c.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#ifndef HAVE_BZLIB_H
#undef HAVE_LIBBZ2
#endif

#ifdef HAVE_LIBBZ2
#include <bzlib.h>
#define BZ2_LEVEL 9
#endif

/*
 * print a C compilable version of the data in this file
 * to stdout. might I remind you that the ANSI spec states
 * that adjacent string constants are to be concatenated, so if
 * you compiler is broken don't wine to me.
 */
void stringify (char *fname, int num)
{
	FILE *fp;
	unsigned char buf[BUFSIZ];
	unsigned int bread;
	unsigned int btot;
	unsigned int ctot;
	unsigned int i;
	unsigned int n;
	char *varname;

#ifdef HAVE_LIBBZ2
	FILE *fp_bz;
	BZFILE *bf;
	char *bfname;
	int bzerror;
#endif

	varname = fname + strlen (fname);
	while ((varname > fname) && (*varname != '/')) {
		varname--;
	};
	if (*varname == '/')
		varname++;

	if ((fp = fopen (fname, "rb")) == NULL) {
		perror (fname);
		return;
	}

	ctot = 0;
	btot = 0;
#ifdef HAVE_LIBBZ2
	bfname = (char *)malloc (strlen (fname) + 5);
	strcpy (bfname, fname);
	strcpy (bfname + strlen (fname), ".bz2");
	fp_bz = fopen (bfname, "wb");
	bf = BZ2_bzWriteOpen (&bzerror, fp_bz, BZ2_LEVEL, 0, 0);
	while ((bread = fread (buf, 1, BUFSIZ, fp)) > 0) {
		BZ2_bzWrite (&bzerror, bf, buf, bread);
	}
	BZ2_bzWriteClose (&bzerror, bf, 0, &btot, &ctot);
	fclose (fp_bz);
	fclose (fp);

	fname = bfname;
	if ((fp = fopen (fname, "rb")) == NULL) {
		perror (fname);
		return;
	}
#endif

	fprintf (stdout, "\n/* data from %s */\n", fname);
	fprintf (stdout, "const char *%s = \"\"\n", varname);

	n = 0;
	fprintf (stdout, "  \"");
	while ((bread = fread (buf, 1, BUFSIZ, fp)) > 0) {
#ifndef HAVE_LIBBZ2
		btot += bread;
#endif
		for (i = 0; i < bread; i++) {
			if ((buf[i] >= 32) && (buf[i] <= 126)) {
				if ((buf[i] == '\\') || (buf[i] == '"') || (buf[i] == '?')) {
					fprintf (stdout, "\\%c", buf[i]);
					n += 2;
				} else {
					fprintf (stdout, "%c", buf[i]);
					n += 1;
				}
			} else {
				fprintf (stdout, "\\%03o", buf[i]);
				n += 4;
			}

			if (n > 60) {
				fprintf (stdout, "\"\n");
				fprintf (stdout, "  \"");
				n = 0;
			}
		}
	}
	fprintf (stdout, "\";\n");

	fprintf (stdout, "unsigned int %s_size = %d;\n", varname, btot);
	fprintf (stdout, "unsigned int %s_compressedsize = %d;\n", varname, ctot ? ctot : btot);

	fclose (fp);

#ifdef HAVE_LIBBZ2
	remove (bfname);
#endif
}

int main (int argc, char **argv)
{
	int i;

	for (i = 1; i < argc; i++) {
		stringify (argv[i], i);
	}

	return 0;
}
