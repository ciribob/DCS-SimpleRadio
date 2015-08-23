#ifndef SR_SOUNDPLAYER_H
#define SR_SOUNDPLAYER_H

#define INIT_GUID

#include <dsound.h>
#include <string>
#include <unordered_map>

#pragma comment(lib, "dsound.lib")
#pragma comment(lib, "dxguid.lib")

namespace SimpleRadio
{
	class SoundPlayer
	{
	public:
		SoundPlayer();
		~SoundPlayer();
		void Initialize();
		void Load(std::string name);
		void Play(std::string name, float pan = 0.0f, float volume = 1.0f);
		void Shutdown();

	private:
		IDirectSound8* directSound;
		IDirectSoundBuffer* primaryBuffer;
		std::unordered_map<std::string, IDirectSoundBuffer8*> sounds;
		static std::string GetSoundsFolder();
	};
};

#endif
