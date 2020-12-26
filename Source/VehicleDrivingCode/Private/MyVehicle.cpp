//+---------------------------------------------------------+
//| Project   : Vehicle Driving Code C++ UE 4.24			|
//| Author    : github.com/LordWake					 		|
//+---------------------------------------------------------+


#include "MyVehicle.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/PlayerInput.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "WheeledVehicleMovementComponent4W.h"
#include "Kismet/GameplayStatics.h"
#include "..\Public\MyVehicle.h"

static const FName NAME_SteerInput("Steering");
static const FName NAME_ThrottleInput("Throttle");

AMyVehicle::AMyVehicle()
{
	UWheeledVehicleMovementComponent4W* Vehicle4WMoveComponent = CastChecked<UWheeledVehicleMovementComponent4W>(GetVehicleMovement());

	Vehicle4WMoveComponent->MinNormalizedTireLoad			= 0.0f;
	Vehicle4WMoveComponent->MinNormalizedTireLoadFiltered	= 0.2f;
	Vehicle4WMoveComponent->MaxNormalizedTireLoad			= 2.0f;
	Vehicle4WMoveComponent->MaxNormalizedTireLoadFiltered	= 2.0f;

	Vehicle4WMoveComponent->MaxEngineRPM = 7000.0f;
	Vehicle4WMoveComponent->EngineSetup.TorqueCurve.GetRichCurve()->Reset();
	Vehicle4WMoveComponent->EngineSetup.TorqueCurve.GetRichCurve()->AddKey(0.0f, 7000.0f);		//At 0 RPM we have 7000 of torque.
	Vehicle4WMoveComponent->EngineSetup.TorqueCurve.GetRichCurve()->AddKey(2090.0f, 5000.0f);	//At 2090 RPM we have 5000 of torque.
	Vehicle4WMoveComponent->EngineSetup.TorqueCurve.GetRichCurve()->AddKey(5500.0f, 4500.0f);	//At 5500 RPM we have 45000 of torque.

	//This is how much can we turn at certain speed. For example, at 0km we can turn 1.0f
	Vehicle4WMoveComponent->SteeringCurve.GetRichCurve()->Reset();
	Vehicle4WMoveComponent->SteeringCurve.GetRichCurve()->AddKey(0.0f, 1.0f);	
	Vehicle4WMoveComponent->SteeringCurve.GetRichCurve()->AddKey(40.0f, 0.85f);	
	Vehicle4WMoveComponent->SteeringCurve.GetRichCurve()->AddKey(120.0f, 0.75f);

	//Use the 4 wheels to move
	Vehicle4WMoveComponent->DifferentialSetup.DifferentialType = EVehicleDifferential4W::LimitedSlip_4W;
	Vehicle4WMoveComponent->DifferentialSetup.FrontRearSplit = 0.65f;

	//Auto gear box
	Vehicle4WMoveComponent->TransmissionSetup.bUseGearAutoBox = true; 
	Vehicle4WMoveComponent->TransmissionSetup.GearSwitchTime = 0.15f; 
	Vehicle4WMoveComponent->TransmissionSetup.GearAutoBoxLatency = 1.0f;

	//Bones names
	Vehicle4WMoveComponent->WheelSetups[0].BoneName = "Wheel_Front_Left";
	Vehicle4WMoveComponent->WheelSetups[1].BoneName = "Wheel_Front_Right";
	Vehicle4WMoveComponent->WheelSetups[2].BoneName = "Wheel_Rear_Left";
	Vehicle4WMoveComponent->WheelSetups[3].BoneName = "Wheel_Rear_Right";

	//Creates the SpringArmComponent
	SpringArm = CreateDefaultSubobject<USpringArmComponent>("SpringArm");
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 750.0f;
	SpringArm->bEnableCameraLag = true;
	SpringArm->CameraLagMaxDistance = 100.0f;
	SpringArm->SetRelativeLocation(FVector(-30.0f, 0.0f, 100.0f));
	SpringArm->bUsePawnControlRotation = true;

	//Creates the CameraComponent
	MyCamera = CreateDefaultSubobject<UCameraComponent>("ChaseCamera");
	MyCamera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	MyCamera->FieldOfView = 90.f;
}

void AMyVehicle::BeginPlay()
{
	Super::BeginPlay();

	//Clamp CameraController Angle
	APlayerCameraManager* PCM = Cast<APlayerCameraManager>(UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0));
	PCM->ViewPitchMax = maxPitchView;
	PCM->ViewPitchMin = minPitchView;
}

void AMyVehicle::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateInAirControl(DeltaTime);
}

void AMyVehicle::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UPlayerInput::AddEngineDefinedActionMapping(FInputActionKeyMapping("Handbrake",		EKeys::SpaceBar));
	UPlayerInput::AddEngineDefinedActionMapping(FInputActionKeyMapping("Handbrake",		EKeys::Gamepad_FaceButton_Right));
	
	UPlayerInput::AddEngineDefinedAxisMapping(FInputAxisKeyMapping("LookUpGamepad",		EKeys::Gamepad_RightY, 1.f));
	UPlayerInput::AddEngineDefinedAxisMapping(FInputAxisKeyMapping("LookUp",			EKeys::MouseY, 1.f));

	UPlayerInput::AddEngineDefinedAxisMapping(FInputAxisKeyMapping("Turn",				EKeys::MouseX, 1.f));
	UPlayerInput::AddEngineDefinedAxisMapping(FInputAxisKeyMapping("Turn",				EKeys::Gamepad_RightX, 1.f));

	UPlayerInput::AddEngineDefinedAxisMapping(FInputAxisKeyMapping(NAME_ThrottleInput,	EKeys::W, 1.f));
	UPlayerInput::AddEngineDefinedAxisMapping(FInputAxisKeyMapping(NAME_ThrottleInput,	EKeys::S, -1.f));
	UPlayerInput::AddEngineDefinedAxisMapping(FInputAxisKeyMapping(NAME_ThrottleInput,	EKeys::Gamepad_RightTrigger, 1.f));
	UPlayerInput::AddEngineDefinedAxisMapping(FInputAxisKeyMapping(NAME_ThrottleInput,	EKeys::Gamepad_LeftTrigger, -1.f));
	UPlayerInput::AddEngineDefinedAxisMapping(FInputAxisKeyMapping(NAME_ThrottleInput,	EKeys::Gamepad_LeftY, 1.f));

	UPlayerInput::AddEngineDefinedAxisMapping(FInputAxisKeyMapping(NAME_SteerInput,		EKeys::D, 1.f));
	UPlayerInput::AddEngineDefinedAxisMapping(FInputAxisKeyMapping(NAME_SteerInput,		EKeys::A, -1.f));
	UPlayerInput::AddEngineDefinedAxisMapping(FInputAxisKeyMapping(NAME_SteerInput,		EKeys::Gamepad_LeftX, 1.f));

	PlayerInputComponent->BindAxis(NAME_SteerInput, this,				&AMyVehicle::ApplySteering);
	PlayerInputComponent->BindAxis("Throttle", this,					&AMyVehicle::ApplyThrottle);
	PlayerInputComponent->BindAxis("Turn", this,						&AMyVehicle::TurnAround);
	
	PlayerInputComponent->BindAxis("LookUpGamepad", this,				&AMyVehicle::LookUpCamera <true>);
	PlayerInputComponent->BindAxis("LookUp", this,						&AMyVehicle::LookUpCamera <false>);
	
	PlayerInputComponent->BindAction("Handbrake", IE_Pressed,	this,	&AMyVehicle::OnHandbrakePressed);
	PlayerInputComponent->BindAction("Handbrake", IE_Released,	this,	&AMyVehicle::OnHandbrakeReleased);
}

void AMyVehicle::LookUpCamera(float LookUpValue, bool bIsGamePad)
{
	if (LookUpValue != 0.f)
	{
		lookUpCameraValue = bIsGamePad ? 0.25f : -0.25f;
		AddControllerPitchInput(LookUpValue * lookUpCameraValue);
	}
}

void AMyVehicle::TurnAround(float TurnValue)
{
	if (TurnValue != 0.f)
		AddControllerYawInput(TurnValue * 0.5f);
}

void AMyVehicle::ApplyThrottle(float ThrottleValue)
{
	GetVehicleMovementComponent()->SetThrottleInput(ThrottleValue);
}

void AMyVehicle::ApplySteering(float SteeringValue)
{
	GetVehicleMovementComponent()->SetSteeringInput(SteeringValue);
}

void AMyVehicle::OnHandbrakePressed()
{
	GetVehicleMovementComponent()->SetHandbrakeInput(true);
}

void AMyVehicle::OnHandbrakeReleased()
{
	GetVehicleMovementComponent()->SetHandbrakeInput(false);
}

void AMyVehicle::UpdateInAirControl(float DeltaTime)
{
	if (UWheeledVehicleMovementComponent4W* Vehicle4W = CastChecked<UWheeledVehicleMovementComponent4W>(GetVehicleMovement()))
	{
		FHitResult Hit;
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(this);

		const FVector TraceStart	= GetActorLocation() + FVector(0.0f, 0.0, 50.0f);
		const FVector TraceEnd		= GetActorLocation() - FVector(0.0f, 0.0, 200.0f);

		const bool bIsInAir		= !GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, QueryParams);
		const bool bNotGrounded = FVector::DotProduct(GetActorUpVector(), FVector::UpVector) < 0.1f; //Check if the car is aligned with the floor.

		if (bIsInAir || bNotGrounded)
		{
			const float ForwardInput	= InputComponent->GetAxisValue(NAME_ThrottleInput);
			const float RightInput		= InputComponent->GetAxisValue(NAME_SteerInput);

			const float AirMovementForcePitch	= inAirPitchForce;
			const float AirMovementForceRoll	= !bIsInAir && bNotGrounded ? groundedRollForce : inAirRollForce;

			if (UPrimitiveComponent* VehicleMesh = Vehicle4W->UpdatedPrimitive)
			{
				const FVector MovementVector		= FVector(RightInput * -AirMovementForceRoll, ForwardInput * AirMovementForcePitch, 0.0f) * DeltaTime * 200.0f;
				const FVector NewAngularMovement	= GetActorRotation().RotateVector(MovementVector);
			
				VehicleMesh->SetPhysicsAngularVelocity(NewAngularMovement, true);
			}
		}
	}
}
