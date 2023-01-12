/*********************************************************************
*                     SEGGER Microcontroller GmbH                    *
*                        The Embedded Experts                        *
**********************************************************************
*                                                                    *
*       (c) 2003 - 2022  SEGGER Microcontroller GmbH                 *
*                                                                    *
*       www.segger.com     Support: support@segger.com               *
*                                                                    *
**********************************************************************
----------------------------------------------------------------------
File        : Bin2C.c
Purpose     : This program will transform a binary http file into a C 
              file suitable for compiling.
---------------------------END-OF-HEADER------------------------------
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>

typedef int  BOOLEAN; 
#define TRUE  1
#define FALSE 0

char outcname[256];
char outhname[256];
char outfname[256];
char outpname[256];

/*********************************************************************
*
*       _ReportOut()
*
*  Function description
*    Terminal output
*
**********************************************************************
*/
static void _ReportOut(const char* sOut) {
  printf(sOut);
}

/*********************************************************************
*
*       _ParsePath
*/
static void _ParsePath(char * topath, char * filename, char * path) {
  char * pfile;    
  char * pto;    
  char * pfr;    
  char * pslash;    
  BOOLEAN colon;

  pto    = topath;    
  pfr    = path;    
  pslash = NULL;
  if (path[0] && path[1] == ':') {
    colon = TRUE;
  } else {
    colon = FALSE;
  }
  //
  // Copy the whole path to topath remembering where the last backslash is
  //
  *pto = 0;
  while (*pfr) {
    *pto = *pfr++;
    if (*pto == '\\') {
      pslash = pto;
    }
    pto++; 
  }
  *pto = '\0';
  //
  // Figure out where the file name starts
  //
  if (pslash) {
      pfile = pslash+1;
  } else {
    if (colon) {
      pfile = topath + 2;
    } else {
      pfile = topath;
    }
  }
  //
  // Copy and filter file name
  //
  while (*pfile) {
    if ((*pfile > 0x40 && *pfile < 0x5B) ||
      (*pfile > 0x60 && *pfile <= 0x7B) ||
      (*pfile > 0x2F && *pfile <= 0x3A)) {
      *(filename++) = *(pfile++);
    } else {
      *(filename++) = '_';
      pfile++;
    }
  }
  *filename = *pfile;
  if (!pslash) {
    if (colon) {
      *(topath+2) = '\0';
    } else {
      *topath = '\0';
    }
  } else {
    if ( (colon && (pslash == topath + 2)) || (!colon && (pslash == topath)) ) {
      *(pslash+1) = '\0';
    } else {
      *pslash = '\0';
    }
  }
}

/*********************************************************************
*
*       _CompareFilenameExt
*
*  Function description
*    Checks if the given filename has the given extension
*    The test is case-sensitive, meaning:
*    _CompareFilenameExt("Index.html", ".html")           ---> Match
*    _CompareFilenameExt("Index.htm",  ".html")           ---> Mismatch
*    _CompareFilenameExt("Index.HTML", ".html")           ---> Mismatch
*    _CompareFilenameExt("Index.html", ".HTML")           ---> Mismatch
*
*  Parameters
*    sFilename     Null-terminated filename, such as "Index.html"
*    sExtension    Null-terminated filename extension with dot, such as ".html"
*
*  Return value
*     0   match
*  != 0   mismatch
*/
static char _CompareFilenameExt(const char * sFilename, const char * sExt) {
  int LenFilename;
  int LenExt;
  int i;
  char c0;
  char c1;

  LenFilename = strlen(sFilename);
  LenExt = strlen(sExt);
  if (LenFilename < LenExt) {
    return 0;                     // mismatch
  }
  for (i = 0; i < LenExt; i++) {
    c0 = *(sFilename + LenFilename -i -1);
    c1 = *(sExt + LenExt -i -1);
    if (c0 != c1) {
      return 0;                   // mismatch
    }
  }
  return 1;
}

/*********************************************************************
*
*       _IsHTML
*/
static int _IsHTML(const char * Filename) {
  int r; 
  r = _CompareFilenameExt(Filename, ".htm");
  if (r) {
    return r;
  } 
  r = _CompareFilenameExt(Filename, ".html");
  return r;
}


/*********************************************************************
*
*       _Add2File
*/
static int _Add2File(FILE * pFile, int Element, int Cnt) {
  static int ColumnCnt;

  if (ColumnCnt < 9) {
    fprintf(pFile, " 0x%02x, ", Element);
    ColumnCnt++;
  } else {
    fprintf(pFile, " 0x%02x,\n", Element);
    ColumnCnt = 0;
  }
  Cnt++;
  return Cnt;
}

/*********************************************************************
*
*       _WriteCountCFile
*/
static int _WriteCountCFile(FILE *infile, FILE *outc, int Compress, char JustCount) {
  long Count    = 0;
  int  Previous = 0;
  int  InHTML   = 0;
  int  c; 
  
  fseek(infile, 0, SEEK_SET);  // Goto start of file.
  //
  // Write c file, data (or just count)
  //
  while ((c = fgetc(infile)) != EOF) {
    //
    // Remove spaces used for the layout of the html page
    //
    if ((c == ' ') && (Compress == 1) && (InHTML == 0)) {
      continue;
    } 
    //
    // If first character of html source code line 
    //
    if (Compress == 1) {
      InHTML = 1;
    }
    //
    // Remove carriage return, line feed if placed on end of a HTML source code line.
    //
    if (InHTML == 1) {
      if (c == '\r') {
        if (Previous == '>') {
          c = fgetc(infile);
          if (c != '\n') {
            if (JustCount) {
              Count++;
            } else {
              Count = _Add2File(outc, c, Count);
            }
          }
          Previous = 0;
          InHTML = 0;
          continue;
        }
      }
    }
    if (JustCount) {
      Count++;
    } else {
      Count  = _Add2File(outc, c, Count);
    }
    Previous = c;
  }
  if (JustCount == 0) {
    fprintf(outc, "};\n");
  }
  return Count;
}

/********************************************************************* 
*
*       _WaitKey()
*
*  Function description
*    Prints "Press any key to exit" and waits for userinput
*/
static void _WaitKey(void) {
  _ReportOut("Press any key to exit...");
  _getch();
}

/********************************************************************* 
*
*       _PrintUsage()
*
*  Function description
*    Prints the utility's usage information.
*/
static void _PrintUsage(void) {
  printf("\n\nBin2C.exe (c) 2002 - 2015 SEGGER Microcontroller --- www.segger.com\n");
  printf("Usage:\n");
  printf("Bin2C <infile> <outfile>\n");
  printf("where <infile>  is the input binary (or text) file (with extension) and\n");
  printf("      <outfile> is the name (without extension) of the .c and .h files to create.\n");
  printf("Example:\n");
  printf("Bin2C index.html index\n");
}

/*********************************************************************
*
*       main
*/
void main(int argc, char *argv[]) {
  FILE *infile;
  FILE *outc; 
  FILE *outh;
  int   Previous; 
  int   InHTML;  
  int   Compress;
  long  Count;
  

  Count     = 0;
  Compress  = -1;
  Previous  = 0;
  InHTML = 0;
  if (argc < 3) {
    _PrintUsage();
    _WaitKey();
    exit(0);
  }
  //
  // Open input file
  //
  infile = fopen(argv[1], "rb");
  if (infile == NULL) {
    perror(argv[1]);
    exit(-1);
  }
  //
  // Isolate the file name to use as the array name in the .c file
  //
  _ParsePath(outpname, outfname, argv[2]);
  strcpy(outcname, argv[2]);
  strcat(outcname, ".c");
  outc = fopen(outcname, "wt");
  if (outc == NULL) {
    perror(outcname);
    fclose(infile);
    exit(-1);
  }
  strcpy(outhname, argv[2]);
  strcat(outhname, ".h");
  outh = fopen(outhname, "wt");
  if (outh == NULL) {
    perror(outhname);
    fclose(outc);
    fclose(infile);
    exit(-1);
  }
  //
  // Check if compression is set and check if file is compressible
  //
  if (argv[3] != NULL) {
    if (strcmp(argv[3], "--compress") == 0) {
      Compress = _IsHTML(argv[1]); // Only .html and .htm files are compressible
    }
  }
  //
  // Count data written to c file.
  //
  Count = _WriteCountCFile(infile, outc, Compress, 1);
  //
  // Write c file, header
  //
  fprintf(outc, "/*********************************************************************\n");
  fprintf(outc, "*                    SEGGER Microcontroller GmbH                     *\n");
  fprintf(outc, "*       Solutions for real time microcontroller applications         *\n");
  fprintf(outc, "**********************************************************************\n");
  fprintf(outc, "*                                                                    *\n");
  fprintf(outc, "*            (c) 1995 - 2018 SEGGER Microcontroller GmbH             *\n");
  fprintf(outc, "*                                                                    *\n");
  fprintf(outc, "*       www.segger.com     Support: support@segger.com               *\n");
  fprintf(outc, "*                                                                    *\n");
  fprintf(outc, "**********************************************************************\n");
  fprintf(outc, " \n");
  fprintf(outc, "----------------------------------------------------------------------\n");
  fprintf(outc, "File    : %s\n", outcname);
  fprintf(outc, "Purpose : Automatically created from %s using Bin2C.exe\n", argv[1]);
  fprintf(outc, "--------  END-OF-HEADER  ---------------------------------------------\n");
  fprintf(outc, "*/\n");
  fprintf(outc, "\n");
  fprintf(outc, "#include \"%s\"\n", outhname);
  fprintf(outc, "\n");
  fprintf(outc, "const unsigned char %s_file[%ld] =\n{\n", strlwr(outfname), Count);
  //
  // Write c file, data
  //
  _WriteCountCFile(infile, outc, Compress, 0);
  //
  // Write c file, footer
  //
  fprintf(outc, "\n");
  fprintf(outc, "/****** End Of File *************************************************/\n");
  //
  // Write header file
  //
  fprintf(outh, "/*********************************************************************\n");
  fprintf(outh, "*                    SEGGER Microcontroller GmbH                     *\n");
  fprintf(outh, "*       Solutions for real time microcontroller applications         *\n");
  fprintf(outh, "**********************************************************************\n");
  fprintf(outh, "*                                                                    *\n");
  fprintf(outh, "*            (c) 1995 - 2018 SEGGER Microcontroller GmbH             *\n");
  fprintf(outh, "*                                                                    *\n");
  fprintf(outh, "*       www.segger.com     Support: support@segger.com               *\n");
  fprintf(outh, "*                                                                    *\n");
  fprintf(outh, "**********************************************************************\n");
  fprintf(outh, " \n");
  fprintf(outh, "----------------------------------------------------------------------\n");
  fprintf(outh, "File    : %s\n", outhname);
  fprintf(outh, "Purpose : Automatically created from %s using Bin2C.exe\n", argv[1]);
  fprintf(outh, "--------  END-OF-HEADER  ---------------------------------------------\n");
  fprintf(outh, "*/\n");
  fprintf(outh, "\n");
  fprintf(outh, "#ifndef __%s_H__\n", strupr(outfname));
  fprintf(outh, "#define __%s_H__\n", strupr(outfname));
  fprintf(outh, "\n");
  fprintf(outh, "#ifdef __cplusplus\n");
  fprintf(outh, "  extern \"C\" {\n");
  fprintf(outh, "#endif\n");
  fprintf(outh, "#define %s_SIZE %ld\n", strupr(outfname), Count);
  fprintf(outh, "\n");
  fprintf(outh, "extern const unsigned char %s_file[%ld];\n", strlwr(outfname), Count);
  fprintf(outh, "\n");
  fprintf(outh, "#ifdef __cplusplus\n");
  fprintf(outh, "  };\n");
  fprintf(outh, "#endif\n");
  fprintf(outh, "#endif  //__%s_H__\n", strupr(outfname));
  fprintf(outh, "\n");
  fprintf(outh, "/****** End Of File *************************************************/\n");
  //
  // Cleanup: close files
  //
  fclose(infile);
  fclose(outc);
  fclose(outh);
}


