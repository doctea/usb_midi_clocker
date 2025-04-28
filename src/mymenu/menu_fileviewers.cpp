#ifdef ENABLE_SD

#include <Arduino.h>

#include "menu.h"

#include "menuitems.h"
#include "menuitems_fileviewer.h"
//#include "menu_sequence_fileviewer.h"
#include "menuitems_pageviewer.h"

PageFileViewerMenuItem *sequence_fileviewer = nullptr;
PageFileViewerMenuItem *project_fileviewer = nullptr;

void update_pattern_filename(String filename) {
    if (sequence_fileviewer!=nullptr) {
        sequence_fileviewer->readFile(filename.c_str());
    }
}

void update_project_filename(String filename) {
    if (project_fileviewer!=nullptr) {
        project_fileviewer->readFile(filename.c_str());
    }
}

#endif