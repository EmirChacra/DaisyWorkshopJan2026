#include "daisy_pod.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisyPod hw;
daisy::Parameter pknob_1, pknob_2;

float sample_rate;

//Objetos para los efectos
daisysp::DelayLine<float, 65536> DSY_SDRAM_BSS delayBuffer;
daisysp::Oscillator chorusMod;
daisysp::OnePole delayFilter;

//Variables numéricas, parámetros para los efectos
float delayTime;
float new_delayTime;
float delayFeedback;
float delayFilterFreq;

bool chorusOn = false;
bool doubleTime = false;

float mix;
int dmix; //Para cambiar el mix con encoder.

void Controls();

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
	float input;
	float delayOutput;

	Controls();

	//daisysp::fonepole(delayTime, new_delayTime, .00007f);
	delayFilter.SetFrequency(delayFilterFreq);

	for (size_t i = 0; i < size; i++)
	{
		input = 0.5 * (in[0][i] + in[1][i]);

		//Leemos, procesamos y escribimos el buffer del delay.
		delayOutput = delayBuffer.ReadHermite(new_delayTime + 480 * chorusOn * (0.5 * chorusMod.Process() + 1));
		//delayOutput = tanh(1.1f * delayOutput);
		delayOutput = delayFilter.Process(delayOutput);

		delayBuffer.Write(input + delayFeedback * delayOutput);

		out[0][i] = (1 - mix) * input + mix * delayOutput;
		out[1][i] = (1 - mix) * input + mix * delayOutput;
	}
}

int main(void)
{
	hw.Init();
	hw.SetAudioBlockSize(4); // number of samples handled per callback
	hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
	sample_rate = 48000.0f;

	pknob_1.Init(hw.knob1,  0.01 * sample_rate, sample_rate, daisy::Parameter::EXPONENTIAL);
	pknob_2.Init(hw.knob2, 0.0f, 1.0f, daisy::Parameter::LINEAR);

	delayBuffer.Init();

	delayFilter.Init(); //Inicializa como LPF por default
	//delayFilter.SetFilterMode(daisysp::OnePole::FilterMode::FILTER_MODE_HIGH_PASS);

	chorusMod.Init(sample_rate);
	chorusMod.SetWaveform(Oscillator::WAVE_SIN);
	chorusMod.SetFreq(0.1); //Frequencia en Hz

	//Inicialización de parámetros
	mix = 0.5f;
	dmix = 25;

	hw.StartAdc();
	hw.StartAudio(AudioCallback);

	while(1) {}
}

void UpdateButtons()
{
	//Turn chorus modulation on/off
	if(hw.button1.RisingEdge())
	{
		chorusOn = !chorusOn;
	}	 

	if(hw.button2.Pressed())
	{
	}	

}

void UpdateKnobs()
{
	//Tiempo con knob 1.
	new_delayTime = pknob_1.Process();
	
	//Feedback con knob 2.
	delayFeedback = pknob_2.Process();
	delayFilterFreq = 0.05f + 0.45f * (1 - delayFeedback * delayFeedback); //Mientras más tiempo más intenso el filtro
}

void UpdateEncoder()
{
	//Change mix with encoder, maybe should be smooothed
	dmix = dmix + hw.encoder.Increment();
    if (dmix < 0) dmix = 0;
	if (dmix > 50) dmix = 50;

	mix = (float)(dmix/50.0f);
}

void UpdateLeds()
{	
	//Indicador de chorus
	if (chorusOn)
	{
		hw.led1.Set(0.7f, 0.0f, 0.7f); // Morado
	}
	else
	{
		hw.led1.Set(0.0, 0.7f, 0.7f); // Celeste
	}

	//Indicador de mix
	if (dmix == 25)
	{
		hw.led2.Set(0.7 * mix, 0.0 , 0.7 * mix);
	}
	else
	{
		hw.led2.Set(0.0, 0.0, 0.7 * mix);
	}

	hw.UpdateLeds();
}

void Controls()
{
    hw.ProcessDigitalControls();
    hw.ProcessAnalogControls();

    UpdateButtons();

	UpdateKnobs();

	UpdateEncoder();

    UpdateLeds();
}
