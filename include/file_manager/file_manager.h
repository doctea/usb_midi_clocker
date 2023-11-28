#ifndef FILE_MANAGER__INCLUDED
#define FILE_MANAGER__INCLUDED

#include "storage.h"
#include "debug.h"

#include <LinkedList.h>

#include "file_manager/file_manager_interfaces.h"

bool load_file(char *filename, IParseKeyValueReceiver *target);
bool save_file(char *filename, ISaveKeyValueSource *source);

#endif
