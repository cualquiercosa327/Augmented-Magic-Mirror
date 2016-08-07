#pragma once

#include "Callback.h"

class Kinect
{
public:
	typedef std::vector<CameraSpacePoint> CameraSpacePointList;

	Kinect(const Vector3 & Offset);

	void Initialize();
	void Release();
	void Update();

	const Vector3 & GetOffset() const;
	float GetRealWorldToVirutalScale() const;

	Callback<CameraSpacePointList, Vector3, float> FaceModelUpdated;
	
private:
	typedef void(Kinect::*EventCallback)(WAITABLE_HANDLE EventHandle);
	typedef std::pair<WAITABLE_HANDLE, EventCallback> Event;
	typedef std::vector<Event> EventList;
	typedef std::vector<Microsoft::WRL::ComPtr<IBody>> BodyVector;

	const Vector3 Offset;
	const float RealWorldToVirutalScale;

	Microsoft::WRL::ComPtr<IKinectSensor> KinectSensor;
	Microsoft::WRL::ComPtr<IBodyFrameSource> BodyFrameSource;
	Microsoft::WRL::ComPtr<IBodyFrameReader> BodyFrameReader;
	Microsoft::WRL::ComPtr<IBody> TrackedBody;
	Microsoft::WRL::ComPtr<IHighDefinitionFaceFrameSource> HighDefinitionFaceFrameSource;
	Microsoft::WRL::ComPtr<IHighDefinitionFaceFrameReader> HighDefinitionFaceFrameReader;
	Microsoft::WRL::ComPtr<IFaceModel> FaceModel;
	Microsoft::WRL::ComPtr<IFaceAlignment> FaceAlignment;
	BodyVector Bodies;
	EventList Events;

	CameraSpacePointList FaceVertices;

	void SetupBodyFrameReader();
	void SetupHighDefinitionFaceFrameReader();
	void SetupFaceModel();

	template<typename InterfaceType>
	void AddEvent(Microsoft::WRL::ComPtr<InterfaceType> Interface, HRESULT(_stdcall InterfaceType::*EventRegister)(WAITABLE_HANDLE *), EventCallback Callback)
	{
		WAITABLE_HANDLE EventHandle;
		Utility::ThrowOnFail((Interface.Get()->*EventRegister)(&EventHandle));

		Events.push_back(std::make_pair(EventHandle, Callback));
	}

	void CheckEvent(Event & Event);

	void BodyFrameRecieved(WAITABLE_HANDLE EventHandle);
	Microsoft::WRL::ComPtr<IBodyFrame> GetBodyFrame(WAITABLE_HANDLE EventHandle);
	void UpdateBodies(Microsoft::WRL::ComPtr<IBodyFrame> & BodyFrame);
	void UpdateTrackedBody();
	bool UpdateCurrentTrackedBody();
	void TrackNewBody();

	void HighDefinitionFaceFrameRecieved(WAITABLE_HANDLE EventHandle);
	bool UpdateFaceModel(Microsoft::WRL::ComPtr<IHighDefinitionFaceFrame> FaceFrame);
	Microsoft::WRL::ComPtr<IHighDefinitionFaceFrame> GetFaceFrame(WAITABLE_HANDLE EventHandle);
};

