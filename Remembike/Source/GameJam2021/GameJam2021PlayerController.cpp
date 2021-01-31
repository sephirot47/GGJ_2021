// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameJam2021PlayerController.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

AGameJam2021PlayerController::AGameJam2021PlayerController()
{
	bShowMouseCursor = false;
}

void AGameJam2021PlayerController::InitializeOnFirstTick()
{
	mCharacter = Cast<ACharacter>( GetPawn() );

	for (int y = 0; y < BuildingArraySize; ++y)
	{
		for (int x = 0; x < BuildingArraySize; ++x)
		{
			const FVector building_position = GetGridWorldPosition( FVector2D(x, y) );
			if (!mBPBuildingClass)
				continue;

			mBuildings.at(y).at(x) = GetWorld()->SpawnActor<AActor>(mBPBuildingClass, building_position, FRotator()); // Spawn object
			// mBuildings.at(y).at(x)->SetActorLabel("Building_" + FString::FromInt(x) + "_" + FString::FromInt(y));

			TArray<UChildActorComponent*> building_child_actor_components;
			mBuildings.at(y).at(x)->GetComponents<UChildActorComponent>(building_child_actor_components, true);
			for (UChildActorComponent* building_child_actor_comp : building_child_actor_components)
				building_child_actor_comp->GetChildActor()->SetActorHiddenInGame(true);
		}
	}

	for (int y = -1; y <= BuildingArraySize; ++y)
	{
		for (int x : { -1 , BuildingArraySize })
		{
			const FVector building_position = GetGridWorldPosition(FVector2D(x, y));
			if (!mBPBarrierClass)
				continue;
			AActor *barrier = GetWorld()->SpawnActor<AActor>(mBPBarrierClass, building_position, FRotator());
			barrier->SetActorRotation(FRotator(0, x == -1 ? 180 : 0, 0));
		}
	}

	for (int x = -1; x <= BuildingArraySize; ++x)
	{
		for (int y : { -1, BuildingArraySize })
		{
			const FVector building_position = GetGridWorldPosition(FVector2D(x, y));
			if (!mBPBarrierClass)
				continue;
			AActor* barrier = GetWorld()->SpawnActor<AActor>(mBPBarrierClass, building_position, FRotator());
			barrier->SetActorRotation(FRotator(0, y == -1 ? 90 : -90, 0));
		}
	}



	TArray<USceneComponent*> scene_comps;
	mCharacter->GetComponents<USceneComponent>(scene_comps, true);
	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, FString::FromInt(scene_comps.Num()));
	for (USceneComponent* scene_comp : scene_comps)
	{
		GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, scene_comp->GetName());
		if (scene_comp->ComponentHasTag("RotationActor"))
		{
			mRotationComp = scene_comp;
			break;
		}
	}
	ensure(mRotationComp != nullptr);

	TArray<UCapsuleComponent*> capsules;
	mCharacter->GetComponents(capsules, true);
	ensure(capsules.Num() >= 1);
	mCharacterCapsule = capsules[0];

	mNextDeliveryGridPosition = FVector2D(BuildingArraySize / 2, BuildingArraySize / 2);
	mNextDeliveryBuildingSide = EDirection::RIGHT; // Initial;
	GenerateNextDelivery(EDirection::FORWARD, true);
}

void AGameJam2021PlayerController::PlayerTick(float inDeltaTime)
{
	Super::PlayerTick(inDeltaTime);

	if (mFirstTick)
	{
		mFirstTick = false;
		InitializeOnFirstTick();
	}

	mRemainingTime -= inDeltaTime;
	if (mRemainingTime < 0.0f)
		GoToLoseScreen();
	SetRemainingTime(mRemainingTime);

	if (mIsStunned)
	{
		mTimeStunned += inDeltaTime;
		if (mTimeStunned >= mStunTime)
		{
			mIsStunned = false;
			mTimeStunned = 0.0f;
		}
	}

	if (mTimeSinceShowArrows < mShowArrowsTime && mDirectionArrowsOpacityCurve)
	{
		mTimeSinceShowArrows += inDeltaTime;
		const float norm_time = FMath::Min(mTimeSinceShowArrows / mShowArrowsTime, 1.0f);
		const float opacity = mDirectionArrowsOpacityCurve->GetFloatValue(norm_time);
		SetDirectionArrowsOpacity(opacity);
	}

	// GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, "DoGoAnimation");

	const float animation_rate = mCharacter->GetVelocity().Size() / mCharacter->GetCharacterMovement()->GetMaxSpeed();
	SetAnimationRate(animation_rate);

	if (!mIsStunned)
	{
		if (mGoingForward)
			mCharacter->AddMovementInput(mCharacter->GetActorForwardVector(), mForwardSpeed * inDeltaTime);
		if (mGoingBack)
			mCharacter->AddMovementInput(mCharacter->GetActorForwardVector(), -mForwardSpeed * inDeltaTime);
		
		if (mTurningLeft)
			ControlRotation = FRotator(0, ControlRotation.Yaw - mTurnSpeed * inDeltaTime, 0);
		if (mTurningRight)
			ControlRotation = FRotator(0, ControlRotation.Yaw + mTurnSpeed * inDeltaTime, 0);
	}

	// GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, mCharacter->GetActorRotation().ToString() );
}

void AGameJam2021PlayerController::OnDeliveryMade()
{
	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, "OnDeliveryMade()");

	ShowThankDelivery();

	EDirection start_dir = EDirection::FORWARD;
	if (mNextDeliveryBuildingSide == EDirection::FORWARD || mNextDeliveryBuildingSide == EDirection::BACK)
		start_dir = mCharacter->GetActorRotation().Yaw < 0 ? EDirection::LEFT : EDirection::RIGHT;
	else
		start_dir = FMath::Abs(mCharacter->GetActorRotation().Yaw) < 90 ? EDirection::FORWARD : EDirection::BACK;

	// Reset rotation
	/*
	float yaw = 0.0f;
	switch (start_dir)
	{
	case EDirection::FORWARD: yaw = 0.0f; break;
	case EDirection::BACK: yaw = 180.0f; break;
	case EDirection::LEFT: yaw = -90.0f; break;
	case EDirection::RIGHT: yaw = 90.0f; break;
	}
	ControlRotation = FRotator(0, yaw, 0);
	*/

	GenerateNextDelivery(start_dir);
}

void AGameJam2021PlayerController::OnStunned()
{
	mIsStunned = true;
	mTimeStunned = 0.0f;
}

FVector AGameJam2021PlayerController::GetGridWorldPosition(const FVector2D& inGridPositionInt) const
{
	const FVector transposed_grid_pos_int = FVector(inGridPositionInt.Y, inGridPositionInt.X, 0);
	return transposed_grid_pos_int * mBuildingSize - FVector(BuildingArraySize / 2, BuildingArraySize / 2, 0) * mBuildingSize;
}

FVector AGameJam2021PlayerController::GetGridWorldPosition(const FVector2D& inGridPositionInt, const EDirection& inBuildingSide) const
{
	const FVector world_pos = GetGridWorldPosition(inGridPositionInt);
	const FVector2D dir_vector = GetDirectionVector(inBuildingSide);
	return world_pos + FVector(dir_vector.Y, dir_vector.X, 0) * mBuildingSize * 0.35f;
}

FVector2D AGameJam2021PlayerController::GetDirectionVector(const EDirection& inDirection) const
{
	switch (inDirection)
	{
		case EDirection::FORWARD: return FVector2D(0, 1);
		case EDirection::BACK: return FVector2D(0, -1);
		case EDirection::LEFT: return FVector2D(-1, 0);
		default: return FVector2D(1, 0);
	}
}

FString AGameJam2021PlayerController::GetDirectionString(const EDirection& inDirection) const
{
	switch (inDirection)
	{
		case EDirection::FORWARD: return FString("FORWARD");
		case EDirection::BACK: return FString("BACK");
		case EDirection::LEFT: return FString("LEFT");
		default: return FString("RIGHT");
	}
}

AGameJam2021PlayerController::EDirection AGameJam2021PlayerController::GetOppositeDirection(const EDirection& inDirection) const
{
	switch (inDirection)
	{
		case EDirection::FORWARD: return EDirection::BACK;
		case EDirection::BACK: return EDirection::FORWARD;
		case EDirection::LEFT: return EDirection::RIGHT;
		default: return EDirection::LEFT;
	}
}

void AGameJam2021PlayerController::GenerateNextDelivery(const EDirection& inStartFaceDirection, const bool inIsFirstDelivery)
{
	static int num_recalls = 0;
	if (++num_recalls == 100)
		return;

	EDirection current_facing_dir = inStartFaceDirection;
	EDirection current_building_side = mNextDeliveryBuildingSide;
	FVector2D current_building_grid_position = mNextDeliveryGridPosition;

	UE_LOG(LogTemp, Warning, TEXT("START FACING DIR: %s"), *GetDirectionString(current_facing_dir));
	UE_LOG(LogTemp, Warning, TEXT("START BUILDING SIDE: %s"), *GetDirectionString(current_building_side));
	UE_LOG(LogTemp, Warning, TEXT("START GRID POSITION: %s"), *current_building_grid_position.ToString());

	int try_i = 0;
	const int num_directions = FMath::Max(int(30 - mPreviousTotalRemainingTime) / 3, 2);
	int num_tries = 100;
	std::vector<EDirection> directions;
	while (directions.size() < num_directions)
	{
		const int next_move_direction_int = (rand() % 4);
		const EDirection next_move_direction = static_cast<EDirection>(next_move_direction_int);
		if (next_move_direction == EDirection::BACK)
			continue;

		if (++try_i >= num_tries)
		{
			// Start over
			GenerateNextDelivery(inStartFaceDirection);
			return;
		}

		// Avoid taking a direction that would go outside
		{
			if (current_building_grid_position.X <= 0)
			{
				if (current_facing_dir == EDirection::LEFT && next_move_direction == EDirection::FORWARD) continue;
				if (current_facing_dir == EDirection::RIGHT && next_move_direction == EDirection::BACK) continue;
				if (current_facing_dir == EDirection::FORWARD && next_move_direction == EDirection::LEFT) continue;
				if (current_facing_dir == EDirection::BACK && next_move_direction == EDirection::RIGHT) continue;
			}

			if (current_building_grid_position.Y <= 0)
			{
				if (current_facing_dir == EDirection::FORWARD && next_move_direction == EDirection::BACK) continue;
				if (current_facing_dir == EDirection::BACK && next_move_direction == EDirection::FORWARD) continue;
				if (current_facing_dir == EDirection::RIGHT && next_move_direction == EDirection::RIGHT) continue;
				if (current_facing_dir == EDirection::LEFT && next_move_direction == EDirection::LEFT) continue;
			}

			if (current_building_grid_position.X >= BuildingArraySize - 1)
			{
				if (current_facing_dir == EDirection::RIGHT && next_move_direction == EDirection::FORWARD) continue;
				if (current_facing_dir == EDirection::LEFT && next_move_direction == EDirection::BACK) continue;
				if (current_facing_dir == EDirection::FORWARD && next_move_direction == EDirection::RIGHT) continue;
				if (current_facing_dir == EDirection::BACK && next_move_direction == EDirection::LEFT) continue;
			}

			if (current_building_grid_position.Y >= BuildingArraySize - 1)
			{
				if (current_facing_dir == EDirection::FORWARD && next_move_direction == EDirection::FORWARD) continue;
				if (current_facing_dir == EDirection::BACK && next_move_direction == EDirection::BACK) continue;
				if (current_facing_dir == EDirection::RIGHT && next_move_direction == EDirection::LEFT) continue;
				if (current_facing_dir == EDirection::LEFT && next_move_direction == EDirection::RIGHT) continue;
			}
		}

		EDirection new_facing_dir = current_facing_dir;
		EDirection new_building_side = current_building_side;
		FVector2D new_building_grid_position = current_building_grid_position;
		if (next_move_direction == EDirection::FORWARD)
		{
			new_facing_dir = current_facing_dir;

			new_building_side = current_building_side;

			new_building_grid_position += GetDirectionVector(current_facing_dir);
		}
		else if (next_move_direction == EDirection::BACK)
		{
			new_facing_dir = GetOppositeDirection(current_facing_dir);

			new_building_side = current_building_side;

			new_building_grid_position -= GetDirectionVector(current_facing_dir);
		}
		else if (next_move_direction == EDirection::LEFT)
		{
			if (current_facing_dir == EDirection::FORWARD) new_facing_dir = EDirection::LEFT;
			else if (current_facing_dir == EDirection::LEFT) new_facing_dir = EDirection::BACK;
			else if (current_facing_dir == EDirection::BACK) new_facing_dir = EDirection::RIGHT;
			else new_facing_dir = EDirection::FORWARD;

			new_building_side = (new_facing_dir == EDirection::FORWARD || new_facing_dir == EDirection::BACK) ? EDirection::RIGHT : EDirection::FORWARD;

			if (current_facing_dir == EDirection::FORWARD)
				new_building_grid_position += (current_building_side == EDirection::RIGHT ? FVector2D(0, 0) : FVector2D(-1, 0));
			else if (current_facing_dir == EDirection::BACK)
				new_building_grid_position += (current_building_side == EDirection::RIGHT ? FVector2D(1, -1) : FVector2D(0, -1));
			else if (current_facing_dir == EDirection::LEFT)
				new_building_grid_position += (current_building_side == EDirection::FORWARD ? FVector2D(-1, 0) : FVector2D(-1, -1));
			else if (current_facing_dir == EDirection::RIGHT)
				new_building_grid_position += (current_building_side == EDirection::FORWARD ? FVector2D(0, 1) : FVector2D(0, 0));
		}
		else if (next_move_direction == EDirection::RIGHT)
		{
			if (current_facing_dir == EDirection::FORWARD) new_facing_dir = EDirection::RIGHT;
			else if (current_facing_dir == EDirection::BACK) new_facing_dir = EDirection::LEFT;
			else if (current_facing_dir == EDirection::LEFT) new_facing_dir = EDirection::FORWARD;
			else new_facing_dir = EDirection::BACK;

			new_building_side = (new_facing_dir == EDirection::FORWARD || new_facing_dir == EDirection::BACK) ? EDirection::RIGHT : EDirection::FORWARD;

			if (current_facing_dir == EDirection::FORWARD)
				new_building_grid_position += (current_building_side == EDirection::RIGHT ? FVector2D(1, 0) : FVector2D(0, 0));
			else if (current_facing_dir == EDirection::BACK)
				new_building_grid_position += (current_building_side == EDirection::RIGHT ? FVector2D(0, -1) : FVector2D(-1, -1));
			else if (current_facing_dir == EDirection::LEFT)
				new_building_grid_position += (current_building_side == EDirection::FORWARD ? FVector2D(-1, 1) : FVector2D(-1, 0));
			else if (current_facing_dir == EDirection::RIGHT)
				new_building_grid_position += (current_building_side == EDirection::FORWARD ? FVector2D(0, 0) : FVector2D(0, -1));
		}

		if (new_building_grid_position.X == -1 && new_building_grid_position.Y == -1)
			continue;

		if (new_building_grid_position.X == -1)
		{
			new_building_grid_position.X = 0;
			new_building_side = EDirection::LEFT;
		}
		else if (new_building_grid_position.Y == -1)
		{
			new_building_grid_position.Y = 0;
			new_building_side = EDirection::BACK;
		}
		/*
		else if (new_facing_dir == EDirection::FORWARD || new_facing_dir == EDirection::BACK)
			new_building_side = EDirection::RIGHT;
		else
			new_building_side = EDirection::FORWARD;
		*/

		UE_LOG(LogTemp, Warning, TEXT("      try***TURN DIR: %s"), *GetDirectionString(next_move_direction));
		UE_LOG(LogTemp, Warning, TEXT("         try***NEW FACING DIR: %s"), *GetDirectionString(new_facing_dir));
		UE_LOG(LogTemp, Warning, TEXT("         try***NEW BUILDING SIDE: %s"), *GetDirectionString(new_building_side));
		UE_LOG(LogTemp, Warning, TEXT("         try***NEW GRID POSITION: %s"), *new_building_grid_position.ToString());

		if (new_building_grid_position.X < 0 || new_building_grid_position.Y < 0 || new_building_grid_position.X >= BuildingArraySize || new_building_grid_position.Y >= BuildingArraySize)
			continue;

		UE_LOG(LogTemp, Warning, TEXT("TURN DIR: %s"), *GetDirectionString(next_move_direction));
		UE_LOG(LogTemp, Warning, TEXT("  NEW FACING DIR: %s"), *GetDirectionString(new_facing_dir));
		UE_LOG(LogTemp, Warning, TEXT("  NEW BUILDING SIDE: %s"), *GetDirectionString(new_building_side));
		UE_LOG(LogTemp, Warning, TEXT("  NEW GRID POSITION: %s"), *new_building_grid_position.ToString());

		current_building_grid_position = new_building_grid_position;
		current_building_side = new_building_side;
		current_facing_dir = new_facing_dir;

		directions.push_back(next_move_direction);
	}

	if (current_building_grid_position == mNextDeliveryGridPosition)
		// FMath::Abs(current_building_grid_position.X - mNextDeliveryGridPosition.X) +
		// FMath::Abs(current_building_grid_position.Y - mNextDeliveryGridPosition.Y) <= 1 ) // Very close, start over
	{
		GenerateNextDelivery(inStartFaceDirection);
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("SUMMARY: ------"));
	for (const EDirection& dir : directions)
	{
		UE_LOG(LogTemp, Warning, TEXT("TURN DIR: %s"), *GetDirectionString(dir));
	}
	UE_LOG(LogTemp, Warning, TEXT("BUILDING SIDE: %s"), *GetDirectionString(current_building_side));
	UE_LOG(LogTemp, Warning, TEXT("GRID POSITION: %s"), *current_building_grid_position.ToString());
	UE_LOG(LogTemp, Warning, TEXT("=========================="));

	const FVector delivery_world_pos = GetGridWorldPosition(current_building_grid_position, current_building_side);
	UE_LOG(LogTemp, Warning, TEXT("delivery_world_pos: %s"), *delivery_world_pos.ToString());
	// DrawDebugLine(GetWorld(), delivery_world_pos, delivery_world_pos + FVector(0, 0, 999999), FColor::Red, true, 15.0f, 0, 100.0f);

	const bool is_delivery_in_boundary = (current_building_grid_position.X == 0 || current_building_grid_position.Y == 0 ||
		current_building_grid_position.X == BuildingArraySize - 1 ||
		current_building_grid_position.Y == BuildingArraySize - 1);
	const bool change_street_side = false; // !is_delivery_in_boundary && (rand() % 2 == 0); NOT WORKING
	mNextDeliveryBuildingSide = (change_street_side ? GetOppositeDirection(current_building_side) : current_building_side);
	mNextDeliveryGridPosition = current_building_grid_position + (change_street_side ? GetDirectionVector(current_building_side) : FVector2D(0, 0));
	verify(mNextDeliveryGridPosition.X >= 0 && mNextDeliveryGridPosition.Y >= 0 && mNextDeliveryGridPosition.X < BuildingArraySize&& mNextDeliveryGridPosition.Y < BuildingArraySize);

	mShowArrowsTime = (mPreviousTotalRemainingTime / 2);

	if (mNextDeliveryTrigger)
		mNextDeliveryTrigger->SetActorHiddenInGame(true);

	AActor* next_delivery_building = mBuildings.at(mNextDeliveryGridPosition.Y).at(mNextDeliveryGridPosition.X);
	UE_LOG(LogTemp, Warning, TEXT("mNextDeliveryGridPosition: %s"), *mNextDeliveryGridPosition.ToString());
	UE_LOG(LogTemp, Warning, TEXT("mNextDeliveryBuildingSide: %s"), *GetDirectionString(mNextDeliveryBuildingSide));
	verify(next_delivery_building != nullptr);
	TArray<UChildActorComponent*> next_delivery_building_child_actor_components;
	next_delivery_building->GetComponents<UChildActorComponent>(next_delivery_building_child_actor_components, true);
	for (UChildActorComponent* next_delivery_building_child_actor_comp : next_delivery_building_child_actor_components)
	{
		UE_LOG(LogTemp, Warning, TEXT("next_delivery_building_child_actor_comp->GetName(): %s"), *next_delivery_building_child_actor_comp->GetName());
		if (next_delivery_building_child_actor_comp->ComponentHasTag(FName(FString("DeliveryTrigger_") + GetDirectionString(mNextDeliveryBuildingSide))))
		{
			mNextDeliveryTrigger = next_delivery_building_child_actor_comp->GetChildActor();
			UE_LOG(LogTemp, Warning, TEXT("FOUND!"), *next_delivery_building_child_actor_comp->GetName());
			break;
		}
	}
	verify(mNextDeliveryTrigger != nullptr);
	mNextDeliveryTrigger->SetActorHiddenInGame(false);

	num_recalls = 0;

	TArray<int> directions_array_for_blueprint;
	for (const EDirection& direction : directions)
	{
		directions_array_for_blueprint.Push(static_cast<int>(direction));
	}

	mTimeSinceShowArrows = 0.0f;
	ShowDirectionArrows(directions_array_for_blueprint);

	mRemainingTime = mPreviousTotalRemainingTime - 1.0f; // Reset remaining time
	mRemainingTime = FMath::Max(mRemainingTime, 12.0f);
	mPreviousTotalRemainingTime = mRemainingTime;
	if (!inIsFirstDelivery)
	{
		mScore += 100.0f;
		SetScore(mScore);
	}
}

void AGameJam2021PlayerController::OnOverlap(AActor* inOverlappedActor)
{
	UE_LOG(LogTemp, Warning, TEXT("OnOverlap with %s"), *inOverlappedActor->GetName());
	if (mNextDeliveryTrigger)
	{
		UE_LOG(LogTemp, Warning, TEXT("mNextDeliveryTrigger->GetNam(): %s"), *mNextDeliveryTrigger->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("mNextDeliveryTrigger is NULL"));
	}

	if (inOverlappedActor == mNextDeliveryTrigger)
	{
		OnDeliveryMade();
	}
}

void AGameJam2021PlayerController::SetupInputComponent()
{
	// set up gameplay key bindings
	Super::SetupInputComponent();

	InputComponent->BindAction("GoForward", IE_Pressed, this, &AGameJam2021PlayerController::GoForwardPressed);
	InputComponent->BindAction("GoBack", IE_Pressed, this, &AGameJam2021PlayerController::GoBackPressed);
	InputComponent->BindAction("TurnLeft", IE_Pressed, this, &AGameJam2021PlayerController::TurnLeftPressed);
	InputComponent->BindAction("TurnRight", IE_Pressed, this, &AGameJam2021PlayerController::TurnRightPressed);
	InputComponent->BindAction("GoForward", IE_Released, this, &AGameJam2021PlayerController::GoForwardReleased);
	InputComponent->BindAction("GoBack", IE_Released, this, &AGameJam2021PlayerController::GoBackReleased);
	InputComponent->BindAction("TurnLeft", IE_Released, this, &AGameJam2021PlayerController::TurnLeftReleased);
	InputComponent->BindAction("TurnRight", IE_Released, this, &AGameJam2021PlayerController::TurnRightReleased);

	FInputActionBinding& toggle = InputComponent->BindAction("Pause", IE_Pressed, this, &AGameJam2021PlayerController::OnPausePressed);
	toggle.bExecuteWhenPaused = true;
}

void AGameJam2021PlayerController::GoForwardPressed()
{
	mGoingForward = true;
}
void AGameJam2021PlayerController::GoBackPressed()
{
	mGoingBack = true;
}
void AGameJam2021PlayerController::TurnLeftPressed()
{
	mTurningLeft = true;
}
void AGameJam2021PlayerController::TurnRightPressed()
{
	mTurningRight = true;
}

void AGameJam2021PlayerController::GoForwardReleased()
{
	mGoingForward = false;
}
void AGameJam2021PlayerController::GoBackReleased()
{
	mGoingBack = false;
}
void AGameJam2021PlayerController::TurnLeftReleased()
{
	mTurningLeft = false;
}
void AGameJam2021PlayerController::TurnRightReleased()
{
	mTurningRight = false;
}

void AGameJam2021PlayerController::OnPausePressed()
{
	OnPausePressedBP();
}