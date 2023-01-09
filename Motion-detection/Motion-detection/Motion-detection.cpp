// Created by Heresy @ 2015/08/11
// Blog Page: https://kheresy.wordpress.com/2015/09/04/vgb-cpp-api/
// This sample is used to read the gesture databases from Visual Gesture Builder and detect gestures.

// Standard Library
#include <string>
#include <iostream>

// Kinect for Windows SDK Header
#include <Kinect.h>
#include <Kinect.VisualGestureBuilder.h>

using namespace std;

enum Act { DOWNKICK, UPKICK, DEFENSE };
const int key1[] = { 0x57, 0x51, 0x45 };
const int key2[] = { 0x55, 0x59, 0x49 };
const Act motion[] = { DEFENSE, DOWNKICK, DOWNKICK, UPKICK, UPKICK }; // According to HCI.gbd

void press(int act) {
	keybd_event(act, 0x1E, 0, 0);
	keybd_event(act, 0x1E, KEYEVENTF_KEYUP, 0);
	// Sleep(1000);
}

// Defined by myself
int userCount = 0;
int p1x, p2x;
Act p1Act, p2Act;

void keyboardInput(Act p1, Act p2) {
	press(key1[p1]);
	press(key2[p2]);
}


int main(int argc, char** argv)
{
#pragma region Sensor related code
	// Get default Sensor
	cout << "Try to get default sensor" << endl;
	IKinectSensor* pSensor = nullptr;
	if (GetDefaultKinectSensor(&pSensor) != S_OK)
	{
		cerr << "Get Sensor failed" << endl;
		return -1;
	}

	// Open sensor
	cout << "Try to open sensor" << endl;
	if (pSensor->Open() != S_OK)
	{
		cerr << "Can't open sensor" << endl;
		return -1;
	}
#pragma endregion

#pragma region Body releated code
	// Get body frame source
	cout << "Try to get body source" << endl;
	IBodyFrameSource* pBodyFrameSource = nullptr;
	if (pSensor->get_BodyFrameSource(&pBodyFrameSource) != S_OK)
	{
		cerr << "Can't get body frame source" << endl;
		return -1;
	}

	// Get the number of body
	INT32 iBodyCount = 0;
	if (pBodyFrameSource->get_BodyCount(&iBodyCount) != S_OK)
	{
		cerr << "Can't get body count" << endl;
		return -1;
	}
	cout << " > Can trace " << iBodyCount << " bodies" << endl;

	// Allocate resource for bodies
	IBody** aBody = new IBody * [iBodyCount];
	for (int i = 0; i < iBodyCount; ++i)
		aBody[i] = nullptr;

	// get body frame reader
	cout << "Try to get body frame reader" << endl;
	IBodyFrameReader* pBodyFrameReader = nullptr;
	if (pBodyFrameSource->OpenReader(&pBodyFrameReader) != S_OK)
	{
		cerr << "Can't get body frame reader" << endl;
		return -1;
	}
#pragma endregion

#pragma region Visual Gesture Builder Database
	// Load gesture dataase from File
	wstring sDatabaseFile = L"HCI.gbd";	// Modify this file to load other file
	IVisualGestureBuilderDatabase* pGestureDatabase = nullptr;
	wcout << L"DataBase: ";
	wcin >> sDatabaseFile;
	wcout << L"Try to load gesture database file " << sDatabaseFile << endl;
	if (CreateVisualGestureBuilderDatabaseInstanceFromFile(sDatabaseFile.c_str(), &pGestureDatabase) != S_OK)
	{
		wcerr << L"Can't read database file " << sDatabaseFile << endl;
		return -1;
	}

	// Get the number of gestures in database
	UINT iGestureCount = 0;
	cout << "Try to read gesture list" << endl;
	if (pGestureDatabase->get_AvailableGesturesCount(&iGestureCount) != S_OK)
	{
		cerr << "Can't read the gesture count" << endl;
		return -1;
	}
	if (iGestureCount == 0)
	{
		cerr << "There is no gesture in the database" << endl;
		return -1;
	}

	// get the list of gestures
	IGesture** aGestureList = new IGesture * [iGestureCount];
	if (pGestureDatabase->get_AvailableGestures(iGestureCount, aGestureList) != S_OK)
	{
		cerr << "Can't read the gesture list" << endl;
		return -1;
	}
	else
	{
		// output the gesture list
		cout << "There are " << iGestureCount << " gestures in the database: " << endl;
		GestureType mType;
		const UINT uTextLength = 260; // magic number, if value smaller than 260, can't get name
		wchar_t sName[uTextLength];
		for (UINT i = 0; i < iGestureCount; ++i)
		{
			if (aGestureList[i]->get_GestureType(&mType) == S_OK)
			{
				if (mType == GestureType_Discrete)
					cout << "\t[D] ";
				else if (mType == GestureType_Continuous)
					cout << "\t[C] ";

				if (aGestureList[i]->get_Name(uTextLength, sName) == S_OK)
					wcout << sName << endl;
			}
		}
	}
#pragma endregion

#pragma region Gesture frame related code
	// create for each possible body
	IVisualGestureBuilderFrameSource** aGestureSources = new IVisualGestureBuilderFrameSource * [iBodyCount];
	IVisualGestureBuilderFrameReader** aGestureReaders = new IVisualGestureBuilderFrameReader * [iBodyCount];
	for (int i = 0; i < iBodyCount; ++i)
	{
		// frame source
		aGestureSources[i] = nullptr;
		if (CreateVisualGestureBuilderFrameSource(pSensor, 0, &aGestureSources[i]) != S_OK)
		{
			cerr << "Can't create IVisualGestureBuilderFrameSource" << endl;
			return -1;
		}

		// set gestures
		if (aGestureSources[i]->AddGestures(iGestureCount, aGestureList) != S_OK)
		{
			cerr << "Add gestures failed" << endl;
			return -1;
		}

		// frame reader
		aGestureReaders[i] = nullptr;
		if (aGestureSources[i]->OpenReader(&aGestureReaders[i]) != S_OK)
		{
			cerr << "Can't open IVisualGestureBuilderFrameReader" << endl;
			return -1;
		}
	}
#pragma endregion

	// Enter main loop
	int iStep = 0;
	while (iStep < 100000)
	{
		// 4a. Get last frame
		IBodyFrame* pBodyFrame = nullptr;
		if (pBodyFrameReader->AcquireLatestFrame(&pBodyFrame) == S_OK)
		{
			++iStep;

			// 4b. get Body data
			if (pBodyFrame->GetAndRefreshBodyData(iBodyCount, aBody) == S_OK)
			{
				// 4c. for each body
				for (int i = 0; i < iBodyCount; ++i)
				{
					IBody* pBody = aBody[i];

					// check if is tracked
					BOOLEAN bTracked = false;
					if ((pBody->get_IsTracked(&bTracked) == S_OK) && bTracked)
					{
						// get tracking ID of body
						UINT64 uTrackingId = 0;
						if (pBody->get_TrackingId(&uTrackingId) == S_OK)
						{
							// get tracking id of gesture
							UINT64 uGestureId = 0;
							if (aGestureSources[i]->get_TrackingId(&uGestureId) == S_OK)
							{
								if (uGestureId != uTrackingId)
								{
									// assign traking ID if the value is changed
									cout << "Gesture Source " << i << " start to track user " << uTrackingId << endl;
									aGestureSources[i]->put_TrackingId(uTrackingId);
								}
							}
						}

						// Get gesture frame for this body
						IVisualGestureBuilderFrame* pGestureFrame = nullptr;
						if (aGestureReaders[i]->CalculateAndAcquireLatestFrame(&pGestureFrame) == S_OK)
						{
							// check if the gesture of this body is tracked
							BOOLEAN bGestureTracked = false;
							if (pGestureFrame->get_IsTrackingIdValid(&bGestureTracked) == S_OK && bGestureTracked)
							{
								GestureType mType;
								const UINT uTextLength = 260;
								wchar_t sName[uTextLength];

								++userCount;
								if (userCount > 2)
									break;

								// for each gestures
								for (UINT j = 0; j < iGestureCount; ++j)
								{
									// get gesture information
									// aGestureList[j]->get_GestureType(&mType);
									// We chose only mType == GestureType_Discrete rather than GestureType_Continuous
									aGestureList[j]->get_Name(uTextLength, sName);

									// get gesture result
									IDiscreteGestureResult* pGestureResult = nullptr;
									if (pGestureFrame->get_DiscreteGestureResult(aGestureList[j], &pGestureResult) == S_OK)
									{
										// check if is detected
										BOOLEAN bDetected = false;
										if (pGestureResult->get_Detected(&bDetected) == S_OK && bDetected)
										{
											float fConfidence = 0.0f;
											pGestureResult->get_Confidence(&fConfidence);

											// output information
											wcout << L"Detected Gesture " << sName << L" @" << fConfidence << endl;
											if (userCount == 1)
											{
												p1Act = motion[i];
											}
											else if (userCount == 2)
											{
												p2Act = motion[i];
											}
										}
										pGestureResult->Release();
									}
								}

								// Tell different from player1 and player 2 via x index
								Joint aJoints[JointType::JointType_Count];
								if (pBody->GetJoints(JointType::JointType_Count, aJoints) == S_OK)
								{
									const Joint& rJointPos = aJoints[JointType::JointType_Head];
									if (rJointPos.TrackingState != TrackingState_NotTracked)
									{
										if (userCount == 1) 
										{
											p1x = rJointPos.Position.X;
										}
										else if (userCount == 2) 
										{
											p2x = rJointPos.Position.X;
										}
									}
								}

								
							}
							pGestureFrame->Release();
						}
					}
				}
				if (userCount == 2) {
					if (p1x > p2x)
					{
						Act tmp = p1Act;
						p1Act = p2Act;
						p2Act = tmp;
					}
					keyboardInput(p1Act, p2Act);
				}
				userCount = 0;
			}
			else
			{
				cerr << "Can't read body data" << endl;
			}

			// release frame
			pBodyFrame->Release();
		}
	}

#pragma region Resource release
	// release gesture data
	for (UINT i = 0; i < iGestureCount; ++i)
		aGestureList[i]->Release();
	delete[] aGestureList;

	// release body data
	for (int i = 0; i < iBodyCount; ++i)
		aBody[i]->Release();
	delete[] aBody;

	// release gesture source and reader
	for (int i = 0; i < iBodyCount; ++i)
	{
		aGestureReaders[i]->Release();
		aGestureSources[i]->Release();
	}
	delete[] aGestureReaders;
	delete[] aGestureSources;


	// release body frame source and reader
	pBodyFrameReader->Release();
	pBodyFrameSource->Release();

	// Close Sensor
	pSensor->Close();

	// Release Sensor
	pSensor->Release();
#pragma endregion

	return 0;
}