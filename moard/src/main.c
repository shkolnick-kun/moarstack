//
// Created by svalov on 7/2/16.
//
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include "libraryLoader.h"
#include "configFiles.h"
#include "moarLibInterface.h"	// MoarLayerStartupParams_T
#include "threadManager.h"
#include "moarLibrary.h"
#include "layerSockets.h"		// socketsPrepare(), socketUp(), socketDown() and so on
#include "moarInterface.h"		// MoarIfaceStartupParams_T
#include <moarLogger.h>

#define IFACE_CHANNEL_SOCKET_FILE	"IfaceChannelSocket.file"
#define SERVICE_APP_SOCKET_FILE		"ServiceAppSocket.file"

//#define LOAD_MULTIPLE_INTERFACES

#ifndef LOAD_MULTIPLE_INTERFACES
#define LAYERS_COUNT	(MoarLayer_LayersCount)
#else
#define LAYERS_COUNT	(MoarLayer_LayersCount+1)
#endif

void signalHandler(int signo){
    if(SIGINT == signo){
        printf("Received SIGINT, terminating\n");
    }
    else
        printf("Received signal %d\n",signo);
}

int runLayer( MoarLibrary_T * layerLibrary ) {
	int				result;
    void			* vsp;
    MoarLayerType_T	layerType = layerLibrary->Info.LayerType;

    if( MoarLayer_Interface == layerType ) {  // I DONT LIKE THIS PLACE
		MoarIfaceStartupParams_T	* spIface;

		spIface = ( MoarIfaceStartupParams_T * )calloc( 1, sizeof( MoarIfaceStartupParams_T ) );

		if( NULL == spIface )
			return -1;

		strncpy( spIface->socketToChannel, IFACE_CHANNEL_SOCKET_FILE, SOCKET_FILEPATH_SIZE ); // I DONT LIKE THIS PLACE
		vsp = spIface;
    } else {
        MoarLayerStartupParams_T	* spNonIface;

		spNonIface = ( MoarLayerStartupParams_T * )calloc( 1, sizeof( MoarLayerStartupParams_T ) );

        if( NULL == spNonIface )
            return -1;

		spNonIface->DownSocketHandler = SocketDown( layerType );
		spNonIface->UpSocketHandler = SocketUp( layerType );
        vsp = spNonIface;
    }

	result = createThread( layerLibrary, vsp );
	printf( FUNC_RESULT_SUCCESS == result ? "%s started\n" : "failed starting %s\n", layerLibrary->Info.LibraryName );
	return result;
}

int LogWorkIllustration( void ) {
	LogHandle_T log;
	int			result;
	char		bd[] = { '0', '1', '\0', '2', '3', '\0', '4', '5' };

	result = LogOpen( "/tmp/someMoarLog.log", &log );

	if( FUNC_RESULT_SUCCESS != result ) {
		printf( "Error starting logger\n" );
		fflush( stdout );
		return result;
	}

	// who cares about the results?..
	result = LogSetLevelLog( log, LogLevel_Information );
	result = LogSetLevelDump( log, LogLevel_Warning );
	result = LogWrite( log, LogLevel_DebugQuiet, "Some%s data of %zu\nlength: %b\n\n(DebugQuiet)\n", " binary", sizeof( bd ), bd, sizeof( bd ) );
	result = LogWrite( log, LogLevel_Information, "Some%s data of %zu\nlength: %b\n\n(Information)\n", " binary", sizeof( bd ), bd, sizeof( bd ) );
	result = LogWrite( log, LogLevel_Warning, "Some%s data of %zu\nlength: %b\n\n(Warning)\n", " binary", sizeof( bd ), bd, sizeof( bd ) );
	result = LogClose( &log );

	return result;
}

int main(int argc, char** argv)
{
    MoarLibrary_T libraries[LAYERS_COUNT];

	LogWorkIllustration();

    char *fileNames[LAYERS_COUNT];
    fileNames[MoarLayer_Interface] = LIBRARY_PATH_INTERFACE;
    fileNames[MoarLayer_Channel] = LIBRARY_PATH_CHANNEL;
    fileNames[MoarLayer_Routing] = LIBRARY_PATH_ROUTING;
    fileNames[MoarLayer_Presentation] = LIBRARY_PATH_PRESENTATION;
    fileNames[MoarLayer_Service] = LIBRARY_PATH_SERVICE;
    //testing multiple instances
#ifdef LOAD_MULTIPLE_INTERFACES
    fileNames[MoarLayer_Service+1] = LIBRARY_PATH_INTERFACE;
#endif

    //setup signal handler
    signal(SIGINT, signalHandler);
    //load
    for(int i=0; i<LAYERS_COUNT;i++) {
        int res = loadLibrary(fileNames[i], libraries+i);
        if (FUNC_RESULT_SUCCESS == res)
            printf("%s by %s loaded, %d\n", libraries[i].Info.LibraryName, libraries[i].Info.Author, libraries[i].Info.TargetMoarApiVersion);
        else
            printf("%s load failed\n",fileNames[i]);
    }
    SocketsPrepare( IFACE_CHANNEL_SOCKET_FILE, SERVICE_APP_SOCKET_FILE );
    //start layers here
	for(int i = 0; i < LAYERS_COUNT; i++)
		runLayer( libraries + i );

    //wait
    pause();
    //stop
    for(int i=0; i<LAYERS_COUNT;i++) {
        int res = exitThread(libraries+i);
        if(FUNC_RESULT_SUCCESS == res)
            printf("thread %s exited\n",libraries[i].Info.LibraryName);
        else
            printf("failed thread exit for %s\n",libraries[i].Info.LibraryName);
    }
    //unload
    for(int i=0; i<LAYERS_COUNT;i++) {
        int res = closeLibrary(libraries+i);
        //check for empty lib closing
        if (FUNC_RESULT_SUCCESS == res)
            printf("library %s closed\n",libraries[i].Filename);
        else
            printf("close %s failed\n",libraries[i].Filename);
    }
    return 0;
}