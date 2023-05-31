#include "RecordController.h"

#include <iostream>
#include <cstdio>
#include <fstream>
#include <cmath>
#include <chrono>
#include <thread>

#include "common/String.h"
#include "common/Platform.h"
#include "graphics/Graphics.h"

// Gif
#define MSF_GIF_IMPL
#include "msf_gif.h"

// WebP
#include <webp/types.h>
#include <webp/encode.h>
#include <webp/mux_types.h>
#include <webp/mux.h>

// Old
#include "Format.h"

// TODO: Error handling...

void RecordController::StartRecordingCurrent()
{
	switch (rs.format)
	{
		case RecordFormat::Gif:
			gifState = new MsfGifState;
			msf_gif_begin(gifState, sxs, sys);
			break;

		case RecordFormat::WebP:
			{
				WebPAnimEncoderOptions enc_options;
				WebPAnimEncoderOptionsInit(&enc_options);
				enc_options.anim_params.bgcolor = 0xFF000000;
				enc_options.anim_params.loop_count = 0;
				enc_options.minimize_size = rs.quality == 10;
				enc = WebPAnimEncoderNew(sxs, sys, &enc_options);
			}
			break;

		case RecordFormat::Old:
			Platform::MakeDirectory(ByteString::Build("recordings", PATH_SEP, rs.file).c_str());
			break;
	}
}

void RecordController::WriteFrameCurrent(uint32_t* buffer)
{
	if (rs.scale != 1)
	{
		uint32_t* oldBuffer = buffer;
		buffer = new uint32_t[sbufs];
		int index = 0;
		for (int y = 0; y < ys; y++)
		{
			for (int s1 = 0; s1 < rs.scale; s1++)
			{
				for (int x = 0; x < xs; x++)
				{
					for (int s2 = 0; s2 < rs.scale; s2++)
					{
						if (rs.spacing && (s1 == 7 || s2 == 7))
						{
							buffer[index++] = 0xFF000000;
						}
						else
						{
							buffer[index++] = oldBuffer[y * xs + x];
						}
					}
				}
			}
		}
		delete[] oldBuffer;
	}
	switch (rs.format)
	{
		case RecordFormat::Gif:
			msf_gif_frame(gifState, (uint8_t*)buffer, (int)rs.delay, 16, sxs * 4);
			break;

		case RecordFormat::WebP:
			{
				WebPConfig config;
				WebPConfigInit(&config);
				config.lossless = true;
				config.quality = (float)(rs.quality * 10);
				WebPPicture pic;
				WebPPictureInit(&pic);
				pic.use_argb = true;
				pic.width = sxs;
				pic.height = sys;
				pic.argb = buffer;
				pic.argb_stride = sxs;
				WebPAnimEncoderAdd(enc, &pic, rs.frame * (int)rs.delay, &config);
				WebPPictureFree(&pic);
			}
			break;

		case RecordFormat::Old:
			{
				VideoBuffer screenshot((pixel*)buffer, sxs, sys);
				std::vector<char> data = format::VideoBufferToPPM(screenshot);
				ByteString path = ByteString::Build("recordings", PATH_SEP, rs.file, PATH_SEP, "frame_", Format::Width(rs.frame++, 6), ".ppm");
				Platform::WriteFile(data, path);
			}
			break;
	}
	delete[] buffer;
}

void RecordController::StopRecordingCurrent()
{
	const char* extension = "";
	switch (rs.format)
	{
		case RecordFormat::Gif:
			extension = ".gif";
			break;

		case RecordFormat::WebP:
			extension = ".webp";
			break;

		case RecordFormat::Old:
			return;
			break;
	}

	try
	{
		std::ofstream outFile(ByteString::Build("recordings", PATH_SEP, rs.file, extension), std::ios::binary | std::ios::trunc);
		if(outFile.is_open())
		{
			switch (rs.format)
			{
				case RecordFormat::WebP:
					{
						WebPAnimEncoderAdd(enc, NULL, rs.frame * (int)rs.delay, NULL);
						WebPData webp_data;
						WebPDataInit(&webp_data);
						WebPAnimEncoderAssemble(enc, &webp_data);
						WebPAnimEncoderDelete(enc);
						outFile.write((const char*)webp_data.bytes, webp_data.size);
						WebPDataClear(&webp_data);
					}
					break;

				case RecordFormat::Gif:
					{
						MsfGifResult result = msf_gif_end(gifState);
						delete gifState;
						outFile.write((const char*)result.data, result.dataSize);
						msf_gif_free(result);
					}
					break;

				case RecordFormat::Old:
					return;
					break;
			}
			outFile.close();
		}
		else
		{
			std::cerr << "RecordController::StopRecordingCurrent->Fail: " << std::endl;
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "RecordController::StopRecordingCurrent->Catch: " << e.what() << std::endl;
	}
}

void RecordController::StartWriteThread()
{
	rs.writing = true;
	stopWrite = false;
	std::thread writeThread([this]() {
		auto tp1 = std::chrono::high_resolution_clock::now();
		switch (rs.buffer)
		{
			case RecordBuffer::Off:
				break;

			case RecordBuffer::Ram:
				StartRecordingCurrent();
				for (WaitForFrames() ; rs.frame < rs.nextFrame || rs.stage != RecordStage::Writing; rs.frame++)
				{
					if (stopWrite)
					{
						FreeRemaining();
						goto ABORT;
					}
					bufferDataMutex.lock();
					uint32_t* buffer = bufferData[rs.frame];
					bufferData[rs.frame] = nullptr;
					bufferDataMutex.unlock();
					WriteFrameCurrent(buffer);
					CallCallback();
					WaitForFrames();
				}
				break;

			case RecordBuffer::Disk:
				StartRecordingCurrent();
				try
				{
					std::ifstream bufferFile(ByteString::Build("recordings", PATH_SEP, rs.file, ".buf").c_str(), std::ios::binary);
					if(bufferFile.is_open())
					{
						for (WaitForFrames() ; rs.frame < rs.nextFrame || rs.stage != RecordStage::Writing; rs.frame++)
						{
							if (stopWrite)
							{
								bufferFile.close();
								Platform::RemoveFile(ByteString::Build("recordings", PATH_SEP, rs.file, ".buf"));
								goto ABORT;
							}
							uint32_t* buffer = new uint32_t[bufs];
							bufferDataMutex.lock();
							bufferFile.seekg(rs.frame * bufs * 4);
							bufferFile.read((char*)buffer, bufs * 4);
							bufferDataMutex.unlock();
							WriteFrameCurrent(buffer);
							CallCallback();
							WaitForFrames();
						}
						bufferFile.close();
						Platform::RemoveFile(ByteString::Build("recordings", PATH_SEP, rs.file, ".buf"));
					}
					else
					{
						std::cerr << "RecordController::StartWriteThread->Fail: " << std::endl;
					}
				}
				catch (const std::exception& e)
				{
					std::cerr << "RecordController::StartWriteThread->Catch: " << e.what() << std::endl;
				}
				break;
		}
ABORT:
		StopRecordingCurrent();
		if (rs.writeThread)
		{
			std::cout << "Wrote " << rs.nextFrame << " frames" << std::endl;
		}
		else
		{
			auto tp2 = std::chrono::high_resolution_clock::now();
			int ms = std::chrono::duration_cast<std::chrono::milliseconds>(tp2 - tp1).count();
			std::cout << "Wrote " << rs.nextFrame << " frames in " << ms << "ms (avg " << (std::round(((float)ms / (float)rs.nextFrame) * 100) / 100) << "ms/frame" << ")" << std::endl;
		}
		rs.stage = RecordStage::Stopped;
		rs.writing = false;
		rs.file = 0;
		CallCallback();
		stopWrite = false;
	});
	writeThread.detach();
}

void RecordController::WaitForFrames()
{
	if (rs.writeThread)
	{
 		while (rs.frame + 1 >= rs.nextFrame && !stopWrite && rs.stage != RecordStage::Writing)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		}
	}
}

void RecordController::CallCallback()
{
	callbackMutex.lock();
	if (callback)
	{
		callback();
	}
	callbackMutex.unlock();
}

void RecordController::FreeRemaining()
{
	bufferDataMutex.lock();
	for (unsigned int i = 0; i < bufferData.size(); i++)
	{
		if (bufferData[i])
		{
			delete[] bufferData[i];
			bufferData[i] = nullptr;
		}
	}
	bufferDataMutex.unlock();
	rs.nextFrame = rs.frame + 1;
}

void RecordController::StartRecording()
{
	xs = rs.x2 - rs.x1;
	ys = rs.y2 - rs.y1;
	bufs = xs * ys;
	sxs = xs * rs.scale;
	sys = ys * rs.scale;
	sbufs = sxs * sys;
	rs.delay = 1000.0f / (float)rs.fps;
	rs.delay = rs.format == RecordFormat::Gif ? std::round(rs.delay / 10.0f) : rs.delay;
	rs.ratio = (int)std::round(60.0f / (float)rs.fps);
	rs.file = (time_t)time(NULL);
	rs.ClearCounters();
	Platform::MakeDirectory("recordings");
	rs.stage = RecordStage::Recording;
	if (rs.buffer == RecordBuffer::Off)
	{
		StartRecordingCurrent();
	}
	else if (rs.writeThread)
	{
		StartWriteThread();
	}
}

void RecordController::WriteFrame(Graphics* g)
{
	uint32_t* buffer = nullptr;
	switch (rs.format)
	{
		case RecordFormat::Gif:
			buffer = g->DumpFrameRGBA(rs.x1, rs.y1, rs.x2, rs.y2);
			break;

		case RecordFormat::WebP:
		case RecordFormat::Old:
			buffer = g->DumpFrameARGB(rs.x1, rs.y1, rs.x2, rs.y2);
			break;
	}

	bufferDataMutex.lock();
	switch (rs.buffer)
	{
		case RecordBuffer::Off:
			WriteFrameCurrent(buffer);
			rs.frame++;
			break;

		case RecordBuffer::Ram:
			if (bufferData.size() > (unsigned int)rs.nextFrame) // This shouldnt be necessary
			{
				bufferData[rs.nextFrame] = buffer;
			}
			else
			{
				bufferData.push_back(buffer);
			}
			break;

		case RecordBuffer::Disk:
			try
			{
				std::ofstream bufferFile(ByteString::Build("recordings", PATH_SEP, rs.file, ".buf"), std::ios::binary | std::ios::app);
				if(bufferFile.is_open())
				{
					bufferFile.write((const char*)buffer, bufs * 4);
					bufferFile.flush();
				}
				else
				{
					std::cerr << "RecordController::WriteFrame->Fail: " << std::endl;
				}
			}
			catch (const std::exception& e)
			{
				std::cerr << "RecordController::WriteFrame->Catch: " << e.what() << std::endl;
			}
			delete[] buffer;
			break;
	}
	bufferDataMutex.unlock();
	rs.nextFrame++;
}

void RecordController::StopRecording()
{
	if (rs.buffer == RecordBuffer::Off)
	{
		rs.stage = RecordStage::Stopped;
		StopRecordingCurrent();
	}
	else
	{
		rs.stage = RecordStage::Writing;
		if (!rs.writeThread)
		{
			StartWriteThread();
		}
	}
}

void RecordController::SetCallback(RecordProgressCallback newCallback)
{
	callbackMutex.lock();
	callback = newCallback;
	callbackMutex.unlock();
}

void RecordController::CancelWrite()
{
	stopWrite = true;
}

RecordController::~RecordController()
{
	FreeRemaining();
}
