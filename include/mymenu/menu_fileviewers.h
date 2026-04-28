#pragma once

#include <Arduino.h>

//#include "menuitems.h"
//#include "menuitems_fileviewer.h"

class PageFileViewerMenuItem;

extern PageFileViewerMenuItem *sequence_fileviewer;
extern PageFileViewerMenuItem *project_fileviewer;
extern PageFileViewerMenuItem *system_settings_fileviewer;

void update_scene_filename(String filename);
void update_project_filename(String filename);
void update_system_settings_filename(String filename);
