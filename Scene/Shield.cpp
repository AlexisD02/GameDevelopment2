#include "Shield.h"
#include "SceneGlobals.h"
#include "MathHelpers.h"

// Shield entity rotates around the Y-axis and pulsates in scale, to give a visual effect.
bool Shield::Update(float frameTime)
{
    // Accumulate time for animation
    mElapsed += frameTime;

    // Rotate the shield: a steady rotation about the Y-axis.
    float rotationSpeed = 15.0f;
    Transform().RotateLocalY(ToRadians(rotationSpeed * frameTime));

    // Make the shield pulse in scale
    float pulseFrequency = 0.5f;
    float pulseAmplitude = 0.05f;
    float scaleFactor = 1.0f + pulseAmplitude * std::sin(2.0f * std::numbers::pi * pulseFrequency * mElapsed);

    // Update the scale of the shield's transform.
    Transform().SetScale(scaleFactor);

    return true;
}
