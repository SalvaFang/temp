#ifndef GLOBALS_H
#define GLOBALS_H

#include <QMutex>



// Executable directory
extern QString gExeDir;

// Settings directory
extern QString gCfgDir;


// True when the gaze estimation is being calibrated -- e.g., to activate marker detection
extern bool gCalibrating;

// Freeze the previews, useful for debugging
extern bool gFreezePreview;

// Basically means: Should we consider gTimer for any of the processing?
extern bool gPostProcessing;

// Simple logging for easily gathering and debugging punctual data
void dbgLog(const QString& s);

#endif // GLOBALS_H
