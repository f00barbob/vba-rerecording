#include <cstring>

#include "../common/System.h"
#include "../common/Util.h"
#include "gbGlobals.h"
#include "gbSound.h"

extern u8  soundBuffer[6][735];
extern u16 soundFinalWave[1470];
extern int soundVolume;

#define SOUND_MAGIC   0x60000000
#define SOUND_MAGIC_2 0x30000000
#define NOISE_MAGIC 5   // (2097152.0 / 44100.0)

extern int gbHardware;

extern void soundResume();

extern u8 soundWavePattern[4][32];

extern u32	 soundBufferLen;
extern u32	 soundBufferTotalLen;
extern int32 soundQuality;
extern int32 soundPaused;
extern int32 soundPlay;
extern int32 soundTicks;
extern int32 SOUND_CLOCK_TICKS;
static int32 GB_SOUND_CLOCK_TICKS = 0;
extern u32	 soundNextPosition;

extern int32 soundLevel1;
extern int32 soundLevel2;
extern int32 soundBalance;
extern int32 soundMasterOn;
extern u32	 soundIndex;
extern u32	 soundBufferIndex;
int			 soundVIN = 0;
extern int32 soundDebug;

extern int32 sound1On;
extern int32 sound1ATL;
int			 sound1ATLreload;
int			 freq1low;
int			 freq1high;
extern int32 sound1Skip;
extern int32 sound1Index;
extern int32 sound1Continue;
extern int32 sound1EnvelopeVolume;
extern int32 sound1EnvelopeATL;
extern int32 sound1EnvelopeUpDown;
extern int32 sound1EnvelopeATLReload;
extern int32 sound1SweepATL;
extern int32 sound1SweepATLReload;
extern int32 sound1SweepSteps;
extern int32 sound1SweepUpDown;
extern int32 sound1SweepStep;
extern u8 *	 sound1Wave;

extern int32 sound2On;
extern int32 sound2ATL;
int			 sound2ATLreload;
int			 freq2low;
int			 freq2high;
extern int32 sound2Skip;
extern int32 sound2Index;
extern int32 sound2Continue;
extern int32 sound2EnvelopeVolume;
extern int32 sound2EnvelopeATL;
extern int32 sound2EnvelopeUpDown;
extern int32 sound2EnvelopeATLReload;
extern u8 *	 sound2Wave;

extern int32 sound3On;
extern int32 sound3ATL;
int			 sound3ATLreload;
int			 freq3low;
int			 freq3high;
extern int32 sound3Skip;
extern int32 sound3Index;
extern int32 sound3Continue;
extern int32 sound3OutputLevel;
extern int32 sound3Last;

extern int32 sound4On;
extern int32 sound4Clock;
extern int32 sound4ATL;
int			 sound4ATLreload;
int			 freq4;
extern int32 sound4Skip;
extern int32 sound4Index;
extern int32 sound4ShiftRight;
extern int32 sound4ShiftSkip;
extern int32 sound4ShiftIndex;
extern int32 sound4NSteps;
extern int32 sound4CountDown;
extern int32 sound4Continue;
extern int32 sound4EnvelopeVolume;
extern int32 sound4EnvelopeATL;
extern int32 sound4EnvelopeUpDown;
extern int32 sound4EnvelopeATLReload;

extern int32 soundEnableFlag;
extern int32 soundMutedFlag;

extern int32 soundFreqRatio[8];
extern int32 soundShiftClock[16];

extern s16	 soundFilter[4000];
extern s16	 soundLeft[5];
extern s16	 soundRight[5];
extern int32 soundEchoIndex;
extern bool8 soundEcho;
extern bool8 soundLowPass;
extern bool8 soundReverse;
extern bool8 soundOffFlag;

bool8 gbDigitalSound = false;

void gbSoundEvent(register u16 address, register int data)
{
	int freq = 0;

	gbMemory[address] = data;

#ifndef FINAL_VERSION
	if (soundDebug)
	{
		// don't translate. debug only
		log("Sound event: %08lx %02x\n", address, data);
	}
#endif
	switch (address)
	{
	case NR10:
		gbMemory[address] = data | 0x80;
		sound1SweepATL	  = sound1SweepATLReload = 344 * ((data >> 4) & 7);
		sound1SweepSteps  = data & 7;
		sound1SweepUpDown = data & 0x08;
		sound1SweepStep	  = 0;
		break;
	case NR11:
		gbMemory[address] = data | 0x3f;
		sound1Wave		  = soundWavePattern[data >> 6];
		sound1ATL		  = sound1ATLreload = 172 * (64 - (data & 0x3f));
		break;
	case NR12:
		sound1EnvelopeVolume	= data >> 4;
		sound1EnvelopeUpDown	= data & 0x08;
		sound1EnvelopeATLReload = sound1EnvelopeATL = 689 * (data & 7);
		break;
	case NR13:
		gbMemory[address] = 0xff;
		freq1low		  = data;
		freq			  = ((((int)(freq1high & 7)) << 8) | freq1low);
		sound1ATL		  = sound1ATLreload;
		freq			  = 2048 - freq;
		if (freq)
		{
			sound1Skip = SOUND_MAGIC / freq;
		}
		else
			sound1Skip = 0;
		break;
	case NR14:
		gbMemory[address] = data | 0xbf;
		freq1high		  = data;
		freq			  = ((((int)(freq1high & 7)) << 8) | freq1low);
		freq			  = 2048 - freq;
		sound1ATL		  = sound1ATLreload;
		sound1Continue	  = data & 0x40;
		if (freq)
		{
			sound1Skip = SOUND_MAGIC / freq;
		}
		else
			sound1Skip = 0;
		if (data & 0x80)
		{
			gbMemory[NR52] |= 1;
			sound1EnvelopeVolume	= gbMemory[NR12] >> 4;
			sound1EnvelopeUpDown	= gbMemory[NR12] & 0x08;
			sound1EnvelopeATLReload = sound1EnvelopeATL = 689 * (gbMemory[NR12] & 7);
			sound1SweepATL			= sound1SweepATLReload = 344 * ((gbMemory[NR10] >> 4) & 7);
			sound1SweepSteps		= gbMemory[NR10] & 7;
			sound1SweepUpDown		= gbMemory[NR10] & 0x08;
			sound1SweepStep			= 0;

			sound1Index = 0;
			sound1On	= 1;
		}
		break;
	case NR21:
		gbMemory[address] = data | 0x3f;
		sound2Wave		  = soundWavePattern[data >> 6];
		sound2ATL		  = sound2ATLreload = 172 * (64 - (data & 0x3f));
		break;
	case NR22:
		sound2EnvelopeVolume	= data >> 4;
		sound2EnvelopeUpDown	= data & 0x08;
		sound2EnvelopeATLReload = sound2EnvelopeATL = 689 * (data & 7);
		break;
	case NR23:
		gbMemory[address] = 0xff;
		freq2low		  = data;
		freq			  = (((int)(freq2high & 7)) << 8) | freq2low;
		sound2ATL		  = sound2ATLreload;
		freq			  = 2048 - freq;
		if (freq)
		{
			sound2Skip = SOUND_MAGIC / freq;
		}
		else
			sound2Skip = 0;
		break;
	case NR24:
		gbMemory[address] = data | 0xbf;
		freq2high		  = data;
		freq			  = (((int)(freq2high & 7)) << 8) | freq2low;
		freq			  = 2048 - freq;
		sound2ATL		  = sound2ATLreload;
		sound2Continue	  = data & 0x40;
		if (freq)
		{
			sound2Skip = SOUND_MAGIC / freq;
		}
		else
			sound2Skip = 0;
		if (data & 0x80)
		{
			gbMemory[NR52]		|= 2;
			sound2EnvelopeVolume = gbMemory[NR22] >> 4;
			sound2EnvelopeUpDown = gbMemory[NR22] & 0x08;
			sound2ATL = sound2ATLreload;
			sound2EnvelopeATLReload = sound2EnvelopeATL = 689 * (gbMemory[NR22] & 7);

			sound2Index = 0;
			sound2On	= 1;
		}
		break;
	case NR30:
		gbMemory[address] = data | 0x7f;
		if (!(data & 0x80))
		{
			gbMemory[NR52] &= 0xfb;
			sound3On		= 0;
		}
		break;
	case NR31:
		gbMemory[address] = 0xff;
		sound3ATL		  = sound3ATLreload = 172 * (256 - data);
		break;
	case NR32:
		gbMemory[address] = data | 0x9f;
		sound3OutputLevel = (data >> 5) & 3;
		break;
	case NR33:
		gbMemory[address] = 0xff;
		freq3low = data;
		freq	 = 2048 - (((int)(freq3high & 7) << 8) | freq3low);
		if (freq)
		{
			sound3Skip = SOUND_MAGIC_2 / freq;
		}
		else
			sound3Skip = 0;
		break;
	case NR34:
		gbMemory[address] = data | 0xbf;
		freq3high		  = data;
		freq = 2048 - (((int)(freq3high & 7) << 8) | freq3low);
		if (freq)
		{
			sound3Skip = SOUND_MAGIC_2 / freq;
		}
		else
		{
			sound3Skip = 0;
		}
		sound3Continue = data & 0x40;
		if ((data & 0x80) && (gbMemory[NR30] & 0x80))
		{
			gbMemory[NR52] |= 4;
			sound3ATL		= sound3ATLreload;
			sound3Index		= 0;
			sound3On		= 1;
		}
		break;
	case NR41:
		sound4ATL = sound4ATLreload = 172 * (64 - (data & 0x3f));
		break;
	case NR42:
		sound4EnvelopeVolume	= data >> 4;
		sound4EnvelopeUpDown	= data & 0x08;
		sound4EnvelopeATLReload = sound4EnvelopeATL = 689 * (data & 7);
		break;
	case NR43:
		freq		 = freq4 = soundFreqRatio[data & 7];
		sound4NSteps = data & 0x08;

		sound4Skip = (freq << 8) / NOISE_MAGIC;

		sound4Clock = data >> 4;

		freq = freq / soundShiftClock[sound4Clock];

		sound4ShiftSkip = (freq << 8) / NOISE_MAGIC;

		break;
	case NR44:
		gbMemory[address] = data | 0xbf;
		sound4Continue	  = data & 0x40;
		if (data & 0x80)
		{
			gbMemory[NR52]		|= 8;
			sound4EnvelopeVolume = gbMemory[NR42] >> 4;
			sound4EnvelopeUpDown = gbMemory[NR42] & 0x08;
			sound4ATL = sound4ATLreload;
			sound4EnvelopeATLReload = sound4EnvelopeATL = 689 * (gbMemory[NR42] & 7);

			sound4On = 1;

			sound4Index		 = 0;
			sound4ShiftIndex = 0;

			if (sound4NSteps)
				sound4ShiftRight = 0x7fff;
			else
				sound4ShiftRight = 0x7f;
		}
		break;
	case NR50:
		soundVIN	= data & 0x88;
		soundLevel1 = data & 7;
		soundLevel2 = (data >> 4) & 7;
		break;
	case NR51:
		soundBalance	  = (data & soundEnableFlag);
		gbMemory[address] = data; // added to fix bug with enabling/disabling sound channels
		break;
	case NR52:
		soundMasterOn = data & 0x80;
		if (!(data & 0x80))
		{
			sound1On = 0;
			sound2On = 0;
			sound3On = 0;
			sound4On = 0;
			gbMemory[address] &= 0xf0;
		}
		gbMemory[address] = data & 0x80 | 0x70 | (gbMemory[address] & 0xf);
		break;
	}

	gbDigitalSound = true;

	if (sound1On && sound1EnvelopeVolume != 0)
		gbDigitalSound = false;
	if (sound2On && sound2EnvelopeVolume != 0)
		gbDigitalSound = false;
	if (sound3On && sound3OutputLevel != 0)
		gbDigitalSound = false;
	if (sound4On && sound4EnvelopeVolume != 0)
		gbDigitalSound = false;
}

void gbSoundChannel1()
{
	int vol = sound1EnvelopeVolume;

	int freq = 0;

	int value = 0;

	if (sound1On && (sound1ATL || !sound1Continue))
	{
		sound1Index += soundQuality * sound1Skip;
		sound1Index &= 0x1fffffff;

		value = ((s8)sound1Wave[sound1Index >> 24]) * vol;
	}

	soundBuffer[0][soundIndex] = value;

	if (sound1On)
	{
		if (sound1ATL)
		{
			sound1ATL -= soundQuality;

			if (sound1ATL <= 0 && sound1Continue)
			{
				gbMemory[NR52] &= 0xfe;
				sound1On		= 0;
			}
		}

		if (sound1EnvelopeATL)
		{
			sound1EnvelopeATL -= soundQuality;

			if (sound1EnvelopeATL <= 0)
			{
				if (sound1EnvelopeUpDown)
				{
					if (sound1EnvelopeVolume < 15)
						sound1EnvelopeVolume++;
				}
				else
				{
					if (sound1EnvelopeVolume)
						sound1EnvelopeVolume--;
				}

				sound1EnvelopeATL += sound1EnvelopeATLReload;
			}
		}

		if (sound1SweepATL)
		{
			sound1SweepATL -= soundQuality;

			if (sound1SweepATL <= 0)
			{
				freq = (((int)(freq1high & 7)) << 8) | freq1low;

				int updown = 1;

				if (sound1SweepUpDown)
					updown = -1;

				int newfreq = 0;
				if (sound1SweepSteps)
				{
					newfreq = freq + updown * freq / (1 << sound1SweepSteps);
					if (newfreq == freq)
						newfreq = 0;
				}
				else
					newfreq = freq;

				if (newfreq < 0)
				{
					sound1SweepATL += sound1SweepATLReload;
				}
				else if (newfreq > 2047)
				{
					sound1SweepATL	= 0;
					sound1On		= 0;
					gbMemory[NR52] &= 0xfe;
				}
				else
				{
					sound1SweepATL += sound1SweepATLReload;
					sound1Skip		= SOUND_MAGIC / (2048 - newfreq);

					freq1low  = newfreq & 0xff;
					freq1high = (freq1high & 0xf8) | ((newfreq >> 8) & 7);
				}
			}
		}
	}
}

void gbSoundChannel2()
{
	//  int freq = 0;
	int vol = sound2EnvelopeVolume;

	int value = 0;

	if (sound2On && (sound2ATL || !sound2Continue))
	{
		sound2Index += soundQuality * sound2Skip;
		sound2Index &= 0x1fffffff;

		value = ((s8)sound2Wave[sound2Index >> 24]) * vol;
	}

	soundBuffer[1][soundIndex] = value;

	if (sound2On)
	{
		if (sound2ATL)
		{
			sound2ATL -= soundQuality;

			if (sound2ATL <= 0 && sound2Continue)
			{
				gbMemory[NR52] &= 0xfd;
				sound2On		= 0;
			}
		}

		if (sound2EnvelopeATL)
		{
			sound2EnvelopeATL -= soundQuality;

			if (sound2EnvelopeATL <= 0)
			{
				if (sound2EnvelopeUpDown)
				{
					if (sound2EnvelopeVolume < 15)
						sound2EnvelopeVolume++;
				}
				else
				{
					if (sound2EnvelopeVolume)
						sound2EnvelopeVolume--;
				}
				sound2EnvelopeATL += sound2EnvelopeATLReload;
			}
		}
	}
}

void gbSoundChannel3()
{
	int value = 0;

	if (sound3On && (sound3ATL || !sound3Continue))
	{
		value = sound3Last;

		sound3Index += soundQuality * sound3Skip;
		sound3Index &= 0x1fffffff;

		value = gbMemory[0xff30 + (sound3Index >> 25)];

		if ((sound3Index & 0x01000000))
		{
			value &= 0x0f;
		}
		else
		{
			value >>= 4;
		}

		value -= 8;
		value *= 2;

		switch (sound3OutputLevel)
		{
		case 0:
			value = 0;
			break;
		case 1:
			break;
		case 2:
			value = (value >> 1);
			break;
		case 3:
			value = (value >> 2);
			break;
		}
		//value += 1;
		sound3Last = value;
	}

	soundBuffer[2][soundIndex] = value;

	if (sound3On)
	{
		if (sound3ATL)
		{
			sound3ATL -= soundQuality;

			if (sound3ATL <= 0 && sound3Continue)
			{
				gbMemory[NR52] &= 0xfb;
				sound3On		= 0;
			}
		}
	}
}

void gbSoundChannel4()
{
	int vol = sound4EnvelopeVolume;

	int value = 0;

	if (sound4Clock <= 0x0c)
	{
		if (sound4On && (sound4ATL || !sound4Continue))
		{
			sound4Index		 += soundQuality * sound4Skip;
			sound4ShiftIndex += soundQuality * sound4ShiftSkip;

			if (sound4NSteps)
			{
				while (sound4ShiftIndex > 0x1fffff)
				{
					sound4ShiftRight = (((sound4ShiftRight << 6) ^
					                     (sound4ShiftRight << 5)) & 0x40) |
					                   (sound4ShiftRight >> 1);
					sound4ShiftIndex -= 0x200000;
				}
			}
			else
			{
				while (sound4ShiftIndex > 0x1fffff)
				{
					sound4ShiftRight = (((sound4ShiftRight << 14) ^
					                     (sound4ShiftRight << 13)) & 0x4000) |
					                   (sound4ShiftRight >> 1);

					sound4ShiftIndex -= 0x200000;
				}
			}

			sound4Index		 &= 0x1fffff;
			sound4ShiftIndex &= 0x1fffff;

			value = ((sound4ShiftRight & 1) * 2 - 1) * vol;
		}
		else
		{
			value = 0;
		}
	}

	soundBuffer[3][soundIndex] = value;

	if (sound4On)
	{
		if (sound4ATL)
		{
			sound4ATL -= soundQuality;

			if (sound4ATL <= 0 && sound4Continue)
			{
				gbMemory[NR52] &= 0xf7;
				sound4On		= 0;
			}
		}

		if (sound4EnvelopeATL)
		{
			sound4EnvelopeATL -= soundQuality;

			if (sound4EnvelopeATL <= 0)
			{
				if (sound4EnvelopeUpDown)
				{
					if (sound4EnvelopeVolume < 15)
						sound4EnvelopeVolume++;
				}
				else
				{
					if (sound4EnvelopeVolume)
						sound4EnvelopeVolume--;
				}
				sound4EnvelopeATL += sound4EnvelopeATLReload;
			}
		}
	}
}

void gbSoundMix()
{
	int res = 0;

	if (gbMemory)
		soundBalance = (gbMemory[NR51] & soundEnableFlag & ~soundMutedFlag);

	if (soundBalance & 16)
	{
		res += ((s8)soundBuffer[0][soundIndex]);
	}
	if (soundBalance & 32)
	{
		res += ((s8)soundBuffer[1][soundIndex]);
	}
	if (soundBalance & 64)
	{
		res += ((s8)soundBuffer[2][soundIndex]);
	}
	if (soundBalance & 128)
	{
		res += ((s8)soundBuffer[3][soundIndex]);
	}

	if (gbDigitalSound)
		res *= soundLevel1 * 256;
	else
		res *= soundLevel1 * 60;

	if (soundEcho)
	{
		res *= 2;
		res += soundFilter[soundEchoIndex];
		res /= 2;
		soundFilter[soundEchoIndex++] = res;
	}

	if (soundLowPass)
	{
		soundLeft[4] = soundLeft[3];
		soundLeft[3] = soundLeft[2];
		soundLeft[2] = soundLeft[1];
		soundLeft[1] = soundLeft[0];
		soundLeft[0] = res;
		res = (soundLeft[4] + 2 * soundLeft[3] + 8 * soundLeft[2] + 2 * soundLeft[1] +
		       soundLeft[0]) / 14;
	}

	switch (soundVolume)
	{
	case 0:
	case 1:
	case 2:
	case 3:
		res *= (soundVolume + 1);
		break;
	case 4:
		res >>= 2;
		break;
	case 5:
		res >>= 1;
		break;
	}

	if (res > 32767)
		res = 32767;
	if (res < -32768)
		res = -32768;

	if (soundReverse)
		soundFinalWave[++soundBufferIndex] = res;
	else
		soundFinalWave[soundBufferIndex++] = res;

	res = 0;

	if (soundBalance & 1)
	{
		res += ((s8)soundBuffer[0][soundIndex]);
	}
	if (soundBalance & 2)
	{
		res += ((s8)soundBuffer[1][soundIndex]);
	}
	if (soundBalance & 4)
	{
		res += ((s8)soundBuffer[2][soundIndex]);
	}
	if (soundBalance & 8)
	{
		res += ((s8)soundBuffer[3][soundIndex]);
	}

	if (gbDigitalSound)
		res *= soundLevel2 * 256;
	else
		res *= soundLevel2 * 60;

	if (soundEcho)
	{
		res *= 2;
		res += soundFilter[soundEchoIndex];
		res /= 2;
		soundFilter[soundEchoIndex++] = res;

		if (soundEchoIndex >= 4000)
			soundEchoIndex = 0;
	}

	if (soundLowPass)
	{
		soundRight[4] = soundRight[3];
		soundRight[3] = soundRight[2];
		soundRight[2] = soundRight[1];
		soundRight[1] = soundRight[0];
		soundRight[0] = res;
		res = (soundRight[4] + 2 * soundRight[3] + 8 * soundRight[2] + 2 * soundRight[1] +
		       soundRight[0]) / 14;
	}

	switch (soundVolume)
	{
	case 0:
	case 1:
	case 2:
	case 3:
		res *= (soundVolume + 1);
		break;
	case 4:
		res >>= 2;
		break;
	case 5:
		res >>= 1;
		break;
	}

	if (res > 32767)
		res = 32767;
	if (res < -32768)
		res = -32768;

	if (soundReverse)
		soundFinalWave[-1 + soundBufferIndex++] = res;
	else
		soundFinalWave[soundBufferIndex++] = res;
}

void gbSoundTick()
{
	if (systemSoundOn)
	{
		if (soundMasterOn)
		{
			gbSoundChannel1();
			gbSoundChannel2();
			gbSoundChannel3();
			gbSoundChannel4();

			gbSoundMix();
		}
		else
		{
			soundFinalWave[soundBufferIndex++] = 0;
			soundFinalWave[soundBufferIndex++] = 0;
		}

		soundIndex++;

		if (2 * soundBufferIndex >= soundBufferLen)
		{
			if (systemSoundOn)
			{
				if (soundPaused && !systemIsPaused())   // this checking is for the old frame timing
				{
					soundResume();
				}

				systemSoundWriteToBuffer();
			}
			soundIndex		 = 0;
			soundBufferIndex = 0;
		}
	}
}

void gbSoundReset()
{
	soundPaused		  = 1;
	soundPlay		  = 0;
	SOUND_CLOCK_TICKS = 2 * 24 * soundQuality;
//  soundTicks = SOUND_CLOCK_TICKS;
	soundTicks		  = 0;
	soundNextPosition = 0;
	soundMasterOn	  = 1;
	soundIndex		  = 0;
	soundBufferIndex  = 0;
	soundLevel1		  = 7;
	soundLevel2		  = 7;
	soundVIN = 0;

	sound1On	   = 0;
	sound1ATL	   = 0;
	sound1Skip	   = 0;
	sound1Index	   = 0;
	sound1Continue = 0;
	sound1EnvelopeVolume	=  0;
	sound1EnvelopeATL		= 0;
	sound1EnvelopeUpDown	= 0;
	sound1EnvelopeATLReload = 0;
	sound1SweepATL			= 0;
	sound1SweepATLReload	= 0;
	sound1SweepSteps		= 0;
	sound1SweepUpDown		= 0;
	sound1SweepStep			= 0;
	sound1Wave				= soundWavePattern[2];

	sound2On	   = 0;
	sound2ATL	   = 0;
	sound2Skip	   = 0;
	sound2Index	   = 0;
	sound2Continue = 0;
	sound2EnvelopeVolume	=  0;
	sound2EnvelopeATL		= 0;
	sound2EnvelopeUpDown	= 0;
	sound2EnvelopeATLReload = 0;
	sound2Wave				= soundWavePattern[2];

	sound3On = 0;
	sound3ATL		  = 0;
	sound3Skip		  = 0;
	sound3Index		  = 0;
	sound3Continue	  = 0;
	sound3OutputLevel = 0;

	sound4On	= 0;
	sound4Clock = 0;
	sound4ATL	= 0;
	sound4Skip	= 0;
	sound4Index = 0;
	sound4ShiftRight		= 0x7f;
	sound4NSteps			= 0;
	sound4CountDown			= 0;
	sound4Continue			= 0;
	sound4EnvelopeVolume	=  0;
	sound4EnvelopeATL		= 0;
	sound4EnvelopeUpDown	= 0;
	sound4EnvelopeATLReload = 0;

	// don't translate
	if (soundDebug)
	{
		log("*** Sound Init ***\n");
	}

	gbSoundEvent(0xff10, 0x80);
	gbSoundEvent(0xff11, 0xbf);
	gbSoundEvent(0xff12, 0xf3);
	gbSoundEvent(0xff14, 0xbf);
	gbSoundEvent(0xff16, 0x3f);
	gbSoundEvent(0xff17, 0x00);
	gbSoundEvent(0xff19, 0xbf);

	gbSoundEvent(0xff1a, 0x7f);
	gbSoundEvent(0xff1b, 0xff);
	gbSoundEvent(0xff1c, 0xbf);
	gbSoundEvent(0xff1e, 0xbf);

	gbSoundEvent(0xff20, 0xff);
	gbSoundEvent(0xff21, 0x00);
	gbSoundEvent(0xff22, 0x00);
	gbSoundEvent(0xff23, 0xbf);
	gbSoundEvent(0xff24, 0x77);
	gbSoundEvent(0xff25, 0xf3);

	if (gbHardware & 0x4)
		gbSoundEvent(0xff26, 0xf0);
	else
		gbSoundEvent(0xff26, 0xf1);

	/* workaround for game Beetlejuice */
	if (gbHardware & 0x1)
	{
		gbSoundEvent(0xff24, 0x77);
		gbSoundEvent(0xff25, 0xf3);
	}

	// don't translate
	if (soundDebug)
	{
		log("*** Sound Init Complete ***\n");
	}

	sound1On = 0;
	sound2On = 0;
	sound3On = 0;
	sound4On = 0;

	int addr = 0xff30;

	while (addr < 0xff40)
	{
		gbMemory[addr++] = 0x00;
		gbMemory[addr++] = 0xff;
	}

	memset(soundFinalWave, 0x00, soundBufferLen);

	memset(soundFilter, 0, sizeof(soundFilter));
	soundEchoIndex = 0;
}

extern bool soundInit();
extern void soundShutdown();

void gbSoundSetQuality(int quality)
{
	if (soundQuality != quality && systemSoundCanChangeQuality())
	{
		if (!soundOffFlag)
			soundShutdown();
		soundQuality	  = quality;
		soundNextPosition = 0;
		if (!soundOffFlag)
			soundInit();
		SOUND_CLOCK_TICKS = 2 * 24 * soundQuality;
		soundIndex		  = 0;
		soundBufferIndex  = 0;
	}
	else
	{
		soundNextPosition = 0;
		SOUND_CLOCK_TICKS = 2 * 24 * soundQuality;
		soundIndex		  = 0;
		soundBufferIndex  = 0;
	}
}

static variable_desc gbSoundSaveStruct[] = {
	{ &soundPaused,				sizeof(int32)				 },
	{ &soundPlay,				sizeof(int32)				 },
	{ &soundTicks,				sizeof(int32)				 },
//	{ &SOUND_CLOCK_TICKS,		sizeof(int32)				 },	// no longer needed
	{ &GB_SOUND_CLOCK_TICKS,	sizeof(int32)				 },
	{ &soundLevel1,				sizeof(int32)				 },
	{ &soundLevel2,				sizeof(int32)				 },
	{ &soundBalance,			sizeof(int32)				 },
	{ &soundMasterOn,			sizeof(int32)				 },
	{ &soundIndex,				sizeof(int32)				 },
	{ &soundVIN,				sizeof(int32)				 },
	{ &sound1On,				sizeof(int32)				 },
	{ &sound1ATL,				sizeof(int32)				 },
	{ &sound1Skip,				sizeof(int32)				 },
	{ &sound1Index,				sizeof(int32)				 },
	{ &sound1Continue,			sizeof(int32)				 },
	{ &sound1EnvelopeVolume,	sizeof(int32)				 },
	{ &sound1EnvelopeATL,		sizeof(int32)				 },
	{ &sound1EnvelopeATLReload, sizeof(int32)				 },
	{ &sound1EnvelopeUpDown,	sizeof(int32)				 },
	{ &sound1SweepATL,			sizeof(int32)				 },
	{ &sound1SweepATLReload,	sizeof(int32)				 },
	{ &sound1SweepSteps,		sizeof(int32)				 },
	{ &sound1SweepUpDown,		sizeof(int32)				 },
	{ &sound1SweepStep,			sizeof(int32)				 },
	{ &sound2On,				sizeof(int32)				 },
	{ &sound2ATL,				sizeof(int32)				 },
	{ &sound2Skip,				sizeof(int32)				 },
	{ &sound2Index,				sizeof(int32)				 },
	{ &sound2Continue,			sizeof(int32)				 },
	{ &sound2EnvelopeVolume,	sizeof(int32)				 },
	{ &sound2EnvelopeATL,		sizeof(int32)				 },
	{ &sound2EnvelopeATLReload, sizeof(int32)				 },
	{ &sound2EnvelopeUpDown,	sizeof(int32)				 },
	{ &sound3On,				sizeof(int32)				 },
	{ &sound3ATL,				sizeof(int32)				 },
	{ &sound3Skip,				sizeof(int32)				 },
	{ &sound3Index,				sizeof(int32)				 },
	{ &sound3Continue,			sizeof(int32)				 },
	{ &sound3OutputLevel,		sizeof(int32)				 },
	{ &sound4On,				sizeof(int32)				 },
	{ &sound4ATL,				sizeof(int32)				 },
	{ &sound4Skip,				sizeof(int32)				 },
	{ &sound4Index,				sizeof(int32)				 },
	{ &sound4Clock,				sizeof(int32)				 },
	{ &sound4ShiftRight,		sizeof(int32)				 },
	{ &sound4ShiftSkip,			sizeof(int32)				 },
	{ &sound4ShiftIndex,		sizeof(int32)				 },
	{ &sound4NSteps,			sizeof(int32)				 },
	{ &sound4CountDown,			sizeof(int32)				 },
	{ &sound4Continue,			sizeof(int32)				 },
	{ &sound4EnvelopeVolume,	sizeof(int32)				 },
	{ &sound4EnvelopeATL,		sizeof(int32)				 },
	{ &sound4EnvelopeATLReload, sizeof(int32)				 },
	{ &sound4EnvelopeUpDown,	sizeof(int32)				 },
	{ &soundEnableFlag,			sizeof(int32)				 },
	{ NULL,						0							 }
};

void gbSoundSaveGame(gzFile gzFile)
{
	GB_SOUND_CLOCK_TICKS = SOUND_CLOCK_TICKS / (gbSpeed ? 2 : 1);   // for backward compatibility
	utilWriteData(gzFile, gbSoundSaveStruct);

	utilWriteInt(gzFile, sound1ATLreload);
	utilWriteInt(gzFile, freq1low);
	utilWriteInt(gzFile, freq1high);
	utilWriteInt(gzFile, sound2ATLreload);
	utilWriteInt(gzFile, freq2low);
	utilWriteInt(gzFile, freq2high);
	utilWriteInt(gzFile, sound3ATLreload);
	utilWriteInt(gzFile, freq3low);
	utilWriteInt(gzFile, freq3high);
	utilWriteInt(gzFile, sound4ATLreload);
	utilWriteInt(gzFile, freq4);

	utilGzWrite(gzFile, soundBuffer, 4 * 735);
	utilGzWrite(gzFile, soundFinalWave, 2 * 735);
	utilGzWrite(gzFile, &soundQuality, sizeof(int));
}

void gbSoundReadGame(int version, gzFile gzFile)
{
	int32 oldSoundPaused = soundPaused;
	int32 oldSoundEnableFlag = soundEnableFlag;
	utilReadData(gzFile, gbSoundSaveStruct);
	soundPaused = oldSoundPaused;
	soundEnableFlag = oldSoundEnableFlag;

	if (version < 11)
	{
		sound1ATLreload = 172 * (64 - (gbMemory[NR11] & 0x3f));
		freq1low		= gbMemory[NR13];
		freq1high		= gbMemory[NR14] & 7;
		sound2ATLreload = 172 * (64 - (gbMemory[NR21] & 0x3f));
		freq2low		= gbMemory[NR23];
		freq2high		= gbMemory[NR24] & 7;
		sound3ATLreload = 172 * (256 - gbMemory[NR31]);
		freq3low		= gbMemory[NR33];
		freq3high		= gbMemory[NR34] & 7;
		sound4ATLreload = 172 * (64 - (gbMemory[NR41] & 0x3f));
		freq4 = soundFreqRatio[gbMemory[NR43] & 7];
	}
	else
	{
		sound1ATLreload = utilReadInt(gzFile);
		freq1low		= utilReadInt(gzFile);
		freq1high		= utilReadInt(gzFile);
		sound2ATLreload = utilReadInt(gzFile);
		freq2low		= utilReadInt(gzFile);
		freq2high		= utilReadInt(gzFile);
		sound3ATLreload = utilReadInt(gzFile);
		freq3low		= utilReadInt(gzFile);
		freq3high		= utilReadInt(gzFile);
		sound4ATLreload = utilReadInt(gzFile);
		freq4 = utilReadInt(gzFile);
	}

	soundBufferIndex = soundIndex * 2;

	utilGzRead(gzFile, soundBuffer, 4 * 735);
	utilGzRead(gzFile, soundFinalWave, 2 * 735);

	if (version >= 7)
	{
		int quality = 1;
		utilGzRead(gzFile, &quality, sizeof(int));
		gbSoundSetQuality(quality);
	}
	else
	{
		soundQuality = -1;
		gbSoundSetQuality(1);
	}

	sound1Wave = soundWavePattern[gbMemory[NR11] >> 6];
	sound2Wave = soundWavePattern[gbMemory[NR21] >> 6];
}

