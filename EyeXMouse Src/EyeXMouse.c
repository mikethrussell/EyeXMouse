#include <Windows.h>
//#include <stdio.h>
//#include <conio.h>
#include <assert.h>
#include "eyex\EyeX.h"

#pragma comment (lib, "Tobii.EyeX.Client.lib")
void HandleNewCoords(float x, float y);

float lastRawX = 0;
float lastRawY = 0;

float filteredX = 0;
float filteredY = 0;
float integratedErrorX = 0;
float integratedErrorY = 0;
float integratingSpeed = 0.1f;
const float FIntegrationDeadZone = 50.0; //pixels
const float FDeadZone = 15.0;  // pixels
const float g_SlowZone = 10; // pixels
const float FSpeed = 0.15f; // how fast to move when far away
const float FSlowSpeed = 0.15f; // now fast to move when close
float filteredErrorX = 0; // this will be the distance between the gaze data and the current mouse position
float filteredErrorY = 0; // this will be the distance between the gaze data and the current mouse position

// ID of the global interactor that provides our data stream; must be unique within the application.
static const TX_STRING InteractorId = "Twilight Sparkle";

// global variables
static TX_HANDLE g_hGlobalInteractorSnapshot = TX_EMPTY_HANDLE;

/*
 * Initializes g_hGlobalInteractorSnapshot with an interactor that has the Gaze Point behavior.
 */
BOOL InitializeGlobalInteractorSnapshot(TX_CONTEXTHANDLE hContext)
{
	TX_HANDLE hInteractor = TX_EMPTY_HANDLE;
	TX_HANDLE hBehavior   = TX_EMPTY_HANDLE;
	TX_GAZEPOINTDATAPARAMS params = { TX_GAZEPOINTDATAMODE_LIGHTLYFILTERED };
	BOOL success;

	success = txCreateGlobalInteractorSnapshot(
		hContext,
		InteractorId,
		&g_hGlobalInteractorSnapshot,
		&hInteractor) == TX_RESULT_OK;
	success &= txCreateInteractorBehavior(hInteractor, &hBehavior, TX_INTERACTIONBEHAVIORTYPE_GAZEPOINTDATA) == TX_RESULT_OK;
	success &= txSetGazePointDataBehaviorParams(hBehavior, &params) == TX_RESULT_OK;

	txReleaseObject(&hBehavior);
	txReleaseObject(&hInteractor);

	return success;
}

/*
 * Callback function invoked when a snapshot has been committed.
 */
void TX_CALLCONVENTION OnSnapshotCommitted(TX_CONSTHANDLE hAsyncData, TX_USERPARAM param)
{
	// check the result code using an assertion.
	// this will catch validation errors and runtime errors in debug builds. in release builds it won't do anything.

	TX_RESULT result = TX_RESULT_UNKNOWN;
	txGetAsyncDataResultCode(hAsyncData, &result);
	assert(result == TX_RESULT_OK || result == TX_RESULT_CANCELLED);
}

/*
 * Callback function invoked when the status of the connection to the EyeX Engine has changed.
 */
void TX_CALLCONVENTION OnEngineConnectionStateChanged(TX_CONNECTIONSTATE connectionState, TX_USERPARAM userParam)
{
	switch (connectionState) {
	case TX_CONNECTIONSTATE_CONNECTED: {
			BOOL success;
			printf("The connection state is now CONNECTED (We are connected to the EyeX Engine)\n");
			// commit the snapshot with the global interactor as soon as the connection to the engine is established.
			// (it cannot be done earlier because committing means "send to the engine".)
			success = txCommitSnapshotAsync(g_hGlobalInteractorSnapshot, OnSnapshotCommitted, NULL) == TX_RESULT_OK;
			if (!success) {
				printf("Failed to initialize the data stream.\n");
			}
	
		}
		break;

	case TX_CONNECTIONSTATE_DISCONNECTED:
		printf("The connection state is now DISCONNECTED (We are disconnected from the EyeX Engine)\n");
		break;

	case TX_CONNECTIONSTATE_TRYINGTOCONNECT:
		printf("The connection state is now TRYINGTOCONNECT (We are trying to connect to the EyeX Engine)\n");
		break;

	case TX_CONNECTIONSTATE_SERVERVERSIONTOOLOW:
		printf("The connection state is now SERVER_VERSION_TOO_LOW: this application requires a more recent version of the EyeX Engine to run.\n");
		break;

	case TX_CONNECTIONSTATE_SERVERVERSIONTOOHIGH:
		printf("The connection state is now SERVER_VERSION_TOO_HIGH: this application requires an older version of the EyeX Engine to run.\n");
		break;
	}
}

/*
 * Handles an event from the Gaze Point data stream.
 */
void OnGazeDataEvent(TX_HANDLE hGazeDataBehavior)
{






	TX_GAZEPOINTDATAEVENTPARAMS eventParams;
	if (txGetGazePointDataEventParams(hGazeDataBehavior, &eventParams) == TX_RESULT_OK) {
		

		HandleNewCoords((float)eventParams.X, (float)eventParams.Y);
		

	} else {
		printf("Failed to interpret gaze data event packet.\n");
	}
}

void HandleNewCoords(float x, float y)
{



	if (abs(lastRawX - x) + abs(lastRawY - y) > 50)
	{
		lastRawX = x;
		lastRawY = y;
		return;//reject noise
	}

	float errorX = x - filteredX;
	float errorY = y - filteredY;

	integratedErrorX += errorX;
	integratedErrorY += errorY;

	float speed = FSlowSpeed;
	if (abs(errorX + errorY) > g_SlowZone) speed = FSpeed;
	

	if (abs(errorX) > FDeadZone){ filteredX += speed * errorX; }
	if (abs(errorY) > FDeadZone){ filteredY += speed * errorY; }

	//		if (abs(integratedErrorX) > FIntegrationDeadZone){ filteredX += integratingSpeed * integratedErrorX; integratedErrorX = 0; }
	//		if (abs(integratedErrorY) > FIntegrationDeadZone){ filteredY += integratingSpeed * integratedErrorY; integratedErrorY = 0; }

	SetCursorPos((int)filteredX, (int)filteredY);
}

/*
 * Callback function invoked when an event has been received from the EyeX Engine.
 */
void TX_CALLCONVENTION HandleEvent(TX_CONSTHANDLE hAsyncData, TX_USERPARAM userParam)
{
	TX_HANDLE hEvent = TX_EMPTY_HANDLE;
	TX_HANDLE hBehavior = TX_EMPTY_HANDLE;

    txGetAsyncDataContent(hAsyncData, &hEvent);

	// NOTE. Uncomment the following line of code to view the event object. The same function can be used with any interaction object.
	//OutputDebugStringA(txDebugObject(hEvent));

	if (txGetEventBehavior(hEvent, &hBehavior, TX_INTERACTIONBEHAVIORTYPE_GAZEPOINTDATA) == TX_RESULT_OK) {
		OnGazeDataEvent(hBehavior);
		txReleaseObject(&hBehavior);
	}

	// NOTE since this is a very simple application with a single interactor and a single data stream, 
	// our event handling code can be very simple too. A more complex application would typically have to 
	// check for multiple behaviors and route events based on interactor IDs.

	txReleaseObject(&hEvent);
}


/*
 * Application entry point.
 */
int main(int argc, char* argv[])

{

	TX_CONTEXTHANDLE hContext = TX_EMPTY_HANDLE;
	TX_TICKET hConnectionStateChangedTicket = TX_INVALID_TICKET;
	TX_TICKET hEventHandlerTicket = TX_INVALID_TICKET;
	BOOL success;

	// initialize and enable the context that is our link to the EyeX Engine.
	success = txInitializeSystem(TX_SYSTEMCOMPONENTOVERRIDEFLAG_NONE, NULL, NULL, NULL) == TX_RESULT_OK;
	success &= txCreateContext(&hContext, TX_FALSE) == TX_RESULT_OK;
	success &= InitializeGlobalInteractorSnapshot(hContext);
	success &= txRegisterConnectionStateChangedHandler(hContext, &hConnectionStateChangedTicket, OnEngineConnectionStateChanged, NULL) == TX_RESULT_OK;
	success &= txRegisterEventHandler(hContext, &hEventHandlerTicket, HandleEvent, NULL) == TX_RESULT_OK;
	success &= txEnableConnection(hContext) == TX_RESULT_OK;

	// let the events flow until a key is pressed.
	if (success) {
		printf("Initialization was successful.\n");

		//HIDE CONSOLE WINDOW
		ShowWindow(GetConsoleWindow(), SW_HIDE);


	} else {
		printf("Initialization failed.\n");
	}
	printf("Press any key to exit...\n");

	_getch();
	printf("Exiting.\n");

	// disable and delete the context.
	txDisableConnection(hContext);
	txReleaseObject(&g_hGlobalInteractorSnapshot);
	txShutdownContext(hContext, TX_CLEANUPTIMEOUT_DEFAULT, TX_FALSE);
	txReleaseContext(&hContext);

	return 0;
}
