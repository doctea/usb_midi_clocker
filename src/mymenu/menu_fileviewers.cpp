#include <Arduino.h>

#include "menu.h"

#include "menuitems.h"
#include "menuitems_fileviewer.h"
//#include "menu_sequence_fileviewer.h"

FileViewerMenuItem *sequence_fileviewer = nullptr;
FileViewerMenuItem *project_fileviewer = nullptr;

void update_sequence_filename(String filename) {
    if (sequence_fileviewer!=nullptr) {
        sequence_fileviewer->setFilename(filename);
        sequence_fileviewer->readFile();
    }
}

void update_project_filename(String filename) {
    if (project_fileviewer!=nullptr) {
        project_fileviewer->setFilename(filename);
        project_fileviewer->readFile();
    }
}