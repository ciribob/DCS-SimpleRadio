#include "SoundPlayer.h"
#include <fstream>
#include <sstream>

using std::stringstream;
using std::ifstream;
using std::streampos;
using std::ios;

namespace SimpleRadio
{
	SoundPlayer::SoundPlayer()
		: directSound(nullptr)
		, primaryBuffer(nullptr)
		, sounds()
	{
	}

	void SoundPlayer::Initialize()
	{
		HRESULT result = DS_OK;

		// Create DirectSound interface
		result = DirectSoundCreate8(nullptr, &this->directSound, nullptr);
		if (FAILED(result))
		{
			throw "DirectSoundCreate8 failed";
		}

		// Set cooperative level
		result = this->directSound->SetCooperativeLevel(GetDesktopWindow(), DSSCL_PRIORITY);
		if (FAILED(result))
		{
			throw "SetCooperativeLevel failed";
		}

		// Create and configure primary buffer
		DSBUFFERDESC bd = { 0 };
		bd.dwSize = sizeof bd;
		bd.dwFlags = DSBCAPS_PRIMARYBUFFER;
		bd.dwBufferBytes = 0;
		bd.dwReserved = 0;
		bd.lpwfxFormat = nullptr;
		bd.guid3DAlgorithm = GUID_NULL;

		result = this->directSound->CreateSoundBuffer(&bd, &(this->primaryBuffer), nullptr);
		if (FAILED(result))
		{
			throw "CreateSoundBuffer failed";
		}

		WAVEFORMATEX wf = { 0 };
		wf.wFormatTag = WAVE_FORMAT_PCM;
		wf.nChannels = 2;
		wf.wBitsPerSample = 16;
		wf.nSamplesPerSec = 44100;
		wf.nBlockAlign = wf.nChannels * (wf.wBitsPerSample / 8);
		wf.nAvgBytesPerSec = wf.nSamplesPerSec * wf.nBlockAlign;
		wf.cbSize = 0;

		result = this->primaryBuffer->SetFormat(&wf);
		if (FAILED(result))
		{
			throw "SetFormat failed";
		}
	}

	void SoundPlayer::Load(std::string name)
	{
		stringstream path;
		path << SoundPlayer::GetSoundsFolder();
		path << name;

		streampos size = 0;
		char* block = nullptr;

		ifstream stream(path.str(), ios::binary | ios::ate);
		if (stream.is_open())
		{
			size = stream.tellg();
			block = new char[size];
			stream.seekg(0, ios::beg);
			stream.read(block, size);
			stream.close();
		}
		else
		{
			throw "Failed to load file";
		}

		HRESULT result = DS_OK;

		WAVEFORMATEX wf = { 0 };
		wf.wFormatTag = WAVE_FORMAT_PCM;
		wf.nChannels = 1;
		wf.wBitsPerSample = 16;
		wf.nSamplesPerSec = 44100;
		wf.nBlockAlign = wf.nChannels * (wf.wBitsPerSample / 8);
		wf.nAvgBytesPerSec = wf.nSamplesPerSec * wf.nBlockAlign;
		wf.cbSize = 0;

		DSBUFFERDESC bd = { 0 };
		bd.dwSize = sizeof bd;
		bd.dwFlags = DSBCAPS_GLOBALFOCUS | DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLPAN;
		bd.dwBufferBytes = static_cast<DWORD>(size);
		bd.dwReserved = 0;
		bd.lpwfxFormat = &wf;
		bd.guid3DAlgorithm = GUID_NULL;

		IDirectSoundBuffer* tmp = nullptr;
		IDirectSoundBuffer8* secondaryBuffer = nullptr;

		result = this->directSound->CreateSoundBuffer(&bd, &tmp, nullptr);
		if (FAILED(result))
		{
			throw "CreateSoundBuffer failed";
		}

		result = tmp->QueryInterface(IID_IDirectSoundBuffer8, (void**)&secondaryBuffer);
		if (FAILED(result))
		{
			throw "QueryInterface failed";
		}

		tmp->Release();
		tmp = nullptr;

		unsigned char *bufferPtr;
		unsigned long bufferSize;
		result = secondaryBuffer->Lock(0, static_cast<DWORD>(size), (void**)&bufferPtr, static_cast<DWORD*>(&bufferSize), nullptr, 0, DSBLOCK_ENTIREBUFFER);
		if (FAILED(result))
		{
			throw "Lock failed";
		}

		memcpy(bufferPtr, block, bufferSize);

		result = secondaryBuffer->Unlock((void*)bufferPtr, bufferSize, nullptr, 0);
		if (FAILED(result))
		{
			throw "Unlock failed";
		}

		this->sounds[name] = secondaryBuffer;

		delete[] block;
	}

	void SoundPlayer::Play(std::string name, float pan, float volume)
	{
		IDirectSoundBuffer8* buffer = this->sounds.at(name);
		buffer->SetCurrentPosition(0);
		buffer->SetPan(static_cast<LONG>(pan * 10000));
		buffer->SetVolume(static_cast<LONG>(-(10000 - (10000 * volume))));
		buffer->Play(0, 0, 0);
	}

	void SoundPlayer::Shutdown()
	{
		for (auto& sound : this->sounds)
		{
			sound.second->Release();
			sound.second = nullptr;
		}

		this->sounds.clear();

		if (this->primaryBuffer != nullptr)
		{
			this->primaryBuffer->Release();
			this->primaryBuffer = nullptr;
		}

		if (this->directSound != nullptr)
		{
			this->directSound->Release();
			this->directSound = nullptr;
		}
	}

	std::string SoundPlayer::GetSoundsFolder()
	{
		char buffer[MAX_PATH];
		char* path;
		HKEY key;
		DWORD size;

		LONG result = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Master Arms\\MARS", 0, KEY_READ, &key);
		if (result == ERROR_SUCCESS)
		{
			size = MAX_PATH;
			result = RegQueryValueExA(key, "InstallPath", nullptr, nullptr, (BYTE*)buffer, &size);
			if (result == ERROR_SUCCESS)
			{
				path = buffer;
			}
			RegCloseKey(key);
		}

		strcat_s(path, MAX_PATH, "Sounds\\");

		return std::string(path);
	}

	SoundPlayer::~SoundPlayer()
	{
		this->Shutdown();
	}
}
