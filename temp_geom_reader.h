#ifndef TEMP_GEOM_READER_PLUGIN_H
#define TEMP_GEOM_READER_PLUGIN_H

#include <clientserver/export.h>
#include <plugins/udaPlugin.h>

#ifdef __cplusplus
extern "C" {
#endif

#define THISPLUGIN_VERSION 1
#define THISPLUGIN_MAX_INTERFACE_VERSION 1 // Interface versions higher than this will not be understood!
#define THISPLUGIN_DEFAULT_METHOD "help"

LIBRARY_API [[maybe_unused]] int TempGeomReader(IDAM_PLUGIN_INTERFACE* idam_plugin_interface);

#ifdef __cplusplus
}
#endif

#endif // TEMP_GEOM_READER_PLUGIN_H
