// Publisher: Fullike (https://github.com/fullike)
// Copyright 2024. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"

ROADBUILDER_API FVector2D GetSpiralPos(double initX, double initY, double initTheta, double initCurv, double dCurv, double length);
ROADBUILDER_API double GetSpiralRadian(double initTheta, double initCurv, double dCurv, double length);
ROADBUILDER_API double ComputeSpiralLength(double theta0, double theta1, double k0, double k1);