#pragma once

#include "ZombieInstanceBase.h"

class PlayerInstance;
class SuperZombieInstance :
	public ZombieInstanceBase
{
	SuperZombieInstance(const ModelInstance& other);				// Not implemented (no copying allowed)
	SuperZombieInstance& operator=(const ModelInstance& other);		// Not implemented (no copying allowed)

public:
	SuperZombieInstance(const ModelParameters& modelParameters, PlayerInstance& targetPlayer);
	virtual ~SuperZombieInstance();
	
	virtual void Update(const RenderParameters& renderParameters) { }
	virtual void Render3D(RenderParameters& renderParameters);

	static shared_ptr<ZombieInstanceBase> Spawn(PlayerInstance& targetPlayer);
};

