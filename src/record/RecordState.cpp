#include "RecordState.h"
#include "Config.h"

#include <cmath>
#include <algorithm>

bool RecordState::CanStopStart()
{
	return stage != RecordStage::Writing && !writing;
}

bool RecordState::IsActive()
{
	return stage == RecordStage::Recording || stage == RecordStage::Paused;
}

bool RecordState::CanEdit()
{
	return stage == RecordStage::Stopped;
}

int RecordState::BufferSize()
{
	return ((x2 - x1) * (y2 - y1) * 4 * (nextFrame - frame + 1)) / 1048576;
}

void RecordState::TogglePause()
{
	if (stage == RecordStage::Paused)
	{
		stage = RecordStage::Recording;
	}
	else if (stage == RecordStage::Recording)
	{
		stage = RecordStage::Paused;
	}
}

void RecordState::CheckBounds(bool inclusive)
{
	int ox1 = x1 == -1 ? 0 : x1;
	int oy1 = y1 == -1 ? 0 : y1;
	int ox2 = x2 == -1 ? WINDOWW : x2;
	int oy2 = y2 == -1 ? WINDOWH : y2;
	x1 = std::clamp((ox2 > ox1 ? ox1 : ox2), 0, WINDOWW - 2);
	x2 = std::clamp((ox2 > ox1 ? ox2 : ox1) + (inclusive ? 1 : 0), x1 + 1, WINDOWW - 1);
	y1 = std::clamp((oy2 > oy1 ? oy1 : oy2), 0, WINDOWH - 2);
	y2 = std::clamp((oy2 > oy1 ? oy2 : oy1) + (inclusive ? 1 : 0), y1 + 1, WINDOWH - 1);
}

void RecordState::ClearCounters()
{
	frame = 0;
	nextFrame = 0;
	ratioFrame = 0;
}

void RecordState::Clear()
{
	format = RecordFormat::Gif;
	buffer = RecordBuffer::Ram;
	bufferLimit = 0;
	writeThread = true;
	quality = 7;
	fps = 60;
	x1 = 0;
	y1 = 0;
	x2 = XRES;
	y2 = YRES;
	scale = 1;
	spacing = false;
	includeUI = false;
	ClearCounters();
}

RecordState::RecordState()
{
	stage = RecordStage::Stopped;
	halt = false;
	writing = false;
	file = 0;
	select = 0;
	Clear();
}
