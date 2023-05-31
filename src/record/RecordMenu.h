#pragma once

#include "RecordState.h"

#include "gui/interface/Window.h"

namespace ui
{
	class Label;
	class Button;
	class Checkbox;
	class Textbox;
	class DropDown;
	class Slider;
}

class RecordMenu : public ui::Window
{
	RecordState& rs;
	RecordStage newStage; // Writing does not exist here

	ui::Label* titleLabel;
	ui::Button* selectButton;
	ui::Label* fpsLabel;
	ui::Textbox* fpsTextbox;
	ui::Label* fullscreenLabel;
	ui::Checkbox* fullscreenCheckbox;
	ui::Label* includeUILabel;
	ui::Checkbox* includeUICheckbox;
	ui::Label* scaleLabel;
	ui::DropDown* scaleDropdown;
	ui::Label* formatLabel;
	ui::DropDown* formatDropdown;
	ui::Label* bufferLabel;
	ui::DropDown* bufferDropdown;
	ui::Label* bufferUsageLabel;
	ui::Button* bufferUsageButton;
	ui::Label* writeThreadLabel;
	ui::Checkbox* writeThreadCheckbox;
	ui::Label* qualityLabel;
	ui::Slider* qualitySlider;
	ui::Button* resetButton;
	ui::Button* pauseButton;
	ui::Button* closeButton;
	ui::Button* startButton;

	bool RecordingActive(RecordStage stage);
	void ToggleRecording(); // newStage only

	void UpdateTitle();
	void StateChanged();
	void ExitSetRecord();

	void OnTryExit(ExitMethod method);
	void OnDraw();

public:
	RecordMenu();
};
