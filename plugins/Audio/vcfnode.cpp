#include "vcfnode.h"

#include <fugio/core/uuid.h>
#include <fugio/audio/uuid.h>

#include <fugio/context_interface.h>
#include <fugio/context_signals.h>
#include <fugio/pin_signals.h>

#include <qmath.h>
#include <cmath>

VCFNode::VCFNode( QSharedPointer<fugio::NodeInterface> pNode )
	: NodeControlBase( pNode ), mCutoff( 1 ), mResonance( 0 )
{
	mPinAudioInput = pinInput( "Audio" );
	mPinCutoff     = pinInput( "Cutoff" );
	mPinResonance  = pinInput( "Resonance" );

	mAudioOutput = pinOutput<fugio::AudioProducerInterface *>( "Audio", mPinAudioOutput, PID_AUDIO );
}

bool VCFNode::initialise()
{
	if( !NodeControlBase::initialise() )
	{
		return( false );
	}

	connect( mPinCutoff->qobject(), SIGNAL(linked(QSharedPointer<fugio::PinInterface>)), this, SLOT(cutoffLinked(QSharedPointer<fugio::PinInterface>)) );
	connect( mPinCutoff->qobject(), SIGNAL(unlinked(QSharedPointer<fugio::PinInterface>)), this, SLOT(cutoffUnlinked(QSharedPointer<fugio::PinInterface>)) );

	connect( mPinResonance->qobject(), SIGNAL(linked(QSharedPointer<fugio::PinInterface>)), this, SLOT(resonanceLinked(QSharedPointer<fugio::PinInterface>)) );
	connect( mPinResonance->qobject(), SIGNAL(unlinked(QSharedPointer<fugio::PinInterface>)), this, SLOT(resonanceUnlinked(QSharedPointer<fugio::PinInterface>)) );

	return( true );
}

bool VCFNode::deinitialise()
{
	disconnect( mPinCutoff->qobject(), SIGNAL(linked(QSharedPointer<fugio::PinInterface>)), this, SLOT(cutoffLinked(QSharedPointer<fugio::PinInterface>)) );
	disconnect( mPinCutoff->qobject(), SIGNAL(unlinked(QSharedPointer<fugio::PinInterface>)), this, SLOT(cutoffUnlinked(QSharedPointer<fugio::PinInterface>)) );

	disconnect( mPinResonance->qobject(), SIGNAL(linked(QSharedPointer<fugio::PinInterface>)), this, SLOT(resonanceLinked(QSharedPointer<fugio::PinInterface>)) );
	disconnect( mPinResonance->qobject(), SIGNAL(unlinked(QSharedPointer<fugio::PinInterface>)), this, SLOT(resonanceUnlinked(QSharedPointer<fugio::PinInterface>)) );

	return( NodeControlBase::deinitialise() );
}

void VCFNode::inputsUpdated( qint64 pTimeStamp )
{
	NodeControlBase::inputsUpdated( pTimeStamp );

	mCutoff = variant( mPinCutoff ).toFloat();
	mResonance = variant( mPinResonance ).toFloat();
}

void *VCFNode::allocAudioInstance( qreal pSampleRate, fugio::AudioSampleFormat pSampleFormat, int pChannels )
{
	AudioInstanceData		*InsDat = new AudioInstanceData();

	if( InsDat )
	{
		InsDat->mSampleRate   = pSampleRate;
		InsDat->mSampleFormat = pSampleFormat;
		InsDat->mChannels     = pChannels;

		InsDat->mAudIns = 0;
		InsDat->mAudPts = 0;
		InsDat->mAudSmp = 0;

		InsDat->mCutoffInstance = 0;

		InsDat->mResonanceInstance = 0;

		InsDat->mAudDat.resize( pChannels );

		AudioChannelData		ACD;

		memset( &ACD, 0, sizeof( ACD ) );

		for( int i = 0 ; i < pChannels ; i++ )
		{
			InsDat->mChnDat << ACD;
		}

		if( fugio::AudioProducerInterface *IAP = input<fugio::AudioProducerInterface *>( mPinAudioInput ) )
		{
			InsDat->mAudIns = IAP->allocAudioInstance( pSampleRate, pSampleFormat, pChannels );
		}

		if( fugio::AudioProducerInterface *IAP = input<fugio::AudioProducerInterface *>( mPinCutoff ) )
		{
			InsDat->mCutoffInstance = IAP->allocAudioInstance( pSampleRate, pSampleFormat, 1 );
		}

		if( fugio::AudioProducerInterface *IAP = input<fugio::AudioProducerInterface *>( mPinResonance ) )
		{
			InsDat->mResonanceInstance = IAP->allocAudioInstance( pSampleRate, pSampleFormat, 1 );
		}

		mInstanceDataMutex.lock();

		mInstanceData.append( InsDat );

		mInstanceDataMutex.unlock();
	}

	return( InsDat );
}

void VCFNode::freeAudioInstance( void *pInstanceData )
{
	AudioInstanceData		*InsDat = static_cast<AudioInstanceData *>( pInstanceData );

	if( InsDat )
	{
		mInstanceDataMutex.lock();

		mInstanceData.removeAll( InsDat );

		mInstanceDataMutex.unlock();

		if( fugio::AudioProducerInterface *IAP = input<fugio::AudioProducerInterface *>( mPinAudioInput ) )
		{
			IAP->freeAudioInstance( InsDat->mAudIns );

			InsDat->mAudIns = nullptr;
		}

		if( fugio::AudioProducerInterface *IAP = input<fugio::AudioProducerInterface *>( mPinCutoff ) )
		{
			IAP->freeAudioInstance( InsDat->mCutoffInstance );

			InsDat->mCutoffInstance = nullptr;
		}

		if( fugio::AudioProducerInterface *IAP = input<fugio::AudioProducerInterface *>( mPinResonance ) )
		{
			IAP->freeAudioInstance( InsDat->mResonanceInstance );

			InsDat->mCutoffInstance = nullptr;
		}

		InsDat->mAudDat.clear();

		delete InsDat;
	}
}

void VCFNode::cutoffLinked( QSharedPointer<fugio::PinInterface> pPin )
{
	if( !pPin->hasControl() )
	{
		return;
	}

	fugio::AudioProducerInterface *IAP = qobject_cast<fugio::AudioProducerInterface *>( pPin->control()->qobject() );

	if( !IAP )
	{
		return;
	}

	QMutexLocker	MutLck( &mInstanceDataMutex );

	for( AudioInstanceData *AID : mInstanceData )
	{
		AID->mCutoffInstance = IAP->allocAudioInstance( AID->mSampleRate, AID->mSampleFormat, 1 );
	}
}

void VCFNode::cutoffUnlinked(QSharedPointer<fugio::PinInterface> pPin)
{
	if( !pPin->hasControl() )
	{
		return;
	}

	fugio::AudioProducerInterface *IAP = qobject_cast<fugio::AudioProducerInterface *>( pPin->control()->qobject() );

	if( !IAP )
	{
		return;
	}

	QMutexLocker	MutLck( &mInstanceDataMutex );

	for( AudioInstanceData *AID : mInstanceData )
	{
		IAP->freeAudioInstance( AID->mCutoffInstance );

		AID->mCutoffInstance = nullptr;
	}
}

void VCFNode::resonanceLinked( QSharedPointer<fugio::PinInterface> pPin )
{
	if( !pPin->hasControl() )
	{
		return;
	}

	fugio::AudioProducerInterface *IAP = qobject_cast<fugio::AudioProducerInterface *>( pPin->control()->qobject() );

	if( !IAP )
	{
		return;
	}

	QMutexLocker	MutLck( &mInstanceDataMutex );

	for( AudioInstanceData *AID : mInstanceData )
	{
		AID->mResonanceInstance = IAP->allocAudioInstance( AID->mSampleRate, AID->mSampleFormat, 1 );
	}
}

void VCFNode::resonanceUnlinked(QSharedPointer<fugio::PinInterface> pPin)
{
	if( !pPin->hasControl() )
	{
		return;
	}

	fugio::AudioProducerInterface *IAP = qobject_cast<fugio::AudioProducerInterface *>( pPin->control()->qobject() );

	if( !IAP )
	{
		return;
	}

	QMutexLocker	MutLck( &mInstanceDataMutex );

	for( AudioInstanceData *AID : mInstanceData )
	{
		IAP->freeAudioInstance( AID->mResonanceInstance );

		AID->mResonanceInstance = nullptr;
	}
}

void VCFNode::updateFilter(const float cutoff, const float res, float &p, float &k, float &r )
{
	p = cutoff * ( 1.8f - 0.8f * cutoff );
	k = 2.0f * qSin( cutoff * M_PI * 0.5f ) - 1.0f;

	float		t1 = ( 1.0f - p ) * 1.386249f;
	float		t2 = 12.0f + t1 * t1;

	r  = res * ( t2 + 6.0f * t1 ) / ( t2 - 6.0f * t1 );
}

void VCFNode::audio( qint64 pSamplePosition, qint64 pSampleCount, int pChannelOffset, int pChannelCount, float **pBuffers, qint64 pLatency, void *pInstanceData ) const
{
	AudioInstanceData		*InsDat = static_cast<AudioInstanceData *>( pInstanceData );

	if( fugio::AudioProducerInterface *IAP = input<fugio::AudioProducerInterface *>( mPinAudioInput ) )
	{
		IAP->audio( pSamplePosition, pSampleCount, pChannelOffset, pChannelCount, pBuffers, pLatency, InsDat->mAudIns );
	}
	else
	{
		return;
	}

	if( fugio::AudioProducerInterface *IAP = input<fugio::AudioProducerInterface *>( mPinCutoff ) )
	{
		InsDat->mCutoffData.resize( pSampleCount );

		float		*SmpPtr = InsDat->mCutoffData.data();

		memset( SmpPtr, 0, sizeof( float ) * pSampleCount );

		IAP->audio( pSamplePosition, pSampleCount, 0, 1, &SmpPtr, pLatency, InsDat->mCutoffInstance );
	}
	else
	{
		InsDat->mCutoffData.clear();
	}

	if( fugio::AudioProducerInterface *IAP = input<fugio::AudioProducerInterface *>( mPinResonance ) )
	{
		InsDat->mResonanceData.resize( pSampleCount );

		float		*SmpPtr = InsDat->mResonanceData.data();

		memset( SmpPtr, 0, sizeof( float ) * pSampleCount );

		IAP->audio( pSamplePosition, pSampleCount, 0, 1, &SmpPtr, pLatency, InsDat->mResonanceInstance );
	}
	else
	{
		InsDat->mResonanceData.clear();
	}

	// http://musicdsp.org/showArchiveComment.php?ArchiveID=24

	float		cutoff = qBound( 0.0f, mCutoff, 1.0f );
	float		res    = qBound( 0.0f, mResonance, 1.0f );

	float		p, k, r;

	updateFilter( cutoff, res, p, k, r );

	for( int c = pChannelOffset ; c < pChannelCount ; c++ )
	{
		AudioChannelData	&ACD = InsDat->mChnDat[ c ];

		float				*AudDst = pBuffers[ c ];

		for( qint64 i = 0 ; i < pSampleCount ; i++ )
		{
			if( !InsDat->mCutoffData.isEmpty() || !InsDat->mResonanceData.isEmpty() )
			{
				if( !InsDat->mCutoffData.isEmpty() )
				{
					cutoff = qBound( 0.0f, InsDat->mCutoffData[ i ], 1.0f );
				}

				if( !InsDat->mResonanceData.isEmpty() )
				{
					res = qBound( 0.0f, InsDat->mResonanceData[ i ], 1.0f );
				}

				updateFilter( cutoff, res, p, k, r );
			}

			float			x = qBound( -1.0f, AudDst[ i ], 1.0f ) - r * ACD.y4;

#if defined( QT_DEBUG )
            if( std::isinf( x ) || std::isnan( x ) )
			{
				x = qBound( -1.0f, AudDst[ i ], 1.0f );
			}
#endif

			//Four cascaded onepole filters (bilinear transform)

			ACD.y1=x*p + ACD.oldx*p - k*ACD.y1;
			ACD.y2=ACD.y1*p+ACD.oldy1*p - k*ACD.y2;
			ACD.y3=ACD.y2*p+ACD.oldy2*p - k*ACD.y3;
			ACD.y4=ACD.y3*p+ACD.oldy3*p - k*ACD.y4;

			//Clipper band limited sigmoid

			ACD.y4 -= ( ACD.y4 * ACD.y4 * ACD.y4 ) / 6.0f;

            if( std::isnan( ACD.y4 ) )
			{
				memset( &ACD, 0, sizeof( ACD ) );

				x = 0;
			}

			ACD.oldx = x;
			ACD.oldy1 = ACD.y1;
			ACD.oldy2 = ACD.y2;
			ACD.oldy3 = ACD.y3;

			AudDst[ i ] = ACD.y4;
		}
	}
}
