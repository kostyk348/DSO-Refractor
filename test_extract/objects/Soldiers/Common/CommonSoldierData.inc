rem **** Filename: CommonSoldierData.inc
rem ** Make soldier be visible 3 times the normal culling distance
ObjectTemplate.cullRadiusScale 3.0
ObjectTemplate.hasDynamicShadow 1

rem .setSkeletonCollisionBone <bonename> <bone radius in meters> <material> 
ObjectTemplate.setSkeletonCollisionBone Bip01_Head 0.05 40
ObjectTemplate.setSkeletonCollisionBone Bip01_L_Forearm 0.03 42
ObjectTemplate.setSkeletonCollisionBone Bip01_R_Forearm 0.03 42
ObjectTemplate.setSkeletonCollisionBone Bip01_Spine2 0.3 41
ObjectTemplate.setSkeletonCollisionBone Bip01_Pelvis 0.3 41
ObjectTemplate.setSkeletonCollisionBone Bip01_L_Calf 0.05 42
ObjectTemplate.setSkeletonCollisionBone Bip01_R_Calf 0.05 42
ObjectTemplate.setSkeletonCollisionBone Bip01_L_Foot 0.05 42
ObjectTemplate.setSkeletonCollisionBone Bip01_R_Foot 0.05 42

rem *** Different camera positions for different soldier poses.
objectTemplate.center1pHands -0.12/-1.56/0.1
ObjectTemplate.setPoseCameraPos c_BfSoldierStanding 0/0.65/0
ObjectTemplate.setPoseCameraPos c_BfSoldierCrouching 0/0.12/0
ObjectTemplate.setPoseCameraPos c_BfSoldierLying 0/-0.7/0
ObjectTemplate.setCharacterHeight -1.00

ObjectTemplate.geometry BodyCollision

ObjectTemplate.setParachuteDrag 24.00
ObjectTemplate.setParachuteSpeed 30.00

ObjectTemplate.HasArmor 1
ObjectTemplate.explosionForceMax 600
ObjectTemplate.explosionDamage 0.0
ObjectTemplate.HitPoints 30
ObjectTemplate.MaxHitPoints 30
ObjectTemplate.timeToLiveAfterDeath 40
ObjectTemplate.healFactor 0.5
ObjectTemplate.SpeedMod 0.5
ObjectTemplate.angleMod 1
ObjectTemplate.Material 40
ObjectTemplate.hasMobilePhysics 1
ObjectTemplate.drag 0.375
ObjectTemplate.mass 100
ObjectTemplate.hasCollisionPhysics 1
ObjectTemplate.hasResponsePhysics 1
rem ObjectTemplate.Grip 20
ObjectTemplate.setAnimationState 0 Lb_Stand
ObjectTemplate.setAnimationState 1 Ub_StandAim
ObjectTemplate.setTurnLeftRightAngle 20.0 14.0
ObjectTemplate.setPointUpDownAngle 20.0  20.0
ObjectTemplate.setLiePointUpDownAngle 0.0 -6.0
ObjectTemplate.set1pFov 0.47
ObjectTemplate.networkableInfo NetSoldierInfo
ObjectTemplate.hpLostWhileDamageFromWater 1
ObjectTemplate.WaterDamageDelay 90
ObjectTemplate.DamageFromWater 1

ObjectTemplate.isPortalPassing 1
ObjectTemplate.portalPassingPosition 0/0/0

objectTemplate.aiTemplate Soldier

ObjectTemplate.addTemplate SoldierEntry

ObjectTemplate.addTemplate SoldierCamera
ObjectTemplate.setIsFirstPersonPart 2

ObjectTemplate.addTemplate Parachute
ObjectTemplate.setPosition 0/0.3/0
ObjectTemplate.setRotation 0/0/0

ObjectTemplate.repairDistance 3.0
ObjectTemplate.disarmMinesDistance 2.0
ObjectTemplate.healDistance 10.0
ObjectTemplate.explosionForceMod 150