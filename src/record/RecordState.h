#pragma once

#include <atomic>

enum RecordStage
{
	Stopped,
	Recording,
	Paused,
	Writing // Only used after recording ended, use .writing for when .writeThread is enabled
};

enum RecordFormat
{
	Gif,
	WebP,
	Old
};

enum RecordBuffer
{
	Off,
	Ram,
	Disk
};

struct RecordState
{
	bool CanStopStart();
	bool IsActive();
	bool CanEdit();
	int BufferSize();

	void TogglePause();
	void CheckBounds(bool inclusive = false);

	void ClearCounters();
	void Clear();

	RecordState();

	// Current Stage
	RecordStage stage;
	bool halt; // Alternate pause
	bool writing; // Writing happens independent of recording stage

	// Settings
	RecordFormat format;
	RecordBuffer buffer;
	int bufferLimit;
	bool writeThread;
	int quality;
	int fps;
	int x1;
	int y1;
	int x2;
	int y2;
	int scale;
	bool spacing;
	bool includeUI;

	// Recording State
	float delay; // Calculated from FPS, delay value for format (Gif uses 1/100 sec)
	std::atomic<int> frame; // Current frame index being processed
	std::atomic<int> nextFrame; // Next frame index to be recorded, not cleared for buffer writing

	// UI State
	int file;
	int select;
	int ratio; // Calculated from FPS, frame skip ratio
	int ratioFrame; // Used when skipping frames
};
