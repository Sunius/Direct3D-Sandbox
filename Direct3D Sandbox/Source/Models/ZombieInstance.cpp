#include "PrecompiledHeader.h"
#include "Constants.h"
#include "PlayerInstance.h"
#include "System.h"
#include "Source\Audio\AudioManager.h"
#include "Source\Graphics\IShader.h"
#include "ZombieInstance.h"

const float ZombieInstance::kAnimationPeriods[ZombieStates::StateCount] = { 2.0f, 0.8f, 1.167f, 1.333f };
const bool ZombieInstance::kDoesAnimationLoop[ZombieStates::StateCount] = { true, true, true, false };
const float ZombieInstance::kAnimationTransitionLength = 0.5f;
const float ZombieInstance::kZombieDistancePerRunningAnimationTime = 1.5f;
const float ZombieInstance::kZombieBodyLastingTime = 20.0f;
const float ZombieInstance::kZombieHitInterval = 1.0f;

static const float kNearPlayerSoundInterval = 5.0f;
static const float kFootStepInterval = 0.4f;

ZombieInstance::ZombieInstance(const ModelParameters& modelParameters, PlayerInstance& targetPlayer, 
							   const vector<shared_ptr<ZombieInstanceBase>>& zombies) :
	ZombieInstanceBase(IShader::GetShader(ShaderType::ANIMATION_NORMAL_MAP_SHADER), 
					   L"Assets\\Animated Models\\Zombie.animatedModel", 
					   L"Assets\\Textures\\Zombie.dds",
					   L"Assets\\Normal Maps\\Zombie.dds",
					   modelParameters,
					   targetPlayer,
					   kZombieDistancePerRunningAnimationTime / kAnimationPeriods[ZombieStates::Running]),
	m_AnimationStateMachine(ZombieStates::Idle),
	m_Zombies(zombies),
	m_LastHitPlayerAt(static_cast<float>(Tools::GetTime())),
	m_LastMadeNearPlayerSound(-kNearPlayerSoundInterval),
	m_LastFootStep(-kFootStepInterval),
	m_NearPlayerSound(AudioManager::GetCachedSound(L"Assets\\Sounds\\ZombieNear.wav", false, true)),
	m_FootStepSound(AudioManager::GetCachedSound(L"Assets\\Sounds\\FootStep.wav", false, true)),
	m_PunchSound(AudioManager::GetCachedSound(L"Assets\\Sounds\\Punch.wav", false, true))
{
	for (int i = 0; i < ZombieStates::Death; i++)
	{
		m_AnimationStateMachine.SetAnimationProgress(i, Tools::Random::GetNextReal(0.0f, 1.0f));
	}

	m_AnimationStateMachine.SetAnimationProgress(ZombieStates::Death, 0.145f);
}

ZombieInstance::~ZombieInstance()
{
}

bool ZombieInstance::CanMoveTo(const DirectX::XMFLOAT2& position, const vector<shared_ptr<ZombieInstanceBase>>& zombies,
	const ZombieInstanceBase* thisPtr)
{
	auto zombieCount = zombies.size();

	for (auto i = 0u; i < zombieCount; i++)
	{
		auto zombie = zombies[i].get();

		if (zombie != thisPtr)
		{
			auto distanceSqr = zombie->HorizontalDistanceSqrTo(position);

			if (distanceSqr < 1.0f)
			{
				return false;
			}
		}
	}

	return true;
}

void ZombieInstance::Update(const RenderParameters& renderParameters)
{
	ZombieStates targetState;
	
	auto emitterPosition = m_Parameters.position;
	emitterPosition.y += 1.6f;
	m_AudioEmitter.SetPosition(m_Parameters.position, DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f));

	float distanceToPlayerSqr;

	if (!m_IsDead)
	{
		auto playerPosition = m_TargetPlayer.GetPosition();
		auto angleY = -atan2(m_Parameters.position.z - playerPosition.z, m_Parameters.position.x - playerPosition.x) - DirectX::XM_PI / 2.0f;

		if (m_TargetPlayer.GetGameState() == GameState::Playing)
		{
			DirectX::XMFLOAT2 vectorToPlayer(playerPosition.x - m_Parameters.position.x, playerPosition.z - m_Parameters.position.z);
			distanceToPlayerSqr = vectorToPlayer.x * vectorToPlayer.x + vectorToPlayer.y * vectorToPlayer.y;	

			if (distanceToPlayerSqr > 1.5f)
			{
				if (distanceToPlayerSqr > 100.0f * 100.0f)
				{
					m_IsDead = true;
					System::GetInstance().RemoveModel(this);
					return;
				}
				else
				{
					auto vectorMultiplier = m_Speed * renderParameters.frameTime / sqrt(distanceToPlayerSqr);

					DirectX::XMFLOAT2 newPosition(m_Parameters.position.x + vectorMultiplier * vectorToPlayer.x, 
						m_Parameters.position.z + vectorMultiplier * vectorToPlayer.y);

					if (CanMoveTo(newPosition, m_Zombies, this))
					{
						targetState = ZombieStates::Running;

						m_Parameters.position.x = newPosition.x;
						m_Parameters.position.z = newPosition.y;
					}
					else
					{
						targetState = ZombieStates::Idle;
					}
				}
			}
			else
			{
				targetState = ZombieStates::Hitting;

				if (renderParameters.time - m_LastMadeNearPlayerSound >= kNearPlayerSoundInterval)
				{
					m_LastMadeNearPlayerSound = renderParameters.time;
					m_NearPlayerSound.Play3D(m_AudioEmitter);
				}
			}
		}
		else
		{
			targetState = ZombieStates::Idle;
		}

		SetRotation(DirectX::XMFLOAT3(0.0f, angleY, 0.0f));
	}
	else
	{
		targetState = ZombieStates::Death;

		if (renderParameters.time - m_DeathTime > kZombieBodyLastingTime)
		{
			System::GetInstance().RemoveModel(this);
		}
	}

	Assert(targetState >= 0 && targetState < ZombieStates::StateCount);
	m_AnimationStateMachine.Update(renderParameters, targetState);

	if (m_AnimationStateMachine.GetCurrentAnimationState() == ZombieStates::Hitting && 
		!m_AnimationStateMachine.IsTransitioningAnimationStates() &&
		m_AnimationStateMachine.GetCurrentStateAnimationProgress() > 0.1f && 
		m_AnimationStateMachine.GetCurrentStateAnimationProgress() < 0.2f &&
		renderParameters.time - m_LastHitPlayerAt >= kZombieHitInterval)
	{
		m_LastHitPlayerAt = renderParameters.time;
		m_TargetPlayer.TakeDamage(Tools::Random::GetNextReal<float>(0.03f, 0.1f));
		m_PunchSound.Play3D(m_AudioEmitter);
	}
	else if (renderParameters.time - m_LastFootStep >= kFootStepInterval &&
			 m_AnimationStateMachine.GetCurrentAnimationState() == ZombieStates::Running && 
			 !m_AnimationStateMachine.IsTransitioningAnimationStates() && 
			 distanceToPlayerSqr < 100.0f)
	{
		m_LastFootStep = renderParameters.time;
		m_FootStepSound.Play3D(m_AudioEmitter, 8.0f);
	}
}

void ZombieInstance::Render3D(RenderParameters& renderParameters)
{
	m_AnimationStateMachine.SetRenderParameters(renderParameters);

	ZombieInstanceBase::Render3D(renderParameters);
}

shared_ptr<ZombieInstanceBase> ZombieInstance::Spawn(PlayerInstance& targetPlayer, const vector<shared_ptr<ZombieInstanceBase>>& zombies)
{
	ModelParameters zombieParameters;

	do
	{
		zombieParameters = GetRandomZombieParameters(targetPlayer);
	}
	while (!CanMoveTo(DirectX::XMFLOAT2(zombieParameters.position.x, zombieParameters.position.z), zombies, nullptr));

	return shared_ptr<ZombieInstanceBase>(new ZombieInstance(zombieParameters, targetPlayer, zombies));
}