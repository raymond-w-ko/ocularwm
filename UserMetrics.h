#pragma once

struct UserMetrics {
    // standing height of user
    float heightInCm;

    // interpupillary distance of user
    float IpdInMm;

    // How far bulging in or out your eye are. Zero would mean you aren't
    // human, and your eyes are inside you brain. Positive number numbers means
    // your eyes are popping out
    float eyesCoronalOffsetInCm;

    // How far down from the from the very top of your skull your eyes are.
    // Zero would mean your eyes are outside your skull, and above your head.
    float eyesTransverseOffsetInCm;

    // For US cretins, like the author of this software
    void SetHeightFromInches(float inches)
    {
        heightInCm = inches * 2.54f;
    }

    Ogre::Vector3 CalculateLeftEyeOffset()
    {
        return Ogre::Vector3(
            -(IpdInMm / 10.0f / 2.0f),
            heightInCm - eyesTransverseOffsetInCm,
            -eyesCoronalOffsetInCm);
    }

    Ogre::Vector3 CalculateRightEyeOffset()
    {
        return Ogre::Vector3(
            +(IpdInMm / 10.0f / 2.0f),
            heightInCm - eyesTransverseOffsetInCm,
            -eyesCoronalOffsetInCm);
    }
};
