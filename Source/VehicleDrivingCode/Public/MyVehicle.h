//+---------------------------------------------------------+
//| Project   : Vehicle Driving Code C++ UE 4.24			|
//| Author    : github.com/LordWake					 		|
//+---------------------------------------------------------+

#pragma once

#include "CoreMinimal.h"
#include "WheeledVehicle.h"
#include "MyVehicle.generated.h"

UCLASS()
class VEHICLEDRIVINGCODE_API AMyVehicle : public AWheeledVehicle
{
	GENERATED_BODY()

	UPROPERTY(Category = Camera, EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* SpringArm;

	UPROPERTY(Category = Camera, EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* MyCamera;

public:

	AMyVehicle();

private:

	const float minPitchView = -40.0f;
	const float maxPitchView = 15.0f;
	const float groundedRollForce = 10.0f;
	const float inAirRollForce = 1.0f;
	const float inAirPitchForce = 1.5f;
	
	float lookUpCameraValue;

	void ApplyThrottle(float ThrottleValue);
	void ApplySteering(float SteeringValue);

	void LookUpCamera(float LookUpValue, bool bIsGamePad);
	void TurnAround(float TurnValue);

	void OnHandbrakePressed();
	void OnHandbrakeReleased();

	void UpdateInAirControl(float DeltaTime);

	/* Template to invert axis value depending of mouse axis and gamepad thumbstick */
	template<bool bIsUsingAGamePad>
	FORCEINLINE void LookUpCamera(float LookUpValue) { LookUpCamera(LookUpValue, bIsUsingAGamePad); }

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
};
