#include "Controller.h"
#include "ControllerObserver.h"
#include "Executor.h"
#include "FunctionObject.h"
#include "MovieEncoder.h"
#include "Settings.h"
#include "messages.h"
#include "Utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Autolock.h>
#include <Bitmap.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <Screen.h>
#include <TranslationKit.h>
#include <TranslatorRoster.h>


static BTranslatorRoster* sTranslatorRoster = NULL;


BLooper *gControllerLooper = NULL;

Controller::Controller()
	:
	BLooper("Controller"),
	fCaptureThread(-1),
	fKillThread(true),
	fPaused(false)
{
	memset(&fDirectInfo, 0, sizeof(fDirectInfo));
	
	fEncoder = new MovieEncoder;
	if (sTranslatorRoster == NULL)
		sTranslatorRoster = BTranslatorRoster::Default();
		
	Run();
}


Controller::~Controller()
{
	delete fEncoder;
}


void
Controller::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case B_OBSERVER_NOTICE_CHANGE:
		{
			int32 what;
			message->FindInt32("be:observe_change_what", &what);
			switch (what) {
				case kMsgGUIToggleCapture:
					ToggleCapture();
					break;
				case kAreaSelected:
				{
					BRect rect;
					if (message->FindRect("selection", &rect) == B_OK) {
						FixRect(rect, true);
						SetCaptureArea(rect);
					}
					break;
				}
				default:
					break;
			}
			break;
		}
		
		case kEncodingFinished:
		{
			status_t error;
			message->FindInt32("status", (int32*)&error);
			_EncodingFinished(error);
			break;
		}
		
		case B_UPDATE_STATUS_BAR:
		case B_RESET_STATUS_BAR:
			SendNotices(message->what, message);
			break;
				
		default:	
			BLooper::MessageReceived(message);
			break;
	}
}


void
Controller::ToggleCapture()
{
	BAutolock _(this);
	if (fCaptureThread < 0)
		StartCapture();
	else
		EndCapture();
}


void
Controller::TogglePause()
{
	BAutolock _(this);
	if (fCaptureThread < 0)
		return;
		
	if (fPaused)
		_ResumeCapture();
	else
		_PauseCapture();
}


void
Controller::EncodeMovie()
{
	BAutolock _(this);
	
	SendNotices(kMsgControllerEncodeStarted);
	
	BList* fileList = new BList;
	
	int32 fileCount = BuildFileList(fTemporaryPath, *fileList);
	 
	BMessage message(kMsgControllerEncodeProgress);
	message.AddInt32("num_files", fileCount);
	
	SendNotices(kMsgControllerEncodeProgress, &message);
	
	fEncoder->SetSource(fileList);
	
	BMessenger messenger(this);
	fEncoder->SetMessenger(messenger);
	
	Settings settings;
	
	const char* fileName = NULL;
	settings.GetOutputFileName(&fileName);
			
	Executor* executor 
		= new Executor(NewMemberFunctionObjectWithResult
			<MovieEncoder, status_t>(&MovieEncoder::Encode, fEncoder));
	
	executor->RunThreaded();
}


void
Controller::SetUseDirectWindow(const bool& use)
{
	BAutolock _(this);
	Settings().SetUseDirectWindow(use);
}


void
Controller::SetCaptureArea(const BRect& rect)
{
	BAutolock _(this);
	Settings().SetCaptureArea(rect);
	
	fEncoder->SetDestFrame(rect);
	
	UpdateAreaDescription(rect);
}


void
Controller::SetVideoDepth(const color_space &space)
{
	BAutolock _(this);
	fEncoder->SetColorSpace(space);
	Settings().SetClipDepth(space);
}


void
Controller::SetOutputFileName(const char *name)
{
	BAutolock _(this);
	Settings().SetOutputFileName(name);
	fEncoder->SetOutputFile(name);
}


void
Controller::SetMediaFormatFamily(const media_format_family& family)
{
	BAutolock _(this);
	fEncoder->SetMediaFormatFamily(family);
}


void
Controller::SetMediaFormat(const media_format& format)
{
	BAutolock _(this);
	fEncoder->SetMediaFormat(format);
}


void
Controller::SetMediaCodecInfo(const media_codec_info& info)
{
	BAutolock _(this);
	fEncoder->SetMediaCodecInfo(info);
}


void
Controller::UpdateDirectInfo(direct_buffer_info* info)
{
	BAutolock _(this);
	fDirectInfo = *info;
	UpdateAreaDescription(Settings().CaptureArea());
}


status_t
Controller::ReadBitmap(BBitmap* bitmap, BRect bounds)
{
	void* from = (void*)((uint8*)fDirectInfo.bits + fAreaDesc.offset);    		
	void* to = bitmap->Bits();
	 
	const int32& bytesPerRow = bitmap->BytesPerRow();
	   		
	for (int32 y = 0; y < fAreaDesc.height; y++) {  
		memcpy(to, from, fAreaDesc.size);
	   	to = (void*)((uint8*)to + bytesPerRow);
		from = (void*)((uint8*)from + fDirectInfo.bytes_per_row);
	}
	 
	return B_OK;
}


void
Controller::UpdateAreaDescription(const BRect &rect)
{
	int32 bytesPerPixel = fDirectInfo.bits_per_pixel >> 3;
	if (bytesPerPixel <= 0)
		return;
		
	uint32 rowBytes = fDirectInfo.bytes_per_row / bytesPerPixel;	    		
		
	fAreaDesc.size = (rect.IntegerWidth() + 1) * bytesPerPixel;
	fAreaDesc.height = rect.IntegerHeight() + 1;
	fAreaDesc.offset = ((uint32)rect.left +
		 ((uint32)rect.top * rowBytes)) * bytesPerPixel;
}


void
Controller::StartCapture()
{
	BPath path;
	status_t status = find_directory(B_COMMON_TEMP_DIRECTORY, &path);
	if (status != B_OK)
		return;
			
	// Create temporary path
	fTemporaryPath = tempnam((char *)path.Path(), (char *)"_BSC");
	if (create_directory(fTemporaryPath, 0777) < B_OK) {
		printf("Unable to create temporary folder");
		return;
	}
		
	fKillThread = false;
	fPaused = false;
	
	fCaptureThread = spawn_thread((thread_entry)CaptureStarter,
		"Capture Thread", B_DISPLAY_PRIORITY, this);
					
	if (fCaptureThread < 0)
		return;
		
	status = resume_thread(fCaptureThread);
	if (status < B_OK) {
		kill_thread(fCaptureThread);
		return;
	}

	SendNotices(kMsgControllerCaptureStarted);
}


void
Controller::EndCapture()
{
	BAutolock _(this);
	printf("Controller::EndCapture()\n");
	if (fCaptureThread > 0) {
		fPaused = false;
		fKillThread = true;
		status_t unused;
		wait_for_thread(fCaptureThread, &unused);
	}
	
	SendNotices(kMsgControllerCaptureFinished);
	
	printf("Encoding movie...\n");
	EncodeMovie();
	printf("done!\n");
}


void
Controller::_PauseCapture()
{
	BAutolock _(this);
	fPaused = true;
	suspend_thread(fCaptureThread);
}


void
Controller::_ResumeCapture()
{
	BAutolock _(this);
	resume_thread(fCaptureThread);
	fPaused = false;
}


void
Controller::_EncodingFinished(const status_t status)
{
	BEntry(fTemporaryPath).Remove();
	free(fTemporaryPath);
	fTemporaryPath = NULL;

	BMessage message(kMsgControllerEncodeFinished);
	message.AddInt32("status", (int32)status);
	SendNotices(kMsgControllerEncodeFinished, &message);
}


int32
Controller::CaptureThread()
{
	const bool &useDirectWindow = Settings().UseDirectWindow();
	BScreen screen;
	BRect bounds = Settings().CaptureArea();
	
	//bigtime_t lastFrameTime = 0;
	bigtime_t waitTime = 1000000 / /*fCaptureOptions.framesPerSecond*/10;
	BFile outFile;
	BBitmap *bitmap = new BBitmap(bounds, screen.ColorSpace());
	
	bool firstFrame = true;
	translator_info translatorInfo;
	
	//fStartTime = system_time();
	
	int32 bitmapCount = 0;
	char string[B_FILE_NAME_LENGTH];
	status_t error = B_ERROR;
	while (!fKillThread) {
		if (!fPaused) {
			screen.WaitForRetrace(waitTime); // Wait for Vsync
		
			if (useDirectWindow)		
				error = ReadBitmap(bitmap, bounds);			
			else 
				error = screen.ReadBitmap(bitmap, false, &bounds);
					 
	    	if (error == B_OK) {
	    		/*if (fCaptureOptions.includeCursor) {    			
	    			
				}*/
					
				//Save bitmap to disk
				BBitmapStream bitmapStream(bitmap);
				if (firstFrame) {
					firstFrame = false;
					sTranslatorRoster->Identify(&bitmapStream, NULL,
						&translatorInfo, 0, NULL, 'BMP ');
				}
				
				snprintf(string, B_FILE_NAME_LENGTH, "%s/frame %5ld",
					fTemporaryPath, bitmapCount);
				outFile.SetTo(string, B_WRITE_ONLY|B_CREATE_FILE);
				error = sTranslatorRoster->Translate(&bitmapStream,
					&translatorInfo, NULL, &outFile, 'BMP ');	
				bitmapCount++;
				
				// Cleanup
				bitmapStream.DetachBitmap(&bitmap);	
				outFile.Unset();
				
			} else {
				//PRINT(("Error while getting screenshot: %s\n", strerror(error)));
				break;
			}
		} else
			snooze(500000);
	}
	
	//fEndTime = system_time();
	
	delete bitmap;
	
	fCaptureThread = -1;
		
	return B_OK;
}


/* static */
int32
Controller::CaptureStarter(void *arg)
{
	return static_cast<Controller *>(arg)->CaptureThread();
}
