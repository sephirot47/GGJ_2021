// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include <array>
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Curves/CurveVector.h"
#include "Curves/CurveFloat.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SceneComponent.h"
#include <vector>
#include "GameJam2021PlayerController.generated.h"

UCLASS()
class AGameJam2021PlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AGameJam2021PlayerController();

	UPROPERTY(EditAnywhere)
	float mStunTime = 1.0f;

	UPROPERTY(EditAnywhere)
	float mForwardSpeed = 100.0f;

	UPROPERTY(EditAnywhere)
	float mTurnSpeed = 1.0f;

	UPROPERTY(EditAnywhere)
	float mBuildingSize = 100.0f;

	UPROPERTY(EditAnywhere)
	float mShowArrowsTime = 2.0f;

	UPROPERTY(EditAnywhere)
	UClass* mBPBuildingClass = nullptr;

	UPROPERTY(EditAnywhere)
	UClass* mBPBarrierClass = nullptr;

	UPROPERTY(EditAnywhere)
	UCurveFloat *mDirectionArrowsOpacityCurve = nullptr;

	UFUNCTION(BlueprintCallable)
	void OnDeliveryMade();

	UFUNCTION(BlueprintCallable)
	void OnStunned();

	UFUNCTION(BlueprintCallable)
	void OnOverlap(AActor* inOverlappedActor);

	UFUNCTION(BlueprintImplementableEvent)
	void ShowDirectionArrows(const TArray<int>& directions);
	void ShowDirectionArrows_Implementation(const TArray<int>& directions) {}
	UFUNCTION(BlueprintImplementableEvent)
	void SetDirectionArrowsOpacity(const float inOpacity);
	void SetDirectionArrowsOpacity_Implementation(const float inOpacity) {}

	UFUNCTION(BlueprintImplementableEvent)
	void ShowThankDelivery();
	void ShowThankDelivery_Implementation() {}

	UFUNCTION(BlueprintImplementableEvent)
	void OnPausePressedBP();
	void OnPausePressedBP_Implementation() {}

	UFUNCTION(BlueprintImplementableEvent)
	void SetAnimationRate(const float inAnimationRate);
	void SetAnimationRate_Implementation(const float inAnimationRate) {}

	UFUNCTION(BlueprintImplementableEvent)
	void SetRemainingTime(const float inRemainingTime);
	void SetRemainingTime_Implementation(const float inRemainingTime) {}

	UFUNCTION(BlueprintImplementableEvent)
	void SetScore(const float inScore);
	void SetScore_Implementation(const float inScore) {}

	UFUNCTION(BlueprintImplementableEvent)
	void GoToLoseScreen();
	void GoToLoseScreen_Implementation() {}

	UPROPERTY(EditAnywhere)
	AActor* mNextDeliveryTrigger = nullptr;

	UFUNCTION(BlueprintCallable)
	void OnPausePressed();

protected:
	enum class EDirection
	{
		BACK,
		FORWARD,
		LEFT,
		RIGHT
	};

	void InitializeOnFirstTick();
	virtual void PlayerTick(float inDeltaTime) override;
	virtual void SetupInputComponent() override;

	void GoForwardPressed();
	void GoBackPressed();
	void TurnLeftPressed();
	void TurnRightPressed();
	void GoForwardReleased();
	void GoBackReleased();
	void TurnLeftReleased();
	void TurnRightReleased();

	void GenerateNextDelivery(const EDirection & inStartFaceDirection, const bool inIsFirstDelivery = false);

private:
	EDirection GetOppositeDirection(const EDirection& inDirection) const;
	FVector GetGridWorldPosition(const FVector2D& inGridPositionInt) const;
	FVector GetGridWorldPosition(const FVector2D& inGridPositionInt, const EDirection &inBuildingSide) const;
	FVector2D GetDirectionVector(const EDirection &inDirection) const;
	FString GetDirectionString(const EDirection& inDirection) const;

	static constexpr int BuildingArraySize = 6;
	std::array<std::array<AActor*, BuildingArraySize>, BuildingArraySize> mBuildings;

	FVector2D mNextDeliveryGridPosition = FVector2D(0, 0);
	EDirection mNextDeliveryBuildingSide = EDirection::FORWARD;

	bool mGoingForward = false;
	bool mGoingBack = false;
	bool mTurningLeft = false;
	bool mTurningRight = false;
	bool mFirstTick = true;

	float mRemainingTime = 0.0f;
	float mPreviousTotalRemainingTime = 25.0;
	float mScore = 0.0f;
	float mTimeStunned = 0.0f;
	bool mIsStunned = true;

	float mTimeSinceShowArrows = 0.0f;

	ACharacter* mCharacter = nullptr;
	UCapsuleComponent *mCharacterCapsule = nullptr;
	USceneComponent* mRotationComp = nullptr;
};


