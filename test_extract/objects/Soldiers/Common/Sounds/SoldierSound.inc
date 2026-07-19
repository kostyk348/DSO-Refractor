rem *************************
rem **** Soldier settings ***
rem *************************
SoldierSound.setStandFrequency 1.5

SoldierSound.setRunFrequency 0.36
SoldierSound.setRandomRunFrequency 0.017

SoldierSound.setWalkFrequency 0.66
SoldierSound.setRandomWalkFrequency 0.04

SoldierSound.setSwimFrequency 1
SoldierSound.setRandomSwimFrequency 0.025

SoldierSound.setCrouchFrequency 0.50
SoldierSound.setRandomCrouchFrequency 0.04

SoldierSound.setCrawlFrequency 0.6
SoldierSound.setRandomCrawlFrequency 0.3

SoldierSound.setLadderFrequency 0.46
SoldierSound.setRandomLadderFrequency 0.01

rem ********************************************************************

rem *** Upright ***
ObjectTemplate.loadSoundScript SoldierStop.ssc
ObjectTemplate.loadSoundScript SoldierWalk.ssc
ObjectTemplate.loadSoundScript SoldierRun.ssc
ObjectTemplate.loadSoundScript SoldierJump.ssc
ObjectTemplate.loadSoundScript SoldierJumpLand.ssc

rem *** Crouch ***
ObjectTemplate.loadSoundScript SoldierToCrouch.ssc
ObjectTemplate.loadSoundScript SoldierFromCrouch.ssc
ObjectTemplate.loadSoundScript SoldierCrouchMove.ssc

rem *** Crawl ***
ObjectTemplate.loadSoundScript SoldierToCrawl.ssc
ObjectTemplate.loadSoundScript SoldierStandUp.ssc
ObjectTemplate.loadSoundScript SoldierCrawlMove.ssc

rem *** Swim ***
ObjectTemplate.loadSoundScript SoldierToSwim.ssc
ObjectTemplate.loadSoundScript SoldierSwim.ssc
ObjectTemplate.loadSoundScript SoldierSwimStand.ssc
ObjectTemplate.loadSoundScript SoldierFromSwim.ssc

rem *** Parachute ***
ObjectTemplate.loadSoundScript SoldierOpenParachute.ssc
ObjectTemplate.loadSoundScript SoldierFallingHigh.ssc
ObjectTemplate.loadSoundScript SoldierParachuteLand.ssc

rem *** Injury ***
ObjectTemplate.loadSoundScript SoldierHitDamage.ssc
ObjectTemplate.loadSoundScript SoldierFFHitDamage.ssc
ObjectTemplate.loadSoundScript SoldierKilled.ssc
ObjectTemplate.loadSoundScript SoldierHealedByMedic.ssc

rem *** Ladder ***
ObjectTemplate.loadSoundScript SoldierClimbLadder.ssc

rem *** RadioVoice ***
ObjectTemplate.loadSoundScript SoldierVoice.ssc

ObjectTemplate.loadSoundScript SoldierCharge.ssc
