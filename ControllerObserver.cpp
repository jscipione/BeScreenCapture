#include "ControllerObserver.h"

#include <Handler.h>
#include <Looper.h>
#include <Message.h>
#include <Messenger.h>

#include <stdio.h>


ControllerObserver::ControllerObserver(BHandler *target)
	:
	fHandler(target),
	fMessenger(NULL)
{
}


ControllerObserver::ControllerObserver(const BMessenger &messenger)
	:
	fHandler(NULL),
	fMessenger(new BMessenger(messenger))
{
}


ControllerObserver::~ControllerObserver()
{
	delete fMessenger;
}


void
ControllerObserver::CaptureStarted()
{
	BMessage message(kMsgControllerCaptureStarted);
	_SendMessage(&message);
}


void
ControllerObserver::CaptureFinished()
{
	BMessage message(kMsgControllerCaptureFinished);
	_SendMessage(&message);
}


void
ControllerObserver::CapturePaused()
{
	BMessage message(kMsgControllerCapturePaused);
	_SendMessage(&message);
}


void
ControllerObserver::CaptureResumed()
{
	BMessage message(kMsgControllerCaptureResumed);
	_SendMessage(&message);
}


void
ControllerObserver::EncodeStarted(const int32 numFiles)
{
	BMessage message(kMsgControllerEncodeStarted);
	message.AddInt32("num_files", numFiles);
	_SendMessage(&message);
}


void
ControllerObserver::EncodeFinished(const status_t status)
{
	BMessage message(kMsgControllerEncodeFinished);
	message.AddInt32("status", status);
	_SendMessage(&message);
}


void
ControllerObserver::AreaSelectionChanged()
{
	BMessage message(kMsgControllerAreaSelectionChanged);
	_SendMessage(&message);
}


void
ControllerObserver::VideoDepthChanged()
{
	BMessage message(kMsgControllerVideoDepthChanged);
	_SendMessage(&message);
}


void
ControllerObserver::OutputFileNameChanged()
{
	BMessage message(kMsgControllerOutputFileNameChanged);
	_SendMessage(&message);
}


void
ControllerObserver::ForwardMessage(BMessage *message)
{
	_SendMessage(message);
}


void
ControllerObserver::_SendMessage(BMessage *message)
{
	if (fHandler) {
		fHandler->Looper()->PostMessage(message, fHandler);
	} else
		fMessenger->SendMessage(message);
}
