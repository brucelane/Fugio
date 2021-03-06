#include "inttobitsnode.h"

#include <QBitArray>

#include <fugio/core/uuid.h>

#include <fugio/context_interface.h>

IntToBitsNode::IntToBitsNode( QSharedPointer<fugio::NodeInterface> pNode )
	: NodeControlBase( pNode )
{
	FUGID( PIN_INPUT_INTEGER, "9e154e12-bcd8-4ead-95b1-5a59833bcf4e" );
	FUGID( PIN_INPUT_BITS, "1b5e9ce8-acb9-478d-b84b-9288ab3c42f5" );
	FUGID( PIN_OUTPUT_ARRAY, "261cc653-d7fa-4c34-a08b-3603e8ae71d5" );

	mPinInputInteger = pinInput( "Integer", PIN_INPUT_INTEGER );
	mPinInputBits    = pinInput( "Bits", PIN_INPUT_BITS );

	mPinInputBits->setValue( 8 );

	mValOutputBitArray = pinOutput<fugio::VariantInterface *>( "Bits", mPinOutputBitArray, PID_BITARRAY, PIN_OUTPUT_ARRAY );
}

void IntToBitsNode::inputsUpdated( qint64 pTimeStamp )
{
	Q_UNUSED( pTimeStamp )

	int		Value = variant( mPinInputInteger ).toInt();
	int		Bits  = variant( mPinInputBits ).toInt();

	if( Bits <= 0 || Bits >= 32 )
	{
		return;
	}

	QBitArray		A;

	A.resize( Bits );

	for( int i = 0 ; i < Bits ; i++, Value >>= 1 )
	{
		bool	v = ( Value & 1 );

		A.setBit( Bits - 1 - i, v );
	}

	if( A != mValOutputBitArray->variant().toBitArray() )
	{
		mValOutputBitArray->setVariant( A );

		pinUpdated( mPinOutputBitArray );
	}
}
