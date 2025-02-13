#pragma once

struct RenderParameters;
class IModelInstance
{
private:
	IModelInstance(const IModelInstance& other);				// Not implemented (copying is not allowed)
	IModelInstance& operator=(const IModelInstance& other);		// Not implemented (copying is not allowed)

public:
	IModelInstance();
	virtual ~IModelInstance();
	
	virtual void Update(const RenderParameters& renderParameters) = 0;
	virtual void Render3D(RenderParameters& renderParameters) = 0;
	virtual void Render2D(RenderParameters& renderParameters) = 0;
};

