#ifndef __DESKBAR_CONTROL_H
#define __DESKBAR_CONTROL_H

#include <Messenger.h>
#include <View.h>

class BBitmap;
class DeskbarControlView : public BView {
public:
	DeskbarControlView(BRect rect, const char *name);
	DeskbarControlView(BMessage *data);
	
	virtual ~DeskbarControlView();
	
	static DeskbarControlView *Instantiate(BMessage *data);
	virtual status_t Archive(BMessage *data, bool deep) const;
	
	virtual void AttachedToWindow();
	virtual void DetachedFromWindow();
	
	virtual void Draw(BRect rect);
	virtual void MessageReceived(BMessage *message);

	virtual void MouseDown(BPoint where);
	
private:
	void InitData();
	
	BMessenger fControllerMessenger;
	BMessenger fAppMessenger;
	BBitmap *fBitmap;
	BBitmap *fRecordingBitmap;
	
	bool fRecording;
};


#endif 
