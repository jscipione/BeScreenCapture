#include "Utils.h"

#include <Directory.h>
#include <Entry.h>
#include <List.h>
#include <Path.h>
#include <String.h>

#include <cstring>
#include <cstdio>

int32
BuildFileList(const BPath &path, BList &list)
{
	BDirectory directory(path.Path());
	BEntry entry;
	BString string;
	char entryName[B_FILE_NAME_LENGTH];
	while (directory.GetNextEntry(&entry) == B_OK) {
		if (entry.GetName(entryName) == B_OK) {
			string.SetTo(path.Path());
			string << "/" << entryName;
			list.AddItem(strdup(string.String()));
		}
	}
	
	return list.CountItems();
}


void
MakeUniqueName(const char *name, char *newName, size_t length)
{
	int32 suffix = 1;
	BEntry entry(name);
	snprintf(newName, length, "%s", name);
	while (entry.Exists()) {
		snprintf(newName, length, "%s %ld", name, suffix);
		entry.SetTo(newName);
		suffix++;
	}
}


BRect
GetScaledRect(const BRect &rect, const float scale)
{
	BRect scaledRect = rect;
	scaledRect.OffsetTo(B_ORIGIN);
	
	scaledRect.right = ceil((scaledRect.right + 1) * scale / 100 - 1);
	scaledRect.bottom = ceil((scaledRect.bottom + 1) * scale / 100 - 1);
	
	return scaledRect;
}


void
FixRect(BRect &rect, const bool fixWidth, const bool fixHeight)
{
	// Adjust width and/or height to be a multiple of 16
	// as some codecs create bad movies otherwise
	int32 diffHorizontal = 16 - (rect.IntegerWidth() + 1) % 16;
	if (fixWidth && diffHorizontal != 16) { 
		if (rect.left < diffHorizontal) {
			diffHorizontal -= (int32)rect.left;
			rect.left = 0;
		} else {
			rect.left -= diffHorizontal;
			diffHorizontal = 0;
		}	
			
		rect.right += diffHorizontal;
	}
	
	int32 diffVertical = 16 - (rect.IntegerHeight() + 1) % 16;
	if (fixHeight && diffVertical != 16) { 
		if (rect.top < diffVertical) {
			diffVertical -= (int32)rect.top;
			rect.top = 0;
		} else {
			rect.top -= diffVertical;
			diffVertical = 0;
		}	
			
		rect.bottom += diffVertical;
	}
}


bool
GetMediaFileFormat(const media_format_family &family,
	media_file_format &format)
{
	int32 cookie = 0;
	while (get_next_file_format(&cookie, &format) == B_OK) {
		if (format.family == family)
			return true;
	}
	
	memset(&format, 0, sizeof(format));
	
	return false;
}
