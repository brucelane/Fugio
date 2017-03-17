#-------------------------------------------------
#
# Project created by QtCreator 2016-02-25T09:19:59
#
#-------------------------------------------------

include( ../../FugioGlobal.pri )

TARGET = $$qtLibraryTarget(fugio-luaqt)
TEMPLATE = lib

CONFIG += plugin c++11

QT += gui widgets

DESTDIR = $$DESTDIR/plugins

SOURCES += \
	luapainter.cpp \
	luapen.cpp \
	luacolor.cpp \
	luabrush.cpp \
	luarectf.cpp \
	luapointf.cpp \
	luasizef.cpp \
	luafont.cpp \
	luafontmetrics.cpp \
	luagradient.cpp \
	luaqtplugin.cpp \
	luaimage.cpp \
	luatransform.cpp \
	luamatrix4x4.cpp \
	luajsondocument.cpp \
	luajsonarray.cpp \
	luajsonobject.cpp \
	luavector3.cpp \
	luaquaternion.cpp

HEADERS +=\
	../../include/fugio/luaqt/uuid.h \
	../../include/fugio/nodecontrolbase.h \
	../../include/fugio/pincontrolbase.h \
	luapainter.h \
	luapen.h \
	luacolor.h \
	luabrush.h \
	luarectf.h \
	luapointf.h \
	luasizef.h \
	luafont.h \
	luafontmetrics.h \
	luagradient.h \
	luaqtplugin.h \
	luaimage.h \
	luatransform.h \
	luamatrix4x4.h \
	luajsondocument.h \
	luajsonarray.h \
	luajsonobject.h \
	luavector3.h \
	luaquaternion.h

RESOURCES += \
    resources.qrc

TRANSLATIONS = \
	translations/fugio_luaqt_de.ts \
	translations/fugio_luaqt_es.ts \
	translations/fugio_luaqt_fr.ts

#------------------------------------------------------------------------------
# OSX plugin bundle

macx {
	DEFINES += TARGET_OS_MAC
	CONFIG -= x86
	CONFIG += lib_bundle

	BUNDLEDIR    = $$DESTDIR/$$TARGET".bundle"
	INSTALLDEST  = $$INSTALLDATA/plugins
	INCLUDEDEST  = $$INSTALLDATA/include/fugio

	DESTDIR = $$BUNDLEDIR/Contents/MacOS
	DESTLIB = $$DESTDIR/"lib"$$TARGET".dylib"

	CONFIG(release,debug|release) {
		QMAKE_POST_LINK += echo

		LIBCHANGEDEST = $$DESTLIB

		QMAKE_POST_LINK += $$qtLibChange( QtWidgets )
		QMAKE_POST_LINK += $$qtLibChange( QtGui )
		QMAKE_POST_LINK += $$qtLibChange( QtCore )

		QMAKE_POST_LINK += && defaults write $$absolute_path( "Contents/Info", $$BUNDLEDIR ) CFBundleExecutable "lib"$$TARGET".dylib"

		# we don't want to copy the Lua library into the bundle, so change its name

#		QMAKE_POST_LINK += && install_name_tool -change /usr/local/opt/lua/lib/liblua.5.2.dylib liblua.5.2.dylib $$LIBCHANGEDEST

#		QMAKE_POST_LINK += && macdeployqt $$BUNDLEDIR -always-overwrite -no-plugins

		#QMAKE_POST_LINK += && $$FUGIO_ROOT/Fugio/mac_fix_libs.sh $$BUNDLEDIR/Contents/MacOS

#		# now change it back

#		QMAKE_POST_LINK += && install_name_tool -change liblua.5.2.dylib /usr/local/lib/liblua.5.2.dylib $$LIBCHANGEDEST

		isEmpty( CASKBASE ) {
			QMAKE_POST_LINK += && $$FUGIO_ROOT/Fugio/mac_fix_libs_shared.sh $$BUNDLEDIR/Contents/MacOS
		}

		isEmpty( CASKBASE ) {
			exists( /usr/local/opt/lua ) {
				QMAKE_POST_LINK += && mkdir -pv $$INSTALLDIR/data/libs

				QMAKE_POST_LINK += && cp -a /usr/local/opt/lua/lib/*.dylib $$INSTALLDATA/libs/

			} else:exists( $$(LIBS)/lua-5.3.3/src ) {
				QMAKE_POST_LINK += && mkdir -pv $$INSTALLDIR/data/libs

				QMAKE_POST_LINK += && cp $$(LIBS)/lua-5.3.3/src/liblua5.3.3.dylib $$INSTALLDATA/libs/
			}
		}

		plugin.path = $$INSTALLDEST
		plugin.files = $$BUNDLEDIR
		plugin.extra = rm -rf $$INSTALLDEST/$$TARGET".bundle"

		INSTALLS += plugin
	}
}

windows {
	INSTALLDIR   = $$INSTALLBASE/packages/com.bigfug.fugio

	CONFIG(release,debug|release) {
		QMAKE_POST_LINK += echo

		QMAKE_POST_LINK += & mkdir $$shell_path( $$INSTALLDIR/data/plugins )

		QMAKE_POST_LINK += & copy /V /Y $$shell_path( $$DESTDIR/$$TARGET".dll" ) $$shell_path( $$INSTALLDIR/data/plugins )

		win32 {
			 QMAKE_POST_LINK += & copy /V /Y $$shell_path( $$(LIBS)/Lua-5.3.3/lua53.dll ) $$shell_path( $$INSTALLDIR/data )
		}
	}
}

#------------------------------------------------------------------------------
# Linux

unix:!macx {
	INSTALLDIR = $$INSTALLBASE/packages/com.bigfug.fugio

	contains( DEFINES, Q_OS_RASPBERRY_PI ) {
		target.path = Desktop/Fugio/plugins
	} else {
		target.path = $$shell_path( $$INSTALLDIR/data/plugins )
	}

	INSTALLS += target
}

#------------------------------------------------------------------------------
# Lua

win32:exists( $$(LIBS)/Lua-5.3.3 ) {
	INCLUDEPATH += $$(LIBS)/Lua-5.3.3/include

	LIBS += -L$$(LIBS)/Lua-5.3.3 -llua53

	DEFINES += LUA_SUPPORTED
}

macx {
	exists( /usr/local/opt/lua ) {
		INCLUDEPATH += /usr/local/opt/lua/include

		LIBS += -L/usr/local/opt/lua/lib -llua

		DEFINES += LUA_SUPPORTED

	} else:exists( $$(LIBS)/lua-5.3.3/src ) {
		INCLUDEPATH += $$(LIBS)/lua-5.3.3/src

		LIBS += -L$$(LIBS)/lua-5.3.3/src -llua5.3.3

		DEFINES += LUA_SUPPORTED
	}
}

unix:!macx {
	exists( $$[QT_SYSROOT]/usr/include/lua5.2/lua.h ) {
		INCLUDEPATH += $$[QT_SYSROOT]/usr/include/lua5.2

		LIBS += -llua5.2

		DEFINES += LUA_SUPPORTED

	} else:exists( /usr/include/lua5.3/lua.h ) {
		INCLUDEPATH += /usr/include/lua5.3

		LIBS += -llua5.3

		DEFINES += LUA_SUPPORTED

	} else:exists( /usr/include/lua5.2/lua.h ) {
		INCLUDEPATH += /usr/include/lua5.2

		LIBS += -llua5.2

		DEFINES += LUA_SUPPORTED
	}
}

!contains( DEFINES, LUA_SUPPORTED ) {
	warning( "Lua not supported" )
}

#------------------------------------------------------------------------------
# API

INCLUDEPATH += $$PWD/../../include

