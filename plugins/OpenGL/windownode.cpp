#include "windownode.h"

#include <QtGlobal>

#include <QMatrix4x4>
#include <QMouseEvent>
#include <QApplication>
#include <QSettings>
#include <QDateTime>
#include <QAction>
#include <QStandardPaths>
#include <QDir>

#include <fugio/global_interface.h>
#include <fugio/core/variant_interface.h>
#include <fugio/context_interface.h>
#include <fugio/opengl/texture_interface.h>
#include <fugio/core/uuid.h>
#include <fugio/image/uuid.h>

#include <fugio/render_interface.h>
#include <fugio/opengl/state_interface.h>
#include <fugio/performance.h>

#include <fugio/context_signals.h>

#include "openglplugin.h"

WindowNode::WindowNode( QSharedPointer<fugio::NodeInterface> pNode )
	: NodeControlBase( pNode ), mTimeStamp( 0 ), mGrabScreenId( 0 ), mUpdate( true )
{
	FUGID( PIN_INPUT_HIDE_CURSOR, "1b5e9ce8-acb9-478d-b84b-9288ab3c42f5" );
	FUGID( PIN_INPUT_STATE, "261cc653-d7fa-4c34-a08b-3603e8ae71d5" );
	FUGID( PIN_INPUT_RENDER, "249f2932-f483-422f-b811-ab679f006381" );
	FUGID( PIN_OUTPUT_WINDOW_SIZE, "e6bf944e-5f46-4994-bd51-13c2aa6415b7" );
	FUGID( PIN_OUTPUT_EVENTS, "524e9f30-7094-4f87-b5ab-ead2da04256b" );

	mPinInputTrigger = pinInput( "Trigger", PID_FUGIO_NODE_TRIGGER );

	mPinInputRender = pinInput( "Render", PIN_INPUT_RENDER );

	mPinMouseShowCursor = pinInput( "Hide Cursor", PIN_INPUT_HIDE_CURSOR );

	mPinState = pinInput( "State", PIN_INPUT_STATE );

	mValWindowSize = pinOutput<VariantInterface *>( "Window Size", mPinWindowSize, PID_SIZE, PIN_OUTPUT_WINDOW_SIZE );

	mValOutputEvents = pinOutput<InputEventsInterface *>( "Events", mPinOutputEvents, PID_INPUT_EVENTS, PIN_OUTPUT_EVENTS );

	mPinMouseShowCursor->setDescription( tr( "The mouse cursor will be visible in the OpenGL window if this is true, hidden if it is false" ) );

	mPinState->setDescription( tr( "The OpenGL rendering state to apply" ) );

	mPinInputRender->setDescription( tr( "The input 3D Geometry to render" ) );
}

WindowNode::~WindowNode( void )
{
//	deinitialise();
}

bool WindowNode::initialise()
{
	if( !NodeControlBase::initialise() )
	{
		return( false );
	}

	if( !mOutput )
	{
		if( ( mOutput = DeviceOpenGLOutput::newDevice() ) == nullptr )
		{
			return( false );
		}

		mOutput->setCurrentNode( mNode );

		mOutput->setInputEventsInterface( mValOutputEvents );

		outputResized( mOutput->size() );

		connect( mOutput.data(), SIGNAL(resized(QSize)), this, SLOT(outputResized(QSize)) );
	}

	if( !OpenGLPlugin::hasContextStatic() )
	{
		return( false );
	}

#if !defined( GL_ES_VERSION_2_0 )
	if( !mOutput->isValid() )
	{
		return( false );
	}
#endif

	mNode->context()->nodeInitialised();

	connect( mNode->context()->qobject(), SIGNAL(frameInitialise()), this, SLOT(contextFrameInitialise()) );
	connect( mNode->context()->qobject(), SIGNAL(frameEnd()), this, SLOT(contextFrameEnd()) );

#if defined( USE_TEXTURE_PIN )
	// set texture settings

	mTexture->setFilter( GL_LINEAR, GL_LINEAR );
	mTexture->setFormat( GL_RGBA );
	mTexture->setGenMipMaps( false );
#if !defined( GL_ES_VERSION_2_0 )
	mTexture->setInternalFormat( GL_RGBA8 );
#else
	mTexture->setInternalFormat( GL_RGBA );
#endif
	mTexture->setTarget( GL_TEXTURE_2D );
	mTexture->setType( QOpenGLTexture::UInt8 );
	mTexture->setWrap( GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE );
#endif

	return( true );
}

bool WindowNode::deinitialise()
{
	if( mNode->context() )
	{
		mNode->context()->qobject()->disconnect( this );
	}

	if( mOutput )
	{
		mOutput->disconnect( this );

		mOutput->unsetCurrentNode( mNode );

		mOutput.clear();
	}

	return( NodeControlBase::deinitialise() );
}

void WindowNode::inputsUpdated( qint64 pTimeStamp )
{
	mTimeStamp = pTimeStamp;

	if( ( mPinInputTrigger->isUpdated( pTimeStamp ) || mPinInputRender->isUpdated( pTimeStamp ) ) && mOutput )
	{
		mOutput->renderLater();
	}
}

void WindowNode::screengrab( const QString &pFileName )
{
	if( pFileName.isEmpty() )
	{
		QString		ImgDir = QStandardPaths::writableLocation( QStandardPaths::PicturesLocation );
		QString		OutNam = "Fugio Screengrabs";
		QString		OutDir = QString( "%1%2%3" ).arg( ImgDir ).arg( QDir::separator() ).arg( OutNam );

		if( QDir( OutDir ).exists() || QDir( ImgDir ).mkdir( OutNam ) )
		{
			ImgDir = OutDir;
		}

		mGrabFileNames.append( QString( "%1%2%3.%4.png" ).arg( ImgDir ).arg( QDir::separator() ).arg( QDate::currentDate().toString( "yyyy-MM-dd" ) ).arg( QTime::currentTime().toString( "hh-mm-ss" ) ) );
	}
	else
	{
		mGrabFileNames.append( pFileName );
	}
}

void WindowNode::resize( int pWidth, int pHeight )
{
	QSize			NewSze( pWidth, pHeight );

	if( NewSze != mValWindowSize->variant().toSize() )
	{
		mValWindowSize->setVariant( NewSze );

		pinUpdated( mPinWindowSize );
	}
}

bool WindowNode::showCursor() const
{
	VariantInterface		*V;

	if( mPinMouseShowCursor->isConnected() && ( V = qobject_cast<VariantInterface *>( mPinMouseShowCursor->connectedPin()->control()->qobject() ) ) != 0 )
	{
		return( V->variant().toBool() );
	}

	return( mPinMouseShowCursor->value().toInt() != 0 );
}

void WindowNode::render( qint64 pTimeStamp )
{
	fugio::Performance		Perf( mNode, "Render", pTimeStamp );

	if( !mOutput->renderInit() )
	{
		return;
	}

	if( !OpenGLPlugin::hasContextStatic() )
	{
		return;
	}

	initializeOpenGLFunctions();

//	const int OutputSamples = mOutput->context()->format().samples();

	OPENGL_DEBUG( mNode->name() );

	if( !mNode->isInitialised() )
	{
		return;
	}

	// We need to keep a reference to ourselves here as OpenGL can
	// call the main app event loop, which can close the context and delete us!

	QSharedPointer<NodeControlInterface>	C = mNode->control();

	mUpdate = false;

	// Set the window size and render size
	// Note we currently can't scale the output if the samples > 1

	const QSize		WindowSize = mOutput->framebufferSize();

	OPENGL_DEBUG( mNode->name() );

#if defined( USE_TEXTURE_PIN )
	if( mTexture->textureSize() != QVector3D( WindowSize.width(), WindowSize.height(), 0 ) )
	{
		mTexture->free();

		mTexture->setSize( WindowSize.width(), WindowSize.height() );
	}

	const bool CaptureWindow = ( !mGrabFileNames.isEmpty() || mPinTexture->isConnectedToActiveNode() );
#endif

	GLuint		FBOId   = 0;

#if defined( USE_TEXTURE_PIN )
	const bool	CaptureWindow = !mGrabFileNames.isEmpty();

	const int	TextureSamples = qMax( OutputSamples, 1 );

	if( CaptureWindow )
	{
		mTexture->update( 0, 0 );

#if !defined( GL_ES_VERSION_2_0 )
		if( TextureSamples > 1 )
		{
			FBOId = mTexture->fboMultiSample( TextureSamples, true );
		}
		else
#endif
		{
			FBOId = mTexture->fbo( true );
		}
	}
#endif

	if( FBOId )
	{
		glBindFramebuffer( GL_FRAMEBUFFER, FBOId );
	}

	glViewport( 0, 0, WindowSize.width(), WindowSize.height() );

	glClearColor( 0.0, 0.0, 0.0, 0.0 );

	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );

	OPENGL_DEBUG( mNode->name() );

	mOutput->renderStart();

	OPENGL_DEBUG( mNode->name() );

	OpenGLStateInterface		*State = input<OpenGLStateInterface *>( mPinState );

	if( State )
	{
		State->stateBegin();

		OPENGL_DEBUG( mNode->name() );
	}

	fugio::RenderInterface		*Render = input<fugio::RenderInterface *>( mPinInputRender );

	if( Render )
	{
		Render->render( pTimeStamp );

		OPENGL_DEBUG( mNode->name() );
	}

	if( State )
	{
		State->stateEnd();
	}

	OPENGL_DEBUG( mNode->name() );

	/*
	if( FBOId )
	{
#if !defined( GL_ES_VERSION_2_0 )
		glBindFramebuffer( GL_FRAMEBUFFER, 0 );

		OPENGL_DEBUG( mNode->name() );
#endif

		if( CaptureWindow )
		{
#if !defined( GL_ES_VERSION_2_0 )
			if( TextureSamples > 1 )
			{
				glBindFramebuffer( GL_READ_FRAMEBUFFER, FBOId );
				glBindFramebuffer( GL_DRAW_FRAMEBUFFER, mTexture->fbo() );

				glBlitFramebuffer( 0, 0, WindowSize.width(), WindowSize.height(), 0, 0, WindowSize.width(), WindowSize.height(), GL_COLOR_BUFFER_BIT, GL_NEAREST );

				glBindFramebuffer( GL_FRAMEBUFFER, 0 );

				OPENGL_DEBUG( mNode->name() );
			}

			glBindFramebuffer( GL_READ_FRAMEBUFFER, mTexture->fbo() );
			glBindFramebuffer( GL_DRAW_FRAMEBUFFER, 0 );

			glBlitFramebuffer( 0, 0, WindowSize.width(), WindowSize.height(), 0, 0, WindowSize.width(), WindowSize.height(), GL_COLOR_BUFFER_BIT, GL_NEAREST );

			glBindFramebuffer( GL_FRAMEBUFFER, 0 );

			OPENGL_DEBUG( mNode->name() );
#endif
			pinUpdated( mPinTexture );
		}
		else
		{
			// Our WindowSize is the same as our window size so we can blit straight from the MS FBO

#if !defined( GL_ES_VERSION_2_0 )
			glBindFramebuffer( GL_READ_FRAMEBUFFER, FBOId );
			glBindFramebuffer( GL_DRAW_FRAMEBUFFER, 0 );

			glBlitFramebuffer( 0, 0, WindowSize.width(), WindowSize.height(), 0, 0, WindowSize.width(), WindowSize.height(), GL_COLOR_BUFFER_BIT, GL_NEAREST );

			glBindFramebuffer( GL_FRAMEBUFFER, 0 );

			OPENGL_DEBUG( mNode->name() );
#endif
		}
	}
*/

	if( !mOutput )
	{
		return;
	}

	mOutput->renderEnd();

	OPENGL_DEBUG( mNode->name() );

/*
	if( mTexture->fbo() && !mGrabFileNames.isEmpty() )
	{
		glBindFramebuffer( GL_FRAMEBUFFER, mTexture->fbo() );

		if( mBufferImage.width() != WindowSize.width() || mBufferImage.height() != WindowSize.height() )
		{
			mBufferImage = QImage( WindowSize, QImage::Format_ARGB32 );
		}

#if !defined( GL_ES_VERSION_2_0 )
		glReadPixels( 0, 0, WindowSize.width(), WindowSize.height(), GL_BGRA, GL_UNSIGNED_BYTE, mBufferImage.bits() );
#else
		glReadPixels( 0, 0, WindowSize.width(), WindowSize.height(), GL_RGBA, GL_UNSIGNED_BYTE, mBufferImage.bits() );
#endif

		glBindFramebuffer( GL_FRAMEBUFFER, 0 );

		if( !mGrabFileNames.isEmpty() )
		{
			mBufferImage = mBufferImage.mirrored();

			if( mBufferImage.format() == QImage::Format_ARGB32_Premultiplied )
			{
				QImage		TmpImg( mBufferImage.constBits(), mBufferImage.width(), mBufferImage.height(), QImage::Format_ARGB32 );

				for( const QString &FN : mGrabFileNames )
				{
					TmpImg.save( FN, 0, 100 );
				}
			}
			else
			{
				for( const QString &FN : mGrabFileNames )
				{
					mBufferImage.save( FN, 0, 100 );
				}
			}

			mGrabFileNames.clear();
		}
	}
*/

	if( !mGrabFileNames.isEmpty() )
	{
		mBufferImage = mOutput->grabFramebuffer();

		if( mBufferImage.format() == QImage::Format_ARGB32_Premultiplied )
		{
			QImage		TmpImg( mBufferImage.constBits(), mBufferImage.width(), mBufferImage.height(), QImage::Format_ARGB32 );

			for( const QString &FN : mGrabFileNames )
			{
				TmpImg.save( FN, 0, 100 );
			}
		}
		else
		{
			for( const QString &FN : mGrabFileNames )
			{
				mBufferImage.save( FN, 0, 100 );
			}
		}

		mGrabFileNames.clear();
	}

	OPENGL_DEBUG( mNode->name() );
}

void WindowNode::contextFrameInitialise()
{
	if( mValOutputEvents )
	{
		mValOutputEvents->inputFrameInitialise();
	}
}

void WindowNode::contextFrameEnd()
{
	if( mOutput && mOutput->isUpdatePending() )
	{
		mOutput->renderLater();
	}
}

void WindowNode::outputResized( QSize Size )
{
	Size = mOutput->framebufferSize();

	if( Size != mValWindowSize->variant().toSize() )
	{
		mValWindowSize->setVariant( Size );

		pinUpdated( mPinWindowSize );
	}
}
