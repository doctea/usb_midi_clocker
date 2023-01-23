#ifndef MENU_SEQUENCE_FILEVIEWER__INCLUDEDf
#define MENU_SEQUENCE_FILEVIEWER__INCLUDED

#include <Arduino.h>

//#include "menuitems.h"
//#include "menuitems_fileviewer.h"

class FileViewerMenuItem;

extern FileViewerMenuItem *sequence_fileviewer;
extern FileViewerMenuItem *project_fileviewer;

void update_sequence_filename(String filename);
void update_project_filename(String filename);

#endif