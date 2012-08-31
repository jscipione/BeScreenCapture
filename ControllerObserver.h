#ifndef __CONTROLLEROBSERVER_H
#define __CONTROLLEROBSERVER_H

#include "Controller.h"


enum {
	kMsgControllerCaptureStarted = 5000,
	kMsgControllerCaptureFinished,
	kMsgControllerCapturePaused,
	kMsgControllerCaptureResumed,
	kMsgControllerEncodeStarted,
	kMsgControllerEncodeProgress,	// int32 num_files
	kMsgControllerEncodeFinished,	// status_t status
	kMsgControllerAreaSelectionChanged,
	kMsgControllerVideoDepthChanged,
	kMsgControllerOutputFileNameChanged
};


#endif // __CONTROLLEROBSERVER_H
