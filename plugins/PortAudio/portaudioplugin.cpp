#include "portaudioplugin.h"

#include <QtPlugin>

#if defined( PORTAUDIO_SUPPORTED )
#include <portaudio.h>
#endif

#include "portaudiooutputnode.h"
#include "portaudioinputnode.h"

#include "deviceportaudio.h"

QList<QUuid>	NodeControlBase::PID_UUID;

ClassEntry		PortAudioPlugin::mNodeClasses[] =
{
	ClassEntry( QT_TR_NOOP( "Audio Input" ),	"PortAudio", NID_PORT_AUDIO_INPUT, &PortAudioInputNode::staticMetaObject ),
	ClassEntry( QT_TR_NOOP( "Audio Output" ),	"PortAudio", NID_PORT_AUDIO_OUTPUT, &PortAudioOutputNode::staticMetaObject ),
	ClassEntry()
};

ClassEntry		PortAudioPlugin::mPinClasses[] =
{
	ClassEntry()
};

PortAudioPlugin	*PortAudioPlugin::mInstance = nullptr;

PortAudioPlugin::PortAudioPlugin( void )
	: mApp( 0 )
{
	mInstance = this;
}

PortAudioPlugin::~PortAudioPlugin()
{
	mInstance = 0;
}

PluginInterface::InitResult PortAudioPlugin::initialise( fugio::GlobalInterface *pApp )
{
	mApp = pApp;

#if defined( PORTAUDIO_SUPPORTED )
	if( Pa_Initialize() != paNoError )
	{
		return( INIT_FAILED );
	}

	qInfo() << Pa_GetVersionText();
#endif

	DevicePortAudio::deviceInitialise();

	mApp->registerNodeClasses( mNodeClasses );

	mApp->registerPinClasses( mPinClasses );

	return( INIT_OK );
}

void PortAudioPlugin::deinitialise()
{
	mApp->unregisterNodeClasses( mNodeClasses );

	mApp->unregisterPinClasses( mPinClasses );

	DevicePortAudio::deviceDeinitialise();

#if defined( PORTAUDIO_SUPPORTED )
	Pa_Terminate();
#endif
}

void PortAudioPlugin::deviceConfigGui( QWidget *pParent )
{
	Q_UNUSED( pParent )
}