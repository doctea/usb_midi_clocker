#pragma once

#include <Arduino.h>

//#include "menuitems.h"
//#include "menuitems_fileviewer.h"

class PageFileViewerMenuItem;

extern PageFileViewerMenuItem *sequence_fileviewer;
extern PageFileViewerMenuItem *project_fileviewer;

void update_pattern_filename(String filename);
void update_project_filename(String filename);
