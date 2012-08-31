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


#include "ControllerObserver.h"

#include <Handler.h>
#include <Looper.h>
#include <Message.h>
#include <Messenger.h>

#include <stdio.h>


class ControllerObserver {
 public:
							ControllerObserver(BHandler *target);
							ControllerObserver(const BMessenger &messenger);
	virtual					~ControllerObserver();

			void			CaptureStarted();
			void			CaptureFinished();
			void			CapturePaused();
			void			CaptureResumed();
			void			EncodeStarted(const int32 numFiles);
			void			EncodeFinished(const status_t status);
			void			AreaSelectionChanged();
			void			VideoDepthChanged();
			void			OutputFileNameChanged();
			void			ForwardMessage(BMessage *message);

 private:
			void			_SendMessage(BMessage *message);

			BHandler*		fHandler;
			BMessenger*		fMessenger;
};

#endif // __CONTROLLEROBSERVER_H
