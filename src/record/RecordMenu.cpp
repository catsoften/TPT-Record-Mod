#include "RecordMenu.h"

#include "RecordController.h"

#include "common/String.h"
#include "common/Platform.h"
#include "graphics/Graphics.h"
#include "gui/Style.h"
#include "gui/interface/Colour.h"
#include "gui/interface/Appearance.h"
#include "gui/interface/Label.h"
#include "gui/interface/Button.h"
#include "gui/interface/Checkbox.h"
#include "gui/interface/Textbox.h"
#include "gui/interface/DropDown.h"
#include "gui/interface/Slider.h"
#include "gui/dialogues/InformationMessage.h"
#include "gui/dialogues/ErrorMessage.h"
#include "gui/dialogues/ConfirmPrompt.h"
#include "gui/dialogues/TextPrompt.h"

#include <cmath>
#include <algorithm>
#include <ctime>

bool RecordMenu::RecordingActive(RecordStage stage)
{
	return stage == RecordStage::Recording || stage == RecordStage::Paused;
}

void RecordMenu::ToggleRecording()
{
	newStage = newStage == RecordStage::Stopped ? RecordStage::Recording : RecordStage::Stopped;
}

void RecordMenu::UpdateTitle()
{
	if (rs.writing)
	{
		titleLabel->SetText(String::Build("Record Menu - ", ((int)rs.frame) + 1, "/", (int)rs.nextFrame));
	}
	else
	{
		titleLabel->SetText("Record Menu");
	}
}

void RecordMenu::StateChanged()
{
	auto col = [](bool en) {
		return en ? ui::Colour(255, 255, 255) : ui::Colour(100, 100, 100);
	};

	bool rec = RecordingActive(newStage);
	bool pas = newStage == RecordStage::Paused;
	bool old = rs.format == RecordFormat::Old;
	bool stop = !rec && rs.stage != RecordStage::Writing;

	bool fullscreen = rs.x1 == 0 && rs.y1 == 0 && rs.x2 == WINDOWW && rs.y2 == WINDOWH;

	// Title Label
	UpdateTitle();

	// Select Button
	if (old)
	{
		rs.x1 = 0;
		rs.y1 = 0;
		rs.x2 = WINDOWW;
		rs.y2 = WINDOWH;
	}
	bool positionChanged = (rs.x1 != 0 || rs.y1 != 0 || rs.x2 != XRES || rs.y2 != YRES) && !old;
	selectButton->Enabled = stop && !old && !fullscreen;
	selectButton->Appearance.BackgroundInactive = positionChanged ? ui::Colour(47, 47, 23) : ui::Colour(0, 0, 0);
	selectButton->Appearance.BackgroundHover = positionChanged ? ui::Colour(63, 63, 31) : ui::Colour(20, 20, 20);

	// FPS Label
	fpsLabel->SetTextColour(col(stop));

	// FPS Textbox
	fpsTextbox->Enabled = stop;
	fpsTextbox->ReadOnly = !stop;
	fpsTextbox->SetText(String::Build(rs.fps));
	fpsTextbox->SetTextColour(col(stop));

	// Fullscreen Label
	fullscreenLabel->SetTextColour(col(stop && !old));

	// Fullscreen Checkbox
	fullscreenCheckbox->Enabled = stop && !old;
	fullscreenCheckbox->SetChecked(fullscreen && !old);

	// Include UI Label
	includeUILabel->SetTextColour(col(stop && !old && !fullscreen));

	// Include UI Checkbox
	if (old)
	{
		rs.includeUI = false;
	}
	else if (fullscreen)
	{
		rs.includeUI = true;
	}
	includeUICheckbox->Enabled = stop && !old && !fullscreen;
	includeUICheckbox->SetChecked(rs.includeUI);

	// Scale Label
	scaleLabel->SetTextColour(col(stop && !old));

	// Scale Dropdown
	if (old)
	{
		rs.scale = 1;
	}
	scaleDropdown->Enabled = stop && !old;
	scaleDropdown->SetOption(rs.spacing ? -8 : rs.scale);

	// Format Label
	formatLabel->SetTextColour(col(stop));

	// Format Dropdown
	formatDropdown->Enabled = stop;
	formatDropdown->SetOption(rs.format);

	// Buffer Label
	bufferLabel->SetTextColour(col(stop));

	// Buffer Dropdown
	bufferDropdown->Enabled = stop;
	bufferDropdown->SetOption(rs.buffer);

	// Buffer Usage Label
	if (rs.buffer == RecordBuffer::Off)
	{
		bufferUsageLabel->SetText("Buffer not enabled");
		bufferUsageLabel->SetTextColour(ui::Colour(100, 100, 100));
	}
	else
	{
		int usage = ((rs.x2 - rs.x1) * (rs.y2 - rs.y1) * rs.fps * 4) / 1048576; // MB/sec;
		bufferUsageLabel->SetText(usage ? String::Build("Usage: ", usage, " MB/sec") : "Usage: <1 MB/sec");
		bufferUsageLabel->SetTextColour(usage > 100 && stop ? ui::Colour(255, 63, 63) : col(stop));
	}

	// Buffer Usage Button
	bufferUsageButton->Enabled = rs.buffer != RecordBuffer::Off && stop;
	bufferUsageButton->Appearance.BackgroundInactive = rs.bufferLimit ? ui::Colour(47, 47, 23) : ui::Colour(0, 0, 0);
	bufferUsageButton->Appearance.BackgroundHover = rs.bufferLimit ? ui::Colour(63, 63, 31) : ui::Colour(20, 20, 20);

	// Write Thread Label
	writeThreadLabel->SetTextColour(rs.buffer == RecordBuffer::Off ? ui::Colour(100, 100, 100) : col(stop));

	// Write Thread Checkbox
	if (rs.buffer == RecordBuffer::Off)
	{
		rs.writeThread = false;
	}
	writeThreadCheckbox->Enabled = rs.buffer != RecordBuffer::Off && stop;
	writeThreadCheckbox->SetChecked(rs.writeThread);

	// Quality Label
	if (rs.format == RecordFormat::WebP)
	{
		qualityLabel->SetText(rs.quality == 10 ? "Compres. (M):" : String::Build("Compres. (", rs.quality, "):"));
	}
	else // Why do nested conditional operators have to look so bad
	{
		qualityLabel->SetText("Compres. (-):");
	}
	qualityLabel->SetTextColour(stop && rs.format == RecordFormat::WebP ? ui::Colour(255, 255, 255) : ui::Colour(100, 100, 100));

	// Quality Slider
	qualitySlider->Enabled = stop && rs.format == RecordFormat::WebP;
	qualitySlider->SetValue(rs.format == RecordFormat::WebP ? rs.quality : 10);
	if (qualitySlider->Enabled)
	{
		qualitySlider->SetColour(
			rs.quality == 10 ? ui::Colour(0, 95, 0) : ui::Colour(0, 0, 0),
			rs.quality == 10 ? ui::Colour(0, 95, 0) : ui::Colour(0, 63, 0)
		);
	}
	else
	{
		qualitySlider->SetColour(ui::Colour(10, 10, 10), ui::Colour(10, 10, 10));
	}

	// Reset Button
	resetButton->Enabled = stop;

	// Pause Button
	pauseButton->Enabled = rec;
	pauseButton->SetText(pas ? String::Build("Unpause") : String::Build("Pause"));
	pauseButton->Appearance.BackgroundInactive = pas ? ui::Colour(47, 47, 95) : ui::Colour(15, 15, 31);
	pauseButton->Appearance.BackgroundHover = pas ? ui::Colour(63, 63, 127) : ui::Colour(31, 31, 63);

	// Close Button

	// Start Button
	if (rs.stage == RecordStage::Writing)
	{
		startButton->SetText("Cancel");
	}
	else
	{
		startButton->SetText(rec ? String::Build("Stop") : String::Build("Start"));
	}
	startButton->Appearance.BackgroundInactive = rec ? ui::Colour(95, 47, 47) : ui::Colour(31, 15, 15);
	startButton->Appearance.BackgroundHover = rec ? ui::Colour(127, 63, 63) : ui::Colour(63, 31, 31);
}

void RecordMenu::ExitSetRecord()
{
	auto& rc = RecordController::Ref();
	if (rs.stage != RecordStage::Writing)
	{
		if (!RecordingActive(rs.stage) && RecordingActive(newStage))
		{
			rc.StartRecording();
		}
		rs.stage = newStage;
	}
	rc.SetCallback(nullptr);
	rs.halt = false;
	CloseActiveWindow();
	SelfDestruct();
}

void RecordMenu::OnTryExit(ExitMethod method)
{
	ExitSetRecord();
}

void RecordMenu::OnDraw()
{
	Graphics* g = GetGraphics();
	g->clearrect(Position.X-2, Position.Y-2, Size.X+3, Size.Y+3);
	g->drawrect(Position.X, Position.Y, Size.X, Size.Y, 200, 200, 200, 255);
}

RecordMenu::RecordMenu() :
	ui::Window(ui::Point(-1, -1), ui::Point(160, 246)),
	rs(RecordController::Ref().rs)
{
	auto& rc = RecordController::Ref();
	newStage = rs.stage;
	rs.halt = true;

	// Title Label
	titleLabel = new ui::Label(ui::Point(4, 5), ui::Point(Size.X - 8, 15), "Record Menu");
	titleLabel->SetTextColour(style::Colour::InformationTitle);
	titleLabel->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;

	// Help Button
	ui::Button* helpButton = new ui::Button(ui::Point(Size.X - 20, 4), ui::Point(16, 16), "?");
	helpButton->SetActionCallback({ []() {
		new InformationMessage("Record Menu Help", String::Build("\bo",
			"Select Area:\n\bw\t",
			"Select the area of the screen to record (inclusive).\n\bo",
			"FPS (Max 60):\n\bw\t",
			"Framerate of final recording, dropping frames if necessary. Not affected by lag, but assumes game should be running at 60fps (Ignores tpt.setfpscap).\n\bo",
			"Fullscreen:\n\bw\t",
			"Records the entire window, including the UI. Overrides area selection.\n\bo",
			"Include UI:\n\bw\t",
			"Includes the HUD in recordings.\n\bo",
			"Scale:\n\bl\t",
			"*** Slows write performance with large areas ***\n\bw\t",
			"Duplicates pixels to make recording larger. Useful if blurry when being upscaled. \"8+Spacing\" adds black areas between pixels (like zoom window).\n\bo",
			"Format:\n\bw\t",
			"Output image format. WebP is much slower at processing frames with long recordings but is lossless. Gif framerate may be incorrect due to precision limitations (10ms increments). Old replicates tpt.record() recordings, but disables most settings and creates many uncompressed files.\n\bo",
			"Buffer:\n\bw\t",
			"Stores image data before processing to improve performance while recording. Final image is created after recording stops. Ram stores data in memory and is fast, but may use large amounts of ram. Disk stores data in the recordings folder temporarily. Size depends on area.\n\bo",
			"Buffer Usage/Limit:\n\bl\t",
			"*** Limiter does not account for memory usage of other programs ***\n\bw\t",
			"Usage shows amount of memory or disk space used for each second of recording with the current settings. Limiter will automatically stop the recording if the buffer is about to exceed this size.\n\bo",
			"Write Thread:\n\bw\t",
			"Start processing frames on a separate thread immediately. Can reduce write time with similar game performance if multiple cores are available.\n\bo"
			"Compression Strength (WebP only):\n\bw\t",
			"Amount of effort put into compression. Larger values result in smaller files at the cost of write time. Max value (M) enables additional size reduction. \n\bu\t",
			"Note: WebP images are always stored with lossless compression.\n\bo",
			"Reset:\n\bw\t"
			"Reset all settings (including record area) to defaults.\n\bo",
			"Pause:\n\bw\t",
			"Temporarily stops recording.\n\bo",
			"\uE06B:\n\bw\t",
			"Bagel."
			), true);
	} });

	int currentY = 25;

	// Select Button
	selectButton = new ui::Button(ui::Point(8, currentY), ui::Point(67, 16), "Select Area");
	selectButton->SetActionCallback({ [this]() {
		rs.select = 1;
		RecordController::Ref().SetCallback(nullptr);
		CloseActiveWindow();
		SelfDestruct();
	} });

	// FPS Label
	fpsLabel = new ui::Label(ui::Point(85, currentY), ui::Point(27, 16), "FPS:");
	fpsLabel->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;

	// FPS Textbox
	fpsTextbox = new ui::Textbox(ui::Point(112, currentY), ui::Point(40, 16), "60", "[fps]");
	fpsTextbox->SetInputType(ui::Textbox::ValidInput::Number);
	fpsTextbox->SetLimit(4);
	fpsTextbox->SetActionCallback({ [this]() {
		try {
			if (fpsTextbox->GetText().size() > 0)
			{
				rs.fps = std::max(std::min(fpsTextbox->GetText().ToNumber<int>(), 60), 1);
				StateChanged();
			}
		}
		catch (const std::exception& e)
		{
			new ErrorMessage("Could not set FPS", "Invalid value provided.");
		}
	} });

	currentY += 20;

	// Fullscreen Label
	fullscreenLabel = new ui::Label(ui::Point(8, currentY), ui::Point(67, 16), "Fullscreen:");
	fullscreenLabel->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;

	// Fullscreen Checkbox
	fullscreenCheckbox = new ui::Checkbox(ui::Point(85, currentY), ui::Point(16, 16), "", "");
	fullscreenCheckbox->SetActionCallback({ [this]() {
		rs.x1 = 0;
		rs.y1 = 0;
		bool checked = fullscreenCheckbox->GetChecked();
		rs.x2 = checked ? WINDOWW : XRES;
		rs.y2 = checked ? WINDOWH : YRES;
		StateChanged();
	} });

	currentY += 20;

	// Include UI Label
	includeUILabel = new ui::Label(ui::Point(8, currentY), ui::Point(67, 16), "Include UI:");
	includeUILabel->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;

	// Include UI Checkbox
	includeUICheckbox = new ui::Checkbox(ui::Point(85, currentY), ui::Point(16, 16), "", "");
	includeUICheckbox->SetActionCallback({ [this]() {
		rs.includeUI = includeUICheckbox->GetChecked();
		StateChanged();
	} });

	currentY += 20;

	// Scale Label
	scaleLabel = new ui::Label(ui::Point(8, currentY), ui::Point(67, 16), "Scale:");
	scaleLabel->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;

	// Scale Dropdown
	scaleDropdown = new ui::DropDown(ui::Point(85, currentY), ui::Point(67, 16));
	scaleDropdown->AddOption(std::pair<String, int>("1", 1));
	scaleDropdown->AddOption(std::pair<String, int>("2", 2));
	scaleDropdown->AddOption(std::pair<String, int>("4", 4));
	scaleDropdown->AddOption(std::pair<String, int>("8", 8)); // Limited max size due to memory usage
	scaleDropdown->AddOption(std::pair<String, int>("8+Spacing", -8));
	scaleDropdown->SetActionCallback({ [this]() {
		int option = scaleDropdown->GetOption().second;
		rs.scale = std::abs(option);
		rs.spacing = option < 0;
		StateChanged();
	} });

	currentY += 20;

	// Format Label
	formatLabel = new ui::Label(ui::Point(8, currentY), ui::Point(67, 16), "Format:");
	formatLabel->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;

	// Format Dropdown
	formatDropdown = new ui::DropDown(ui::Point(85, currentY), ui::Point(67, 16));
	formatDropdown->AddOption(std::pair<String, int>("Gif", RecordFormat::Gif));
	formatDropdown->AddOption(std::pair<String, int>("WebP", RecordFormat::WebP));
	formatDropdown->AddOption(std::pair<String, int>("Old", RecordFormat::Old));
	formatDropdown->SetActionCallback({ [this]() {
		rs.format = RecordFormat(formatDropdown->GetOption().second);
		StateChanged();
	} });

	currentY += 20;

	// Buffer Label
	bufferLabel = new ui::Label(ui::Point(8, currentY), ui::Point(67, 16), "Buffer:");
	bufferLabel->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;

	// Buffer Dropdown
	bufferDropdown = new ui::DropDown(ui::Point(85, currentY), ui::Point(67, 16));
	bufferDropdown->AddOption(std::pair<String, int>("Off", RecordBuffer::Off));
	bufferDropdown->AddOption(std::pair<String, int>("Ram", RecordBuffer::Ram));
	bufferDropdown->AddOption(std::pair<String, int>("Disk", RecordBuffer::Disk));
	bufferDropdown->SetActionCallback({ [this]() {
		rs.buffer = RecordBuffer(bufferDropdown->GetOption().second);
		StateChanged();
	} });

	currentY += 20;

	// Buffer Usage Label
	bufferUsageLabel = new ui::Label(ui::Point(12, currentY), ui::Point(100, 16), "");

	// Buffer Usage Button
	bufferUsageButton = new ui::Button(ui::Point(Size.X - 40 , currentY), ui::Point(32, 16), "Limit");
	bufferUsageButton->SetActionCallback({ [this]() {
		String output = TextPrompt::Blocking("Buffer Usage Limit", "Limit for record buffer in MB.\nSet to 0 for none. Max 32GB.", String::Build(rs.bufferLimit), ui::Textbox::ValidInput::Number, "", false);
		try
		{
			if (output.size() > 0)
			{
				rs.bufferLimit  = std::min(output.ToNumber<int>(), 32768);
			}
			StateChanged();
		}
		catch (const std::exception& e)
		{
			new ErrorMessage("Could not set buffer limit", "Invalid value provided.");
		}
	} });

	currentY += 20;

	// Write Thread Label
	writeThreadLabel = new ui::Label(ui::Point(8, currentY), ui::Point(67, 16), "Write Thread:");
	writeThreadLabel->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;

	// Write Thread Checkbox
	writeThreadCheckbox = new ui::Checkbox(ui::Point(85, currentY), ui::Point(16, 16), "", "");
	writeThreadCheckbox->SetActionCallback({ [this]() {
		rs.writeThread = writeThreadCheckbox->GetChecked();
		StateChanged();
	} });

	currentY += 20;

	// Quality Label (Actually compression strength in lossless mode)
	qualityLabel = new ui::Label(ui::Point(8, currentY), ui::Point(67, 16), "Compres. (-):");
	qualityLabel->Appearance.HorizontalAlign = ui::Appearance::AlignLeft;

	// Quality Slider
	qualitySlider = new ui::Slider(ui::Point(85, currentY), ui::Point(67, 16), 10);
	qualitySlider->SetActionCallback({ [this]() {
		rs.quality = qualitySlider->GetValue();
		StateChanged();
	} });

	currentY += 20;

	// Reset Button
	resetButton = new ui::Button(ui::Point(8, currentY), ui::Point(67, 16), "Reset");
	resetButton->SetActionCallback({ [this]() {
		rs.Clear();
		StateChanged();
	} });

	// Pause Button
	pauseButton = new ui::Button(ui::Point(85, currentY), ui::Point(67, 16), "Pause");
	pauseButton->Appearance.BackgroundHover = ui::Colour(63, 63, 127);
	pauseButton->SetActionCallback({ [this]() {
		newStage = newStage == RecordStage::Recording ? RecordStage::Paused : RecordStage::Recording;
		StateChanged();
	} });

	// Close Button
	closeButton = new ui::Button(ui::Point(0, Size.Y - 17), ui::Point((Size.X / 2) + 1, 17), "Close");
	closeButton->Appearance.BorderInactive = ui::Colour(200, 200, 200);
	closeButton->SetActionCallback({ [this]() {
		ExitSetRecord();
	} });

	// Start Button
	startButton = new ui::Button(ui::Point(Size.X / 2, Size.Y - 17), ui::Point(Size.X / 2, 17), "Start");
	startButton->Appearance.BorderInactive = ui::Colour(200, 200, 200);
	startButton->SetActionCallback({ [this]() {
		auto& rc = RecordController::Ref();
		if (rs.stage == RecordStage::Writing)
		{
			if (!ConfirmPrompt::Blocking("Cancel Recording", "The remaining unprocessed frames will be lost."))
			{
				return;
			}
			rc.CancelWrite();
		}
		else if (RecordingActive(rs.stage) && RecordingActive(newStage))
		{
			if (!ConfirmPrompt::Blocking("Stop Recording", "This will immediately end the recording."))
			{
				return;
			}
			rc.StopRecording();
			newStage = rs.stage;
		}
		else
		{
			ToggleRecording();
		}
		StateChanged();
	} });

	StateChanged();

	AddComponent(titleLabel);
	AddComponent(helpButton);
	AddComponent(selectButton);
	AddComponent(fpsLabel);
	AddComponent(fpsTextbox);
	AddComponent(fullscreenLabel);
	AddComponent(fullscreenCheckbox);
	AddComponent(includeUILabel);
	AddComponent(includeUICheckbox);
	AddComponent(scaleLabel);
	AddComponent(scaleDropdown);
	AddComponent(formatLabel);
	AddComponent(formatDropdown);
	AddComponent(bufferLabel);
	AddComponent(bufferDropdown);
	AddComponent(bufferUsageLabel);
	AddComponent(bufferUsageButton);
	AddComponent(writeThreadLabel);
	AddComponent(writeThreadCheckbox);
	AddComponent(qualityLabel);
	AddComponent(qualitySlider);
	AddComponent(resetButton);
	AddComponent(pauseButton);
	AddComponent(closeButton);
	AddComponent(startButton);

	SetOkayButton(closeButton);
	MakeActiveWindow();
	rc.SetCallback([this]() {
		if (rs.stage == RecordStage::Stopped)
		{
			newStage = rs.stage;
			StateChanged();
		}
		else
		{
			UpdateTitle();
		}
	});
}
