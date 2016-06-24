#ifndef GLOBAL_PRIVATE_H
#define GLOBAL_PRIVATE_H

#include <QObject>
#include <QMap>
#include <QDir>
#include <QSharedPointer>
#include <QNetworkReply>
#include <QMultiMap>
#include <QElapsedTimer>
#include <QCommandLineParser>

#include <fugio/global.h>

#include <fugio/global_interface.h>
#include <fugio/context_interface.h>
#include <fugio/device_factory_interface.h>

#include <fugio/global_signals.h>

//class IPlugin;

class FUGIOLIBSHARED_EXPORT GlobalPrivate : public fugio::GlobalSignals, public fugio::GlobalInterface
{
	Q_OBJECT
	Q_INTERFACES( fugio::GlobalInterface )

	explicit GlobalPrivate( QObject *pParent = 0 );

public:
	virtual ~GlobalPrivate( void );

    void loadPlugins( QDir pDir );

	void unloadPlugins( void );

	bool loadPlugin( const QString &pFileName );

	void registerPlugin( QObject *pPluginInstance );

	void initialisePlugins( void );

	//-------------------------------------------------------------------------
	// fugio::IGlobal

	virtual fugio::GlobalSignals *qobject( void ) Q_DECL_OVERRIDE
	{
		return( this );
	}

	virtual qint64 timestamp( void ) const Q_DECL_OVERRIDE
	{
		return( mGlobalTimer.elapsed() );
	}

	//-------------------------------------------------------------------------
	// Pause global execution

	virtual bool isPaused( void ) const Q_DECL_OVERRIDE
	{
		return( mPause );
	}

	virtual void pause( void ) Q_DECL_OVERRIDE
	{
		mPause = true;
	}

	virtual void unpause( void ) Q_DECL_OVERRIDE
	{
		mPause = false;
	}

	//-------------------------------------------------------------------------

	virtual QCommandLineParser &commandLineParser( void ) Q_DECL_OVERRIDE
	{
		return( mCommandLineParser );
	}

	virtual void registerInterface( const QUuid &pUuid, QObject *pInterface ) Q_DECL_OVERRIDE;
	virtual void unregisterInterface( const QUuid &pUuid ) Q_DECL_OVERRIDE;

	virtual QObject *findInterface( const QUuid &pUuid ) Q_DECL_OVERRIDE;

	virtual void registerNodeClasses( const fugio::ClassEntryList &pNodes ) Q_DECL_OVERRIDE;
	virtual void unregisterNodeClasses( const fugio::ClassEntryList &pNodes ) Q_DECL_OVERRIDE;

	virtual void registerNodeClasses( const fugio::ClassEntry pNodes[] ) Q_DECL_OVERRIDE;
	virtual void unregisterNodeClasses( const fugio::ClassEntry pNodes[] ) Q_DECL_OVERRIDE;

	virtual void registerPinClasses( const fugio::ClassEntryList &pNodes ) Q_DECL_OVERRIDE;
	virtual void unregisterPinClasses( const fugio::ClassEntryList &pNodes ) Q_DECL_OVERRIDE;

	virtual void registerPinClasses( const fugio::ClassEntry pNodes[] ) Q_DECL_OVERRIDE;
	virtual void unregisterPinClasses( const fugio::ClassEntry pNodes[] ) Q_DECL_OVERRIDE;

	virtual QSharedPointer<fugio::NodeInterface> createNode( fugio::ContextInterface *pContext, const QString &pName, const QUuid &pUuid, const QUuid &pOrigId, const QVariantHash &pSettings ) Q_DECL_OVERRIDE;

	virtual QSharedPointer<fugio::PinInterface> createPin(const QString &pName, const QUuid &pLocalId, QSharedPointer<fugio::NodeInterface> pNode, PinDirection pDirection, const QUuid &pControlUUID , const QVariantHash &pSettings ) Q_DECL_OVERRIDE;

	virtual QSharedPointer<fugio::PinControlInterface> createPinControl( const QUuid &pUUID , QSharedPointer<fugio::PinInterface> pPin ) Q_DECL_OVERRIDE;

	virtual const QMetaObject *findNodeMetaObject( const QString &pClassName ) const Q_DECL_OVERRIDE;

	virtual fugio::ClassEntry findNodeClassEntry( const QUuid &pNodeUuid ) const Q_DECL_OVERRIDE;

	virtual const QMetaObject *findNodeMetaObject( const QUuid &pNodeUuid ) const Q_DECL_OVERRIDE;
	virtual const QMetaObject *findPinMetaObject( const QUuid &pPinUuid ) const Q_DECL_OVERRIDE;

	virtual QString nodeName( const QUuid &pUuid ) const Q_DECL_OVERRIDE;
	virtual QString pinName( const QUuid &pUuid ) const Q_DECL_OVERRIDE;

	virtual QUuid findNodeByClass( const QString &pClassName ) const Q_DECL_OVERRIDE;
	virtual QUuid findPinByClass( const QString &pClassName ) const Q_DECL_OVERRIDE;

	virtual QStringList pinNames( void ) const Q_DECL_OVERRIDE;

	virtual QMap<QUuid, QString> pinIds( void ) const Q_DECL_OVERRIDE
	{
		return( mPinNameMap );
	}

	virtual bool updatePinControl( QSharedPointer<fugio::PinInterface> pPin, const QUuid &pPinControlUuid ) Q_DECL_OVERRIDE;

	virtual void clear( void ) Q_DECL_OVERRIDE;

	virtual QSharedPointer<fugio::ContextInterface> newContext( void ) Q_DECL_OVERRIDE;

	virtual void delContext( QSharedPointer<fugio::ContextInterface> pContext ) Q_DECL_OVERRIDE;

	virtual QList< QSharedPointer<fugio::ContextInterface> > contexts( void ) Q_DECL_OVERRIDE;

	virtual void setMainWindow( QMainWindow *pMainWindow ) Q_DECL_OVERRIDE;

	virtual QMainWindow *mainWindow( void ) Q_DECL_OVERRIDE;

	virtual void setEditTarget( fugio::EditInterface *pEditTarget ) Q_DECL_OVERRIDE;

	virtual void registerDeviceFactory( fugio::DeviceFactoryInterface *pFactory ) Q_DECL_OVERRIDE;
	virtual void unregisterDeviceFactory( fugio::DeviceFactoryInterface *pFactory ) Q_DECL_OVERRIDE;

	virtual QStringList deviceFactoryMenuTextList( void ) const Q_DECL_OVERRIDE;
	virtual void deviceFactoryGui( QWidget *pParent, const QString &pMenuText ) Q_DECL_OVERRIDE;

	virtual void loadConfig( QSettings &pSettings ) Q_DECL_OVERRIDE;
	virtual void saveConfig( QSettings &pSettings ) const Q_DECL_OVERRIDE;

	//-------------------------------------------------------------------------
	// Joining and Splitting

	virtual void registerPinSplitter( const QUuid &pPinId, const QUuid &pNodeId ) Q_DECL_OVERRIDE;
	virtual void registerPinJoiner( const QUuid &pPinId, const QUuid &pNodeId ) Q_DECL_OVERRIDE;

	virtual QList<QUuid> pinSplitters( const QUuid &pPinId ) const Q_DECL_OVERRIDE;
	virtual QList<QUuid> pinJoiners( const QUuid &pPinId ) const Q_DECL_OVERRIDE;

	//-------------------------------------------------------------------------
	// Menus

	virtual void menuAddEntry( fugio::MenuId, QString pName, QObject *pObject, const char *pSlot ) Q_DECL_OVERRIDE;

	//-------------------------------------------------------------------------

	QUuid instanceId( void ) const;

	static GlobalPrivate *instance( void );

	QList<QObject *> plugInitList( void )
	{
		return( mPluginInitList );
	}

	QList<QObject *> plugInstList( void )
	{
		return( mPluginInstances );
	}

	typedef QMap<QUuid, const QMetaObject *>		UuidClassMap;
	typedef QMap<QUuid, QString>					UuidNameMap;
	typedef QMap<QUuid, QObject *>					UuidObjectMap;
	typedef QMap<QUuid, fugio::ClassEntry>			UuidClassEntryMap;

	const UuidClassEntryMap &nodeMap( void ) const
	{
		return( mNodeMap );
	}

protected:
	bool registerNodeClass( const fugio::ClassEntry &E );

	void unregisterNodeClass( const QUuid &pUUID );

	bool registerPinClass( const QString &pName, const QUuid &pUUID, const QMetaObject *pMetaObject );

	void unregisterPinClass( const QUuid &pUUID );

signals:
	void nodeClassAdded( const fugio::ClassEntry &pClassEntry );

	void globalStart( qint64 pTimeStamp );
	void globalEnd( qint64 pTimeStamp );

private slots:
	void timeout( void );

private:
	static GlobalPrivate			*mInstance;

	QElapsedTimer					 mGlobalTimer;

	QNetworkAccessManager			*mNetworkManager;

//	UuidClassMap					 mNodeClassMap;
//	UuidNameMap						 mNodeNameMap;
	UuidClassEntryMap				 mNodeMap;
	UuidClassMap					 mPinClassMap;
	UuidNameMap						 mPinNameMap;
	UuidObjectMap					 mInterfaceMap;
	int								 mFrameCount;
	qint64							 mLastTime;
	QMainWindow						*mMainWindow;

	QList<QObject *>									 mPluginInstances;
	QList<QSharedPointer<fugio::ContextInterface> >		 mContexts;
	QList<fugio::DeviceFactoryInterface *>				 mDeviceFactories;

	QList<QObject *>				 mPluginInitList;

	fugio::EditInterface					*mEditTarget;

	QMultiMap<QUuid,QUuid>			 mPinSplitters;
	QMultiMap<QUuid,QUuid>			 mPinJoiners;

	QCommandLineParser				 mCommandLineParser;

	bool							 mPause;
};

#endif // GLOBAL_PRIVATE_H